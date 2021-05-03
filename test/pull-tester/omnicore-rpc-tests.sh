#!/usr/bin/env bash

export LC_ALL=C

# Get BUILDDIR
CURDIR=$(cd $(dirname "$0") || exit; pwd)
# shellcheck source=/dev/null
. "${CURDIR}/../config.ini"

if [ -z "$BUILDDIR" ]; then
  BUILDDIR="$CURDIR";
  TESTDIR="$CURDIR/test/tmp/omnicore-rpc-tests";
else
  TESTDIR="$BUILDDIR/test/tmp/omnicore-rpc-tests"
fi

OMNICORED="$BUILDDIR/src/omnicored$EXEEXT"
OMNICORECLI="$BUILDDIR/src/omnicore-cli$EXEEXT"
DATADIR="$TESTDIR/.bitcoin"

# Start clean
rm -rf "$BUILDDIR/test/tmp"

git clone https://github.com/OmniLayer/OmniJ.git $TESTDIR
mkdir -p "$DATADIR/regtest"
touch "$DATADIR/regtest/omnicore.log"
cd $TESTDIR || exit
echo "Omni Core RPC test dir: "$TESTDIR
echo "Last OmniJ commit: "$(git log -n 1 --format="%H Author: %cn <%ce>")
if [ "$1" = "true" ]; then
    echo "Debug logging level: maximum"
    $OMNICORED -datadir="$DATADIR" -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=1 -omnidebug=all -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -limitancestorcount=750 -limitdescendantcount=750 -rpcserialversion=0 -discover=0 -listen=0 -acceptnonstdtxn=1 -deprecatedrpc=label -deprecatedrpc=labelspurpose &
else
    echo "Debug logging level: minimum"
    $OMNICORED -datadir="$DATADIR" -regtest -txindex -server -daemon -rpcuser=bitcoinrpc -rpcpassword=pass -debug=0 -omnidebug=none -omnialertallowsender=any -omniactivationallowsender=any -paytxfee=0.0001 -minrelaytxfee=0.00001 -limitancestorcount=750 -limitdescendantcount=750 -rpcserialversion=0 -discover=0 -listen=0 -acceptnonstdtxn=1 -deprecatedrpc=label -deprecatedrpc=labelspurpose &
fi
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait getblockchaininfo
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait omni_getinfo
./gradlew --console plain :omnij-rpc:regTest
STATUS=$?
$OMNICORECLI -datadir="$DATADIR" -regtest -rpcuser=bitcoinrpc -rpcpassword=pass -rpcwait stop

# If $STATUS is not 0, the test failed.
if [ $STATUS -ne 0 ]; then tail -100 $DATADIR/regtest/omnicore.log; fi


exit $STATUS
