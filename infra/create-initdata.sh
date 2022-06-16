#!/usr/bin/env bash
#
# ItCoin
#
# Starts an ephemeral local itcoin daemon, and computes the information
# necessary to perform a first initialization of a 1-of-1 signet.
# At script exit the temporary data is deleted and the itcoin daemon is killed.
#
# Diagnostic messages are printed on stderr, the output (in case of success) is
# printed on stdout.
#
# If the script succeeds, it exits with 0 and prints a minified JSON object with
# the following structure:
#     {
#       "descriptors": [
#         {
#           "desc": "tr(tprv8ZgxMBicQKsPfP82yMAwmNZaiUkRfQiZgD9qofxjyA7ief44zuyymko2sSeH2E1YPTDjF9XFyGZJkgvVJKoSqtVuY41RoUueHkPNLTfre29/86'/1'/0'/0/*)#n29weql5",
#           "timestamp": 1656074219,
#           "active": true,
#           "internal": false,
#           "range": [
#             0,
#             999
#           ],
#           "next": 0
#         },
#         {
#           "desc": "tr(tprv8ZgxMBicQKsPfP82yMAwmNZaiUkRfQiZgD9qofxjyA7ief44zuyymko2sSeH2E1YPTDjF9XFyGZJkgvVJKoSqtVuY41RoUueHkPNLTfre29/86'/1'/0'/1/*)#z7q0y40v",
#           "timestamp": 1656074220,
#           "active": true,
#           "internal": true,
#           "range": [
#             0,
#             999
#           ],
#           "next": 0
#         }
#       ],
#       "blockscript": "51209a955d04fa1e5819b3e206752793566a1cf14facfbf83b2564fa2a4a4c4daa8b"
#     }
#
# In case of error, the script exits with non-zero exit code.
#
# REQUIREMENTS:
# - jq
# - the itcoin source must already be built with /configure-itcoin.sh && make &&
#   make install
#
# USAGE:
#     create-initdata.sh
#
#     If you want to run this inside the itcoin container, do something along
#     the lines of:
#
#     docker run \
#         --rm \
#         arthub.azurecr.io/itcoin-core:git-abcdef \
#         create-initdata.sh
#
# Author: muxator <antonio.muci@bancaditalia.it>

set -eu
shopt -s inherit_errexit

errecho() {
    # prints to stderr
    >&2 echo "${@}"
}

cleanup() {
    # stop the ephemeral daemon
    "${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest stop >/dev/null
    # destroy the temporary wallet.
    errecho "create-initdata: cleaning up (deleting temporary directory ${TMPDIR})"
    rm -rf "${TMPDIR}"
}

checkPrerequisites() {
    if ! command -v mktemp &> /dev/null; then
        echo "Please install mktemp"
        exit 1
    fi
    if ! command -v jq &> /dev/null; then
        echo "Please install jq (https://stedolan.github.io/jq/)"
        exit 1
    fi
}

# Starts an ephemeral bitcoind daemon on the regtest network. Creates and opens
# a temporary wallet.
#
# Assumes that PATH_TO_BINARIES has been given a value, and that the global
# variable TMPDIR points to an empty temporary directory.
startEphemeralDaemon() {
    "${PATH_TO_BINARIES}"/bitcoind -daemon -datadir="${TMPDIR}" -regtest >/dev/null

    errecho "create-initdata: waiting (at most 10 seconds) for itcoin daemon to warmup"
    timeout 10 "${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest -rpcwait -rpcclienttimeout=3 uptime >/dev/null
    errecho "create-initdata: warmed up"

    "${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest -named createwallet wallet_name=mywallet descriptors=true >/dev/null
} # startEphemeralDaemon()

# Compute a possible signet scriptpubkey given a private descriptor, and print
# it on stdout.
#
# This functions assumes that an ephemeral bitcoin daemon has already been
# started via startEphemeralDaemon().
#
# EXAMPLE:
#     {
#       read -r SCRIPTPUBKEY
#     } <<< "$(computeScriptPubKey tr(tprvXXX/86'/1'/0'/1/*)#YYY)"
computeScriptPubKey() {
    # In order to trap errors in shell substitutions assigned to local
    # variables, we have to split the declaration and the assignment.
    #
    # https://unix.stackexchange.com/questions/23026/how-can-i-get-bash-to-exit-on-backtick-failure-in-a-similar-way-to-pipefail#146900
    local TMPADDR
    local SCRIPTPUBKEY

    TMPADDR=$("${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest deriveaddresses "$1" "[0,0]" | jq --raw-output '.[0]')
    # in our example: bcrt1pn2246p86revpnvlzqe6j0y6kdgw0znavl0urkftylg4y5nzd429s696ucu

    SCRIPTPUBKEY=$("${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest getaddressinfo "${TMPADDR}" | jq --raw-output '.scriptPubKey')
    # in our example: 51209a955d04fa1e5819b3e206752793566a1cf14facfbf83b2564fa2a4a4c4daa8b

    echo ${SCRIPTPUBKEY}
} # computeScriptPubKey()

# Automatically stop the container (wich will also self-remove at script exit
# or if an error occours
trap cleanup EXIT

# Do not run if the required packages are not installed
checkPrerequisites

# https://stackoverflow.com/questions/59895/how-to-get-the-source-directory-of-a-bash-script-from-within-the-script-itself#246128
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Depending on whether we are running on bare metal or inside a container, the
# path to the itcoin binaries will vary.
#
# Let's check if "${MYDIR}/../target/bin" exists.
# - if it exists, we are running on bare metal, and the binaries will be at
#   "${MYDIR}/../target/bin";
# - if not, we are running inside a container, and the binaries reside in the
#   same directory containing this script.
PATH_TO_BINARIES=$(realpath --quiet --canonicalize-existing "${MYDIR}/../target/bin" || echo "${MYDIR}")

TMPDIR=$(mktemp --tmpdir --directory itcoin-temp-XXXX)

startEphemeralDaemon

# Take the 2 private descriptors which start with "tr(tprv" and contain
# "/86'/1'/0'/[0 or 1]/*)".
#
# Short reference of the meaning of the path components:
#     m / purpose' / coin_type' / account' / change / address_index
#
# For a thorough description, please refer to BIP-44:
# https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki#path-levels=
#
# The regex is convoluted because it has use escaping for "/" and ")" and has to
# to nest quoting in order to include the single quotes. Source:
# https://stackoverflow.com/questions/1250079/how-to-escape-single-quotes-within-single-quoted-strings
TR_DESCRIPTORS=$("${PATH_TO_BINARIES}"/bitcoin-cli -datadir="${TMPDIR}" -regtest -named listdescriptors private=true | jq  '.descriptors | map(select(.desc | test("^tr\\(tprv.*\/86'"'"'\/1'"'"'\/0'"'"'\/[0-1]\/\\*\\)")))')

# Among the previously selected descriptors, take the one whose change field is
# 1 (the so called "internal chain"). We decide to use the internal chain for
# coinbase transaction as the miner is sending money to itself, rather than
# receiving it from third parties.
TR_CHANGE_DESCRIPTOR=$(echo ${TR_DESCRIPTORS} | jq --raw-output '.[] | select(.desc | contains("/1/*")) | .desc')
# in our example: tr(tprv8ZgxMBicQKsPfP82yMAwmNZaiUkRfQiZgD9qofxjyA7ief44zuyymko2sSeH2E1YPTDjF9XFyGZJkgvVJKoSqtVuY41RoUueHkPNLTfre29/86'/1'/0'/1/*)#z7q0y40v
{
  read -r BLOCKSCRIPT
} <<< "$(computeScriptPubKey "${TR_CHANGE_DESCRIPTOR}")"

# 4. print out the results
cat <<EOF | jq --compact-output --monochrome-output
{
    "descriptors": ${TR_DESCRIPTORS},
    "blockscript": "${BLOCKSCRIPT}"
}
EOF
