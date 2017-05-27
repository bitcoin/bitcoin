#!/bin/bash
# Intelligently run the correct version of Bitcoin
echo "Starting Bitcoin..."
echo "This may take a while..."
MACHINE_TYPE=`uname -m`
WORKING_DIR=`dirname $0`
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
  # run 64-bit bitcoin
  $WORKING_DIR/bin/64/bitcoin $@
else
  # run 32-bit bitcoin
  $WORKING_DIR/bin/32/bitcoin $@
fi
