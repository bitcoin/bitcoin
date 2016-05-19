############## Dash Project Commands #####################
#
# An easy to use way of testing dash core 
#
# ATTENTION: 
# **** change these each project, so that the default is correct usually ****
#

HOMEUSER="evan"
DASHLOG="mnbudget,mnpayments"
DASHDEBUG="lldb --"
DASHBINARY="./dashd"
DASHNETWORK="testnet" #mainnet, testnet, regtest
DASHDIR=".dash-develop"
#DASHBINARY="./dash-qt"

touch_logs()
{  
  touch ~/.dash/debug.log;
  for ((i=2;i<=9;i++));
  do
     touch ~/.dash$i/debug.log;
  done
}

stop_cluster()
{  
  ./dash-cli --datadir=/Users/$HOMEUSER/.dash stop
  for ((i=2;i<=9;i++));
  do
     echo "stopping dash$i "
    ./dash-cli --datadir=/Users/$HOMEUSER/.dash$i stop
  done
}

start_cluster()
{  
  for ((i=1;i<=9;i++));
  do
    echo "starting dash$i "
    ./dashd --datadir=/Users/$HOMEUSER/.dash$i --daemon
  done
}

cmd_cluster()
{  
  array=$@;
  re='^[0-9]$'
  locked="any";
  if [[ $1 =~ $re ]] ; then
    #if the first parameter is a number, we'll assume we want to execute that specific daemon
    locked=$1;
    array="${array[@]:1}";
  fi; 

  for ((i=1;i<=9;i++));
  do
    if [ $locked == $i ] ; then
      echo "dash$i $array"
      ./dash-cli --datadir=/Users/$HOMEUSER/.dash$i $array
    fi;
    if [ $locked = "any" ] ; then
      echo "dash$i "
      ./dash-cli --datadir=/Users/$HOMEUSER/.dash$i $array
    fi;
  done
}

cmd_dash()
{
  if [ "$1" = "help" ]; then
    echo "dash command center";
    echo "  example: dash set debug on"
    echo "-----------------------------------";
    echo "";
    echo "setting up environment:";
    echo "  set debug on|off (lldb mode)";
    echo "  set mode qt|daemon";
    echo "  set log options";
    echo "";
    echo "starting/stopping the client:";
    echo "  start";
    echo "  stop";
    echo "";
    echo "using cli:";
    echo "  cli [options]";
    echo "";
  fi;

  
  #---- debug mode off/on
  if [ "$1 $2 $3" = "set debug on" ]; then DASHDEBUG="lldb --"; echo "debug mode on"; return; fi;
  if [ "$1 $2 $3" = "set debug off" ]; then DASHDEBUG=""; echo "debug mode off"; return; fi;
  if [ "$1 $2" = "set debug" ]; then return; fi;

  #---- setup binaries
  if [ "$1 $2 $3" = "set mode qt" ]; then DASHBINARY="./dash-qt";echo "qt mode on"; return; fi;
  if [ "$1 $2 $3" = "set mode daemon" ]; then DASHBINARY="./dashd";echo "daemon mode on"; return; fi;
  if [ "$1 $2" = "set mode" ]; then return; fi;

  #---- generic commands
  if [ "$1" = "start" ]; then $DASHDEBUG $DASHBINARY --datadir=/Users/$HOMEUSER/$DASHDIR --debug=$DASHLOG -logthreadnames; return; fi;
  if [ "$1 $2" = "hard start" ]; then $DASHDEBUG $DASHBINARY --datadir=/Users/$HOMEUSER/$DASHDIR --debug=$DASHLOG -reindex -logthreadnames; return; fi;
  if [ "$1" = "cd" ]; then cd ~/Desktop/dash-develop; return; fi;

  #---- tail $DASHDIR/network/debug.log
  if [ "$1 $DASHNETWORK" = "tail testnet" ]; then cd ~/$DASHDIR/testnet3 && tail -f debug.log ; return; fi;
  if [ "$1 $DASHNETWORK" = "tail mainnet" ]; then cd ~/$DASHDIR/ && tail -f debug.log ; return; fi;
  if [ "$1 $DASHNETWORK" = "tail regtest" ]; then cd ~/$DASHDIR/regtest && tail -f debug.log ; return; fi;

  array=$@;
  array="${array[@]:3}";

  echo $array;
  #----- cli commands
  if [ "$1" = "cli" ]; then cd ~/Desktop/dash-develop && ./dash-cli --datadir=/Users/$HOMEUSER/$DASHDIR $array; return; fi;

  echo "unknown dash command. see 'dash help'"
}

alias dash=cmd_dash;