#!/bin/bash
# Copyright (c) 2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
TIMEOUT=10
SIGNAL=HUP
PIDFILE=.send.pid
if [ $# -eq 0 ]; then
  echo -e "Usage:\t$0 <cmd>"
  echo -e "\tRuns <cmd> and wait ${TIMEOUT} seconds or until SIG${SIGNAL} is received."
  echo -e "\tReturns: 0 if SIG${SIGNAL} is received, 1 otherwise."
  echo -e "Or:\t$0 -STOP"
  echo -e "\tsends SIG${SIGNAL} to running send.sh"
  exit 0
fi

if [ $1 = "-STOP" ]; then
  if [ -s ${PIDFILE} ]; then
      kill -s ${SIGNAL} $(<$PIDFILE 2>/dev/null) 2>/dev/null
  fi
  exit 0
fi

trap '[[ ${PID} ]] && kill ${PID}' ${SIGNAL}
trap 'rm -f ${PIDFILE}' EXIT
echo $$ > ${PIDFILE}
"$@"
sleep ${TIMEOUT} & PID=$!
wait ${PID} && exit 1

exit 0
