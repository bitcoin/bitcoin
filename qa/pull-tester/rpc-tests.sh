#!/bin/bash
set -e

CURDIR=$(cd $(dirname "$0"); pwd)
# Get BUILDDIR and REAL_BITCOIND
. "${CURDIR}/tests-config.sh"

export BITCOIND=${REAL_BITCOIND}
export BITCOINCLI=${REAL_BITCOINCLI}

if [ "x${EXEEXT}" = "x.exe" ]; then
  echo "Win tests currently disabled"
  exit 0
fi

#Run the tests

testScripts=(
    'wallet.py'
    'listtransactions.py'
    'mempool_resurrect_test.py'
    'txn_doublespend.py --mineblock'
    'txn_clone.py'
    'getchaintips.py'
    'rawtransactions.py'
    'rest.py'
    'mempool_spendcoinbase.py'
    'mempool_coinbase_spends.py'
    'httpbasics.py'
    'zapwallettxes.py'
    'proxy_test.py'
    'merkle_blocks.py'
    'fundrawtransaction.py'
    'signrawtransactions.py'
    'walletbackup.py'
    'nodehandling.py'
    'reindex.py'
    'decodescript.py'
    'p2p-fullblocktest.py'
);
testScriptsExt=(
    'bipdersig-p2p.py'
    'bipdersig.py'
    'getblocktemplate_longpoll.py'
    'getblocktemplate_proposals.py'
    'txn_doublespend.py'
    'txn_clone.py --mineblock'
    'pruning.py'
    'forknotify.py'
    'invalidateblock.py'
    'keypool.py'
    'receivedby.py'
    'rpcbind_test.py'
#   'script_test.py'
    'smartfees.py'
    'maxblocksinflight.py'
    'invalidblockrequest.py'
#    'forknotify.py'
    'p2p-acceptblock.py'
    'mempool_packages.py'
);

#if [ "x$ENABLE_ZMQ" = "x1" ]; then
#  testScripts+=('zmq_test.py')
#fi

extArg="-extended"
passOn=${@#$extArg}

if [ "x${ENABLE_BITCOIND}${ENABLE_UTILS}${ENABLE_WALLET}" = "x111" ]; then
    for (( i = 0; i < ${#testScripts[@]}; i++ ))
    do
        if [ -z "$1" ] || [ "${1:0:1}" == "-" ] || [ "$1" == "${testScripts[$i]}" ] || [ "$1.py" == "${testScripts[$i]}" ]
        then
            echo -e "Running testscript \033[1m${testScripts[$i]}...\033[0m"
            ${BUILDDIR}/qa/rpc-tests/${testScripts[$i]} --srcdir "${BUILDDIR}/src" ${passOn}
        fi
    done
    for (( i = 0; i < ${#testScriptsExt[@]}; i++ ))
    do
        if [ "$1" == $extArg ] || [ "$1" == "${testScriptsExt[$i]}" ] || [ "$1.py" == "${testScriptsExt[$i]}" ]
        then
            echo -e "Running \033[1m2nd level\033[0m testscript \033[1m${testScriptsExt[$i]}...\033[0m"
            ${BUILDDIR}/qa/rpc-tests/${testScriptsExt[$i]} --srcdir "${BUILDDIR}/src" ${passOn}
        fi
    done
else
  echo "No rpc tests to run. Wallet, utils, and bitcoind must all be enabled"
fi
