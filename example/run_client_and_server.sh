#!/bin/bash

PORT=9009

sout=$(./server --port ${PORT} --cnt1 44 --cnt2 33 --cnt3 22 --cnt4 11) &
cout=$(./client --port ${PORT})

# ensure the client returns the right result
expected_client_output="results: 44 33 22 11"
if [[ "$cout" == *"$expected_client_output"* ]]; then
  echo "OK"
  exit 0
else
  echo "error: cout is ${cout}"
  exit -1
fi

