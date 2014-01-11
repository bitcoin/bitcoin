#!/bin/bash
TIMEOUT=10
SIGNAL=HUP
if [ $# -eq 0 ]; then
  echo -e "Usage:\t$0 <cmd>"
  echo -e "\tRuns <cmd> and wait ${TIMEOUT} seconds or until SIG${SIGNAL} is received."
  echo -e "\tReturns: 0 if SIG${SIGNAL} is received, 1 otherwise."
  exit 0
fi
trap '[[ ${PID} ]] && kill ${PID}' ${SIGNAL}
"$@"
sleep ${TIMEOUT} & PID=$!
wait ${PID} && exit 1
exit 0
