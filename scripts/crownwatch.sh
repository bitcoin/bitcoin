#!/bin/bash
# Copyright (c) 2018-2019 The Crown developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
# 
# crownwatch.sh
# A watchdog program to alleviate problems caused by increased memory
# usage of the Crown daemon. 
#
# It does 2 things:
#  1. Check if crownd is running and start it if not
#  2. If started with "mem" option, check how much memory is free and 
#     pre-emptively restart the daemon if it is judged too low. The 
#     default amount is rather arbitrary and may not be appropriate 
#     for everyone. That's why it is easily configurable a few lines 
#     down from here where it says    MINFREE=524288
#
# These are referred to as watchdog levels 1 and 2 by the 
# crown-server-install.sh installation/update script.
#
# It is expected to be installed as /usr/local/bin/crownwatch.sh and 
# executed by whichever userid runs the daemon on your system, by cron 
# every 15 minutes. If you installed it manually, or want to change the
# watchdog level, add either one of the following (uncommented) lines
# to make it so:
#
# */15 * * * * /usr/local/bin/crownwatch.sh 
# */15 * * * * /usr/local/bin/crownwatch.sh mem 
#

# Customise these to suit your environment
MINFREE=524288			# safe minimum free memory
PREFIX="/usr/local/bin"		# path to Crown executables
DATADIR=~/.crown		# Crown datadir

# Announce ourselves
echo "Crown watchdog script running at" `date`

# Start off by looking for running daemon
PID=$(pidof crownd)

# Start it if it's not running
if [[ $? -eq 1 ]]; then
  echo "crownd not running. Removing any old flags and starting it."
  rm -f "${DATADIR}/crownd.pid" "${DATADIR}/.lock"
  ${PREFIX}/crownd -daemon

# Daemon is running, check free memory if requested by the user
elif [[ $1 == "mem" ]]; then
  echo "crownd running with PID=${PID}. Checking free memory."
  TMP=$("mktemp")
  free > ${TMP}
  FREEMEM=$(awk '$1 ~ /Mem|Swap/ {sum += $4} END {print sum}' ${TMP})
  rm ${TMP}

# If free memory is getting low, pre-emptively stop the daemon
  if [[ ${FREEMEM} -lt ${MINFREE} ]]; then
    echo "Total free memory is less than minimum. Shutting down crownd."
    ${PREFIX}/crown-cli stop

# Allow up to 10 minutes for it to shutdown gracefully
    for ((i=0; i<10; i++)); do
      echo "...waiting..."
      sleep 60
      if  [[ $(ps -p ${PID} | wc -l) -lt 2 ]]; then
        break
      fi
    done

# If it still hasn't shutdown, terminate with extreme prejudice
    if [[ ${i} -eq 10 ]]; then
      echo "Shutdown still incomplete, killing the daemon."
      kill -9 ${PID}
      sleep 10
      rm -f "${DATADIR}/crownd.pid" "${DATADIR}/.lock"
    fi

# Restart it if we stopped it
    echo "Starting crownd."
    ${PREFIX}/crownd -daemon

# Nothing to do if there was enough free memory
  else
    echo "Total free memory is above safe minimum, doing nothing."
  fi
fi
