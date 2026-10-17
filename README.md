# TCP Server in C

This is a low-level TCP server built in C language on Linux to explore sockets, non-blocking I/O, `epoll`, thread pools, and low level systems.

## Overview

I build this project not to create a production ready server but to understand what happens under the high level framewoks.

The implementation uses:

- Non-blocking TCP sockets
- Listener uses Level Triggered `epoll` with while loop for optimization
- Client uses Edge Trigger`epoll` with while loop to drain all data at once
- A `pthread` worker pool
- A task queue and a separate completion queue
- `eventfd` to notify the event loop about completed work
- Custom input and output buffers in Connection struts
- HTTP request parsing, routing, controllers, and response generation

## Why I Built This


I have been working with Java and Spring Boot building projects, but I wanted to understand more clearly what happens under the abstraction those frmeworks provide. I wanted to understand how server manages thousand of client connectiosn at once without blocking and how network I/O interact with application.

I chose C because I have some prior knowledge of it from my university studies also it gave me direct experience with linux sockets, file descriptors , epoll , etc without much abstraction.

## What I Built


The server accepts TCP conenctions , read HTTP request bytes into connection input buffer , dispatch request to worker pool , route the request to the controller then generate and send back the response.

The event loop owns socket I/O. Worker threads process requests and prepare response bytes, but they do not write directly to client sockets. When a worker finishes, it places the connection in a completion queue and signals an `eventfd`. The event loop handles that notification and sends the response when the client socket is writable.

## Build and Run

### Requirements

- Linux
- GCC
- GNU Make
- POSIX thread support

### Build

From the project root, run:

```bash
make
```

### Run

Start the server with:

```bash
./server
```

The server's configured port is `8080` in the examples below. If the port is configured elsewhere in the source, use that value instead.

## Architecture

```text
                         Client
                           |
                           v
                    Listening socket
                           |
                        accept()
                           |
                           v
                  Client Connection
                  input / output buffers
                           |
                           v
                   epoll event loop
                           |
               read bytes until EAGAIN
                           |
                           v
                 Complete HTTP request
                           |
                           v
                       Task queue
                           |
                           v
                    Worker threads
                 HTTP parser / router
                  controller / response
                           |
                           v
                  Connection output buffer
                           |
                           v
                   Completion queue
                           |
                       eventfd
                           |
                           v
                   epoll event loop
                           |
                  enable EPOLLOUT
                           |
                           v
                 write response bytes
                           |
                           v
                         Client
```

The main components have separate responsibilities:

| Component | Responsibility |
|---|---|
| Event loop | Handles socket I/O and connection state. |
| `epoll` | Reports readiness events for registered file descriptors. |
| Connection buffers | Store received request bytes and pending response bytes. |
| Task queue | Holds request-processing work waiting for a worker. |
| Worker pool | Processes requests and generates responses. |
| Completion queue | Records connections whose worker processing has finished. |
| `eventfd` | Wakes the event loop when completion work may be available. |

### Ownership and coordination


The event loop is responsible for the socket reads/writes and changes to the `epoll`. Worker thread processes the request then places the response in the output buffer of the connection and the worker adds the connection to the completion queue and signals the `eventfd`.

`eventfd` is only a notification mechanism. The completion queue carry the completion information, and the connection's output buffer holds the response bytes. After receiving the notification, the event loop drains the completion queue, enables write readiness with `EPOLL_CTL_MOD`, and sends the response when `epoll` reports `EPOLLOUT`.

The event loop and workers can still refer to the same `Connection` object. The current server uses connection state and a `close_requested` flag fpr some cleanup, but ownership and error handling are not as good as they would need to be in a production server.

Details of how epoll events are dispatched to the right handler (`EpollContext`) are in [internals.md](/internals.md).

## Request Lifecycle

1. **Accept a connection.** The server creates a listening socket, binds it to a local address and port, and calls `listen()`. The event loop accepts an incoming connection with `accept()`. The accepted client socket has its own file descriptor and a  `Connection` object.
2. **Register the client socket.** The accepted socket is configured for non-blocking I/O and registered with `epoll`, for `EPOLLIN | EPOLLET`.
3. **Read available bytes.** When `epoll_wait()` reports that the socket is readable, the event loop calls `recv()` repeatedly until no more data is left, indicated by `EAGAIN` or `EWOULDBLOCK`. Received bytes are appended to the connection's input buffer.
4. **Identify a complete request.** The server examines the accumulated bytes to determine whether a complete HTTP request is available. When it is identified, it marks the connection as processing and submits work to the task queue.
5. **Process the request.** A worker retrieves the task, processes the HTTP request, routes it to the controller, and created a response. The response bytes are appended to the connection's output buffer.
6. **Report completion.** The worker adds the connection to the completion queue and writes to `eventfd` so the event loop knows that completion work is available.
7. **Return control to the event loop.** The event loop handles the `eventfd` notification and drains the completion queue. If the connection is still usable, it updates the connection state and modifies the existing `epoll` registration to include `EPOLLOUT`.
8. **Write the response.** When the client socket is writable, the event loop sends bytes from the output buffer.
9. **Resume monitoring input.** After the output buffer is empty, the event loop removes `EPOLLOUT` interest and continues monitoring the connection for incoming data.

## Connection Lifecycle

Each accepted client connection has a `Connection` structure containing its file descriptor, input and output buffers, parser state, and other connection metadata.

The connection uses two primary processing states:

- `CONNECTION_IDLE`: no worker is currently processing a request for the connection.
- `CONNECTION_PROCESSING`: a worker is processing a request associated with the connection.

When the event loop identifies that the request is complete it marks the connection of that request as processign and submit the task. When the task finishes properly and there is no indication that connection has disconnected it notifies the event loop and changes the state of connection to idle.

Closing a connection requires care. If a client disconnects while a worker is using a `Connection *` freeing the object immediately can cause a use-after-free. The `close_requested` flag records that closure has been requested so cleanup can be coordinated with processing state.

## Threading Model

The server uses a fixed worker pool rather than creating a new thread for every request.As , reusable workers provide better efficiency rather than creating new thread for every task.

The event loop receives bytes and identifies complete requests. It then submits a task to a shared task queue. Worker threads wait for tasks in the `thread_pool` and process them when they are available. A mutex protects the queue's shared state, and a condition variable allows workers to sleep while the queue is empty and wake when work is submitted.

The task queue and completion queue serve two different purposes:

- **Task queue:** work waiting to be processed by a worker.
- **Completion queue:** connections whose worker processing has finished and which need attention from the event loop.

Both queues currently have fixed capacities. 

## Event-Driven I/O

The server uses non-blocking sockets and Linux `epoll` to manage multiple client connections through an event loop.

A non-blocking socket operation may transfer fewer bytes than requested or return before any data can be transferred. The code must handle conditions such as `EAGAIN` and `EWOULDBLOCK` instead of assuming that one call reads or writes everything.

The server uses Level Triggered(LT) notification through `EPOLLIN` for the listener . While using  Edge Triggered(ET) through `EPOLLET` for the clients, the event loop must drain available input until the socket operation reports that no more data is currently available. The same  care is needed when handling output and partial writes.


## HTTP Processing

HTTP processing is separated into parsing, routing, controller execution, and response generation.

The event loop accumulates incoming bytes in the connection's input buffer. Once the implementation identifies a complete request, a worker processes it. The router matches the HTTP method and path to a supported route and invokes the corresponding controller. The response-generation code creates the response bytes, which are stored in the connection's output buffer for the event loop to send.

The current routing implementation includes these endpoints:

| Method | Path | Purpose |
|---|---|---|
| `GET` | `/` | Root endpoint. |
| `GET` | `/users` | Users endpoint. |
| `POST` | `/print` | Prints the request body and generates a response. |

This is a limited HTTP implementation, not a complete implementation of the HTTP specification. Its behavior is bounded by the request parser and response-handling code in the project.

## Bugs I Encountered and How I Investigated Them

### Double-free and connection lifetime

During  stress test with about 1000 concurrent connections, the server processed roughly 20,932 requests per second before encountering a double-free. The issue was the event loop could destroy the Connection stuct even when the worker held a raw pointer to it.

I added connection state first because I doubted that the connectionw as being accessed somewhere else it didn't fixed the whole situation then I added  a `close_requested` flag so closure could be coordinated with in-progress work. This reduced one class of lifetime error, but it did not eliminate every possible ownership problem.

### Use-after-free after a disconnect

After the first fix, another segmentation fault appeared when `recv()` returned `0`, indicating that the peer had closed the connection. The event loop freed the connection but then continued processing the same event and could reach the write-handling path. I fixed that control-flow issue by tracking that the connection had been destroyed and avoiding further access to it in that iteration.

These bugs taught me that connection cleanup is not just a call to `close()` and `free()`. The code must account for every place that can still access the connection, especially when work is shared between an event loop and worker threads.

### File-descriptor limits

Higher-concurrency tests initially ran into the process file-descriptor limit, which was 1,024. The server needs descriptors for its listening socket and client connections, and `wrk` needs descriptors for its own connections. I checked and raised the limits for both processes before repeating the tests.

This showed that the maximum concurrency of a test is affected by operating-system limits as well as by the server implementation.

### Request input was never cleared on a keep-alive connection

While reading the `wrk` output I divided the bytes read by the number of requests and got about 36 to 52 KB per response. A single `curl -i` showed a 114-byte response, so the numbers did not match.

The cause was that the connection's input buffer and the HTTP parser were only reset in `connection_init`. After the first request on a connection, the parser stayed in its complete state and nothing cleared `conn->input`. Each time a worker parsed the connection, it parsed the first request and treated everything after it, which was all the later requests, as the body. The response code uses the body when there is one, so the server sent the accumulated input back to the client.

The fix was to reset the connection once the worker has finished the request and built the response: set `conn->input.length = 0` and call `http_parser_init(&conn->parser)`. After that, `wrk` read about 114 bytes per response, the same as `curl`. The cost is that pipelined requests (a second request sent before the first is answered) are not supported.

### Listen backlog of 10

The listening socket was created with `listen(server_fd, 10)`. When `wrk` opened 2,000 connections at once, the kernel's queue of pending connections overflowed. With `nstat` I recorded between 4,782 and 11,755 `TcpExtListenOverflows` per run. Clients whose connection was dropped had to retry, which matches the maximum latencies of about 1.7 to 1.9 seconds and the 25 to 91 timeouts per run in three of those five runs.

I changed the backlog to `SOMAXCONN`, which is 4,096 on this machine. In the later tests the overflow and drop counters were 0 in all 15 runs, and there were no timeouts. I changed the input handling at the same time, so I cannot say how much of the timeout improvement came from each change. The counters, though, only measure the pending-connection queue.

### Restarting the server

I also set `SO_REUSEADDR` on the listening socket before `bind()`, so the server can be restarted right after it is stopped without waiting for old connections to leave `TIME_WAIT`.

## Testing

I used `curl` for basic endpoint testing and `wrk` for load testing.

Example functional checks:

```bash
curl -i http://localhost:8080/
curl -i http://localhost:8080/users
curl -i -X POST http://localhost:8080/print \
  -H 'Content-Type: text/plain' \
  --data 'hello from curl'
```

For the benchmarks I used `bench/run.sh`. It runs a short warm-up, then five 10-second `wrk --latency` runs for each configuration and saves the raw output in `bench/`. During each run it also samples the response size with `curl` and records the kernel's listen-queue counters with `nstat`.

I also tested concurrent connections and investigated failures that appeared under load, including client disconnections, connection-lifetime bugs, file-descriptor limits, and the two bugs above. These tests exposed problems that did not appear in simple single-request checks.

The current test coverage is limited. More systematic tests for malformed requests, queue saturation, partial writes, client disconnects, and shutdown behavior would improve confidence in the implementation.

## Benchmark Results

I used `wrk` to examine how the server behaved under concurrent load. One example is:

```bash
wrk -t8 -c2000 -d10s --latency http://localhost:8080/
```

The following are observed runs from development, not a formal benchmark suite. Results depend on the machine, build, server configuration, `wrk` configuration, operating-system limits, and system load. They should be treated as measurements from those runs rather than as a general performance claim.

**Environment.** HP EliteBook 840 G5, Intel Core i7-8650U (8 logical CPUs), 7.6 GB RAM, Linux kernel 6.14.0-37, gcc 13.3.0 with `-Wall -Wextra -g -O2`, `wrk` 4.1.0, `ulimit -n 65535`, power profile set to `performance`. `wrk` and the server ran on the same laptop, so they shared the same CPUs. The server used 4 worker threads, a task queue of 64 entries, and an `epoll` event array of 64. The raw output and the environment details are in [`bench/`](bench/).

Each row is the median of five 10-second runs, with the range of throughput in brackets.

| Test configuration | Throughput | p50 | p99 | Max latency | Timeouts |
|---|---:|---:|---:|---:|---:|
| 100 connections, 2 `wrk` threads | 74,634 req/s (74,265 to 80,996) | 0.85 ms | 1.14 ms | 1.6 to 14.1 ms | 0 |
| 1,000 connections, 4 `wrk` threads | 71,321 req/s (71,110 to 74,488) | 0.87 ms | 1.13 ms | 9.8 to 21.6 ms | 0 |
| 2,000 connections, 8 `wrk` threads | 72,912 req/s (70,833 to 73,424) | 0.86 ms | 1.17 ms | 22.4 to 27.2 ms | 0 |

`wrk` threads are client-side load-generator threads; they are not the server's worker threads. There were no socket errors, and the listen-queue counters were 0 in all 15 runs. Each response was about 114 bytes.

Throughput stays between about 71,000 and 75,000 requests per second from 100 to 2,000 connections, and p99 stays near 1.1 to 1.2 ms. I have not profiled where that ceiling comes from. The single event loop and `wrk` sharing the same CPUs are two possibilities.

For comparison, I ran 2,000 connections with 4 workers before the input-clearing and backlog fixes. Five runs gave a median of about 36,900 requests/s (range 33,800 to 43,300), p50 of about 2.05 ms, p99 of about 5.56 ms, and 179 timeouts in total across three of the five runs. The bytes `wrk` reported per response in those runs were not the response size, because of the input bug above. So the higher number after the fixes mostly reflects the server no longer copying a growing input back to the client, not a tuning improvement.

## Design Decisions

### Separate socket I/O from request processing

The event loop owns the socket reads and writes and the lifetime management of sockets , while the worker threads perform request processing and response generation. This keeps the application processing outside the event loop, but it also require a clear way to maintain the ownership of shared data such as the connection struct.

### Track processing and closure separately

A connection may be processing a request when the client disconnects. Tracking processing state separately from a requested close helps avoid freeing the connection while a worker may still be using it. The current implementation is an improvement over the original version, but it does not yet provide a complete ownership model for every failure path.


## Current Limitations

This project is a learning implementation, not a production-ready HTTP server.

Current limitations include:

- Fixed-capacity task and completion queues.
- Incomplete handling of queue saturation and some worker errors.
- Some error paths can leave connection state requiring better recovery.
- Limited HTTP feature support and request validation.
- Pipelined requests are not supported, because the input is cleared after each response.
- A single event loop.
- No TLS support.
- Limited systematic tests for failure scenarios.
- No formal, repeatable performance comparison against established servers.
- Connection ownership and cleanup need further work.
- Shutdown behavior and recovery from unexpected failures need more testing.

In particular, queue submission and completion errors need explicit handling. For example, if a task cannot be queued or a worker fails before reporting completion, the connection may remain in a processing state. The current implementation should not be treated as robust against all such failures.

## Future Improvements

1. Handle task-queue and completion-queue saturation when they get full gracefully without loosing work.
2. Make sure worker errors cannot leave connections stuck in the processing state.
3. Make connection ownership and cleanup explicit across the event loop and workers.
4. Improve shutdown behavior and resource cleanup.
5. Expand HTTP parsing and request validation to proper standardize version.
6. Add repeatable tests for partial writes, client disconnects, malformed requests, and queue saturation.
7. Add metrics for queue depth, worker utilization, active connections, and request latency.
8. Repeat benchmarks with controlled settings, including pinning the server and `wrk` to separate CPUs.
9. Explore multiple event loops after the single-loop implementation is better tested.

## What I Learned

Working with non-blocking sockets and `epoll` taught me that readiness is not the same as completion. A readable socket does not guarantee that a full HTTP request is available, and a writable socket does not guarantee that the entire response can be sent in one call.

Separating socket I/O from request processing introduced me to the coordination problems that appear when an event loop and worker threads interact with the same connection. Stress testing made connection-lifetime bugs and operating-system resource limits visible in ways that basic functional tests did not.

The input bug only showed up when I got confused with the results getting worst with concurrent tests and divided the bytes `wrk` read by the number of requests. The throughput and latency numbers alone looked plausible, so checking what the numbers meant mattered as much as measuring them.

## Project Structure

The project separates public declarations and data structures from their C implementations:

- `include/` contains project header files.
- `src/` contains C implementation files.

The main components are:

| File | Responsibility |
|---|---|
| `main.c` | Server initialization, accepting connections, and the event loop. |
| `connection.c` | Connection management. |
| `buffer.c` | Input and output buffer operations. |
| `task_queue.c` | Shared task queue. |
| `thread_pool.c` | Worker-thread management and request processing. |
| `completion_queue.c` | Queue used to report completed worker processing to the event loop. |
| `http_parser.c` | HTTP request parsing. |
| `http_request.c` | HTTP request representation and cleanup. |
| `router.c` | Routes requests to controllers. |
| `controller.c` | Endpoint handling. |
| `response.c` | HTTP response generation. |