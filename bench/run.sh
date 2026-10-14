if [ $# -ne 3 ]; then
  echo "usage: $0 <server_workers> <wrk_threads> <connections>"
  exit 1
fi
W=$1; T=$2; C=$3
URL=http://localhost:8080/
mkdir -p bench

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
    echo "server config: epoll event array 64, task queue 64 (workers set per run, see file names)"
  } > bench/environment.txt
fi

# Warm-up (not saved).
wrk -t2 -c100 -d5s "$URL" > /dev/null 2>&1
sleep 3

for run in 1 2 3 4 5; do
  EXTRA=bench/extra_${C}conn_${W}workers_${T}threads_run${run}.txt
  : > "$EXTRA"

  # Sample the response size and status while the load is running.
  (
    sleep 2
    for i in $(seq 1 15); do
      curl -s -m 2 -o /dev/null -w 'bytes=%{size_download} status=%{http_code} time=%{time_total}\n' "$URL"
      sleep 0.4
    done
  ) >> "$EXTRA" 2>&1 &
  SAMPLER=$!

  command -v nstat > /dev/null && nstat -n   # reset counters

  wrk -t$T -c$C -d10s --latency "$URL" \
    | tee bench/wrk_${C}conn_${W}workers_${T}threads_run${run}.txt

  wait $SAMPLER
  if command -v nstat > /dev/null; then
    echo "--- kernel listen-queue counters during this run (blank = no drops) ---" >> "$EXTRA"
    nstat -z TcpExtListenOverflows TcpExtListenDrops >> "$EXTRA" 2>&1
  fi
  sleep 5
done
