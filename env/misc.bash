


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