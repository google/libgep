#!/bin/bash

# ensure 2 arguments
if [[ "$#" -lt "2" ]]; then
  echo "error: need 1 parameters ($0 <client> <server>)"
  exit -1
fi


CLIENT="$1"
shift
SERVER="$1"

# use a random name for a fifo to communicate client and server
tmpfifo=$(mktemp -u /tmp/tmp.fifo.XXXXXXXX)
mkfifo "${tmpfifo}"

sout=$(${SERVER} --fifo ${tmpfifo} --cnt1 44 --cnt2 33 --cnt3 22 --cnt4 11) &
cout=$(${CLIENT} --fifo ${tmpfifo})

rm "${tmpfifo}"

# ensure the client returns the right result
expected_client_output="results: 44 33 22 11"
if [[ "$cout" == *"$expected_client_output"* ]]; then
  echo "OK"
  exit 0
else
  echo "error: cout is ${cout}"
  exit -1
fi

