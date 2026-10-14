# Internals

### EpollContext and Event Dispatch

The event loop monitors different types of file descriptors, including the listening socket, client sockets, and the `eventfd` used to notify the event loop when workers finish processing requests.

Each descriptor needs to be associated with enough information for the event loop to identify it and dispatch the event to the correct handler.

For this purpose, the server uses an `EpollContext` structure:

```c
typedef struct {
    EpollType type;
    int fd;
    Connection *connection;
} EpollContext;
```

The `type` field identifies the kind of descriptor, `fd` stores its file descriptor, and `connection` points to the associated client connection when one exists.

The `Connection` structure also stores a pointer to its associated `EpollContext`:

```c
EpollContext *context;
```

A forward declaration allows `EpollContext` to refer to `Connection` before the complete `Connection` structure is defined:

```c
typedef struct Connection Connection;
```

Both structures refer to each other through pointers rather than containing each other by value. This allows the structures to be declared without requiring the complete definition of `Connection` before declaring `Connection *`.

#### Registering and Dispatching Events

When a client connects, the server creates a `Connection` and an `EpollContext`. The context records the client's file descriptor, identifies it as `EPOLL_CONNECTION`, and points to the connection. The connection stores a pointer back to its context.

The context is then associated with the epoll registration:

```c
client_event.data.ptr = client_context;
```

When `epoll_wait()` returns an event, the event loop retrieves the context:

```c
EpollContext *context = events[i].data.ptr;
```

It uses the context's type to select the appropriate handler:

```c
switch (context->type) {
    case EPOLL_LISTENER:
        // Accept incoming connections.
        break;

    case EPOLL_EVENTFD:
        // Process worker completion notifications.
        break;

    case EPOLL_CONNECTION:
        // Handle client socket I/O.
        break;
}
```

The listener and `eventfd` also have their own `EpollContext` objects, but their `connection` pointers are `NULL` because they do not represent client connections.

#### Accessing the Context After Worker Completion

When a worker finishes processing a request, it places the associated `Connection *` in the completion queue and signals `eventfd`.

The event loop wakes up, drains the completion queue, and retrieves each completed connection. It can then access the associated context through:

```c
conn->context
```

The event loop uses this pointer when updating the existing epoll registration to include `EPOLLOUT`, allowing the response to be sent when the client socket becomes writable.

The completion queue carries the connection pointers, while `eventfd` provides the notification. `EpollContext` maintains the metadata needed to identify and dispatch subsequent epoll events.

This design separates client connection state from event-dispatch metadata while keeping the two structures associated through pointers.