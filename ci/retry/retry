#!/usr/bin/env bash

GETOPT_BIN=$IN_GETOPT_BIN
GETOPT_BIN=${GETOPT_BIN:-getopt}

__sleep_amount() {
  if [ -n "$constant_sleep" ]; then
    sleep_time=$constant_sleep
  else
    #TODO: check for awk
    #TODO: check if user would rather use one of the other possible dependencies: python, ruby, bc, dc
    sleep_time=`awk "BEGIN {t = $min_sleep * $(( (1<<($attempts -1)) )); print (t > $max_sleep ? $max_sleep : t)}"`
  fi
}

__log_out() {
  echo "$1" 1>&2
}

# Paramters: max_tries min_sleep max_sleep constant_sleep fail_script EXECUTION_COMMAND
retry()
{
  local max_tries="$1"; shift
  local min_sleep="$1"; shift
  local max_sleep="$1"; shift
  local constant_sleep="$1"; shift
  local fail_script="$1"; shift
  if [ -n "$VERBOSE" ]; then
    __log_out "Retry Parameters: max_tries=$max_tries min_sleep=$min_sleep max_sleep=$max_sleep constant_sleep=$constant_sleep"
    if [ -n "$fail_script" ]; then __log_out "Fail script: $fail_script"; fi
    __log_out ""
    __log_out "Execution Command: $*"
    __log_out ""
  fi

  local attempts=0
  local return_code=1


  while [[ $return_code -ne 0 && $attempts -le $max_tries ]]; do
    if [ $attempts -gt 0 ]; then
      __sleep_amount
      __log_out "Before retry #$attempts: sleeping $sleep_time seconds"
      sleep $sleep_time
    fi

    P="$1"
    for param in "${@:2}"; do P="$P '$param'"; done
    #TODO: replace single quotes in each arg with '"'"' ?
    export RETRY_ATTEMPT=$attempts
    bash -c "$P"
    return_code=$?
    #__log_out "Process returned $return_code on attempt $attempts"
    if [ $return_code -eq 127 ]; then
      # command not found
      exit $return_code
    elif [ $return_code -ne 0 ]; then
      attempts=$[$attempts +1]
    fi
  done

  if [ $attempts -gt $max_tries ]; then
    if [ -n "$fail_script" ]; then
      __log_out "Retries exhausted, running fail script"
      eval $fail_script
    else
      __log_out "Retries exhausted"
    fi
  fi

  exit $return_code
}

# If we're being sourced, don't worry about such things
if [ "$BASH_SOURCE" == "$0" ]; then
  # Prints the help text
  help()
  {
    local retry=$(basename $0)
    cat <<EOF
Usage: $retry [options] -- execute command
    -h, -?, --help
    -v, --verbose                    Verbose output
    -t, --tries=#                    Set max retries: Default 10
    -s, --sleep=secs                 Constant sleep amount (seconds)
    -m, --min=secs                   Exponenetial Backoff: minimum sleep amount (seconds): Default 0.3
    -x, --max=secs                   Exponenetial Backoff: maximum sleep amount (seconds): Default 60
    -f, --fail="script +cmds"        Fail Script: run in case of final failure
EOF
  }

  # show help for no arguments if stdin is a terminal
  if { [ -z "$1" ] && [ -t 0 ] ; } || [ "$1" == '-h' ] || [ "$1" == '-?' ] || [ "$1" == '--help' ]
  then
    help
    exit 0
  fi

  $GETOPT_BIN --test > /dev/null
  if [[ $? -ne 4 ]]; then
      echo "I’m sorry, 'getopt --test' failed in this environment. Please load GNU getopt."
      exit 1
  fi

  OPTIONS=vt:s:m:x:f:
  LONGOPTIONS=verbose,tries:,sleep:,min:,max:,fail:

  PARSED=$($GETOPT_BIN --options="$OPTIONS" --longoptions="$LONGOPTIONS" --name "$0" -- "$@")
  if [[ $? -ne 0 ]]; then
    # e.g. $? == 1
    #  then getopt has complained about wrong arguments to stdout
    exit 2
  fi
  # read getopt’s output this way to handle the quoting right:
  eval set -- "$PARSED"

  max_tries=10
  min_sleep=0.3
  max_sleep=60.0
  constant_sleep=
  fail_script=

  # now enjoy the options in order and nicely split until we see --
  while true; do
      case "$1" in
          -v|--verbose)
              VERBOSE=true
              shift
              ;;
          -t|--tries)
              max_tries="$2"
              shift 2
              ;;
          -s|--sleep)
              constant_sleep="$2"
              shift 2
              ;;
          -m|--min)
              min_sleep="$2"
              shift 2
              ;;
          -x|--max)
              max_sleep="$2"
              shift 2
              ;;
          -f|--fail)
              fail_script="$2"
              shift 2
              ;;
          --)
              shift
              break
              ;;
          *)
              echo "Programming error"
              exit 3
              ;;
      esac
  done

  retry "$max_tries" "$min_sleep" "$max_sleep" "$constant_sleep" "$fail_script" "$@"

fi
