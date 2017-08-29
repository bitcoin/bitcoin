#!/bin/sh
set -o xtrace   # Write all commands first to stderr
set -o errexit  # Exit the script with error if any of the commands fail

DIR=$(dirname $0)
# Functions to fetch MongoDB binaries
. $DIR/download-mongodb.sh

get_distro
get_mongodb_download_url_for "$DISTRO" "$MONGODB_VERSION"
download_and_extract "$MONGODB_DOWNLOAD_URL" "$EXTRACT"


OS=$(uname -s | tr '[:upper:]' '[:lower:]')

AUTH=${AUTH:-noauth}
SSL=${SSL:-nossl}
TOPOLOGY=${TOPOLOGY:-server}

case "$OS" in
   cygwin*)
      export MONGO_ORCHESTRATION_HOME="c:/data/MO"
      ;;
   *)
      export MONGO_ORCHESTRATION_HOME=$(pwd)"/MO"
      ;;
esac
rm -rf $MONGO_ORCHESTRATION_HOME
mkdir -p $MONGO_ORCHESTRATION_HOME/lib
mkdir -p $MONGO_ORCHESTRATION_HOME/db

if [ -z $ORCHESTRATION_FILE ]; then
   ORCHESTRATION_FILE="basic"
fi
if [ "$AUTH" = "auth" ]; then
  ORCHESTRATION_FILE="auth"
  MONGO_SHELL_CONNECTION_FLAGS="-ubob -ppwd123"
fi

if [ "$IPV4_ONLY" = "on" ]; then
  ORCHESTRATION_FILE="${ORCHESTRATION_FILE}-ipv4-only"
fi

if [ "$SSL" != "nossl" ]; then
   cp -f tests/x509gen/* $MONGO_ORCHESTRATION_HOME/lib/
   # find print0 and xargs -0 not available on Solaris. Lets hope for good paths
   find orchestration_configs -name \*.json | xargs perl -p -i -e "s|/tmp/orchestration-home|$MONGO_ORCHESTRATION_HOME/lib|g"
   ORCHESTRATION_FILE="${ORCHESTRATION_FILE}-ssl"
fi

export ORCHESTRATION_FILE="orchestration_configs/${TOPOLOGY}s/${ORCHESTRATION_FILE}.json"
export ORCHESTRATION_URL="http://localhost:8889/v1/${TOPOLOGY}s"

export TMPDIR=$MONGO_ORCHESTRATION_HOME/db
echo From shell `date` > $MONGO_ORCHESTRATION_HOME/server.log


case "$OS" in
   cygwin*)
      # Python has problems with unix style paths in cygwin. Must use c:\\ paths
      rm -rf /cygdrive/c/mongodb
      cp -r mongodb /cygdrive/c/mongodb
      echo "{ \"releases\": { \"default\": \"c:\\\\mongodb\\\\bin\" }}" > orchestration.config

      # Make sure MO is running latest version
      python.exe -m virtualenv venv
      cd venv
      . Scripts/activate
      git clone https://github.com/10gen/mongo-orchestration.git
      cd mongo-orchestration
      pip install .
      cd ../..
      ls `pwd`/mongodb/bin/mongo* || true
      nohup mongo-orchestration -f orchestration.config -e default --socket-timeout-ms=60000 --bind=127.0.0.1  --enable-majority-read-concern -s wsgiref start > $MONGO_ORCHESTRATION_HOME/out.log 2> $MONGO_ORCHESTRATION_HOME/err.log < /dev/null &
      if [ "$SSL" != "nossl" ]; then
         export MONGO_SHELL_CONNECTION_FLAGS="$MONGO_SHELL_CONNECTION_FLAGS --host localhost --ssl --sslCAFile=$MONGO_ORCHESTRATION_HOME/lib/ca.pem --sslPEMKeyFile=$MONGO_ORCHESTRATION_HOME/lib/client.pem"
      fi
      ;;
   sunos*)
      # Can't make a virtual env here
      echo "{ \"releases\": { \"default\": \"`pwd`/mongodb/bin\" } }" > orchestration.config
      nohup mongo-orchestration -f orchestration.config -e default --socket-timeout-ms=60000 --bind=127.0.0.1  --enable-majority-read-concern start > $MONGO_ORCHESTRATION_HOME/out.log 2> $MONGO_ORCHESTRATION_HOME/err.log < /dev/null &
      if [ "$SSL" != "nossl" ]; then
         export MONGO_SHELL_CONNECTION_FLAGS="$MONGO_SHELL_CONNECTION_FLAGS --host localhost --ssl --sslCAFile=$MONGO_ORCHESTRATION_HOME/lib/ca.pem --sslPEMKeyFile=$MONGO_ORCHESTRATION_HOME/lib/client.pem"
      fi
      ;;
   *)
      echo "{ \"releases\": { \"default\": \"`pwd`/mongodb/bin\" } }" > orchestration.config
      # Make sure MO is running latest version
      python -m virtualenv venv
      cd venv
      . bin/activate
      git clone https://github.com/10gen/mongo-orchestration.git
      cd mongo-orchestration
      # Our zSeries machines are static-provisioned, cache corruptions persist.
      if [ $(uname -m) = "s390x" ]; then
         echo "Disabling pip cache"
         PIP_PARAM="--no-cache-dir"
      fi
      pip $PIP_PARAM install .
      cd ../..
      nohup mongo-orchestration -f orchestration.config -e default --socket-timeout-ms=60000 --bind=127.0.0.1  --enable-majority-read-concern start > $MONGO_ORCHESTRATION_HOME/out.log 2> $MONGO_ORCHESTRATION_HOME/err.log < /dev/null &
      if [ "$SSL" != "nossl" ]; then
         export MONGO_SHELL_CONNECTION_FLAGS="$MONGO_SHELL_CONNECTION_FLAGS --host localhost --ssl --sslCAFile=$MONGO_ORCHESTRATION_HOME/lib/ca.pem --sslPEMKeyFile=$MONGO_ORCHESTRATION_HOME/lib/client.pem"
      fi
      ;;
esac

sleep 15
curl http://localhost:8889/ --silent --max-time 120 --fail

sleep 5

pwd
curl --silent --data @"$ORCHESTRATION_FILE" "$ORCHESTRATION_URL" --max-time 300 --fail

sleep 15

`pwd`/mongodb/bin/mongo $MONGO_SHELL_CONNECTION_FLAGS --eval 'printjson(db.serverBuildInfo())' admin
`pwd`/mongodb/bin/mongo $MONGO_SHELL_CONNECTION_FLAGS --eval 'printjson(db.isMaster())' admin
