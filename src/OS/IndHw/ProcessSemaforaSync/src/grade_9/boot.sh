#!/bin/bash

pids=()

export PATH_TO_IPC_DIR="ipc"

for _ in {1..5}; do
  ./bin/process_sync_9 &
  pids+=($!)
done

cleanup() {
  kill -SIGINT "${pids[@]}" 2>/dev/null
  sleep 1
  kill -9 "${pids[@]}" 2>/dev/null
  wait
}

trap cleanup SIGINT SIGTERM

wait