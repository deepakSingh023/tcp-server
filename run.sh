
if [ $# -ne 4 ]; then
    echo "usage: $0 <server_workers> <wrk_threads> <connections> <ECHO|GET|POST>"
    exit 1
fi

W=$1
T=$2
C=$3
METHOD=$(echo "$4" | tr '[:lower:]' '[:upper:]')

if [[ "$METHOD" != "ECHO" && "$METHOD" != "GET" && "$METHOD" != "POST" ]]; then
    echo "Method must be ECHO, GET or POST"
    exit 1
fi

URL=http://localhost:8080
mkdir -p bench

# Unique directory for every invocation.
DIR="bench/${METHOD}_${C}conn_${W}workers_${T}threads_$(date +%Y%m%d_%H%M%S)"
mkdir "$DIR"

# Machine and build details, written once.
if [ ! -f bench/environment.txt ]; then
    {
        echo "date: $(date -I)"
        echo "kernel: $(uname -r)"
        lscpu | grep -E 'Model name|^CPU\(s\)'
        free -h | sed -n 1,2p
        gcc --version | head -1
        wrk --version 2>&1 | head -1
        echo "ulimit -n: $(ulimit -n)"
        grep '^CFLAGS' Makefile
        echo "power profile: $(powerprofilesctl get 2>/dev/null || echo unknown)"
        echo "somaxconn: $(cat /proc/sys/net/core/somaxconn)"
        echo "server config: epoll event array 64, task queue 64"
    } > bench/environment.txt
fi

# Select endpoint.
case "$METHOD" in
    ECHO) ENDPOINT="/" ;;
    GET)  ENDPOINT="/users" ;;
    POST) ENDPOINT="/print" ;;
esac

# Warm-up.
METHOD="$METHOD" wrk -t2 -c100 -d5s -s request.lua "$URL" > /dev/null 2>&1
sleep 3

for run in 1 2 3 4 5; do

    EXTRA="$DIR/extra_run${run}.txt"

    # Sample response size, status and latency during the test.
    (
        sleep 2
        for i in $(seq 1 15); do

            if [ "$METHOD" = "POST" ]; then
                curl -s -m 2 -X POST \
                    -H "Content-Type: text/plain" \
                    -d "Hello World" \
                    -o /dev/null \
                    -w 'bytes=%{size_download} status=%{http_code} time=%{time_total}\n' \
                    ""$URL$ENDPOINT"" || true
            else
                curl -s -m 2 -o /dev/null \
                    -w 'bytes=%{size_download} status=%{http_code} time=%{time_total}\n' \
                    ""$URL$ENDPOINT"" || true
            fi

            sleep 0.4
        done
    ) >> "$EXTRA" 2>&1 &

    SAMPLER=$!

    command -v nstat > /dev/null && nstat -n

    # Actual benchmark.
    METHOD="$METHOD" wrk -t"$T" -c"$C" -d10s --latency \
        -s request.lua "$URL" \
        | tee "$DIR/wrk_run${run}.txt"

    wait "$SAMPLER" || true

    if command -v nstat > /dev/null; then
        echo "--- kernel listen-queue counters during this run ---" >> "$EXTRA"
        nstat -z TcpExtListenOverflows TcpExtListenDrops >> "$EXTRA" 2>&1
    fi

    sleep 5
done

echo ""
echo "Benchmark complete!"
echo "Method: $METHOD"
echo "Results: $DIR"