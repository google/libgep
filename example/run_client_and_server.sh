#!/bin/bash

# ensure 3 arguments
if [[ "$#" -lt "3" ]]; then
  echo "error: need 2 parameters ($0 <client> <server> <port>)"
  exit -1
fi


CLIENT="$1"
shift
SERVER="$1"
shift
PORT="$1"

sout=$(${CLIENT} --port ${PORT} --cnt1 44 --cnt2 33 --cnt3 22 --cnt4 11) &
cout=$(${SERVER} --port ${PORT})

# ensure the client returns the right result
expected_client_output="results: 44 33 22 11"
if [[ "$cout" == *"$expected_client_output"* ]]; then
  echo "OK"
  exit 0
else
  echo "error: cout is ${cout}"
  exit -1
fi

