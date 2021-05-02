#!/usr/bin/env bash
#
# Copyright (c) 2018-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for circular dependencies

export LC_ALL=C

EXPECTED_CIRCULAR_DEPENDENCIES=(
    "chainparamsbase -> util/system -> chainparamsbase"
    "index/txindex -> validation -> index/txindex"
    "policy/fees -> txmempool -> policy/fees"
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel"
    "qt/bitcoingui -> qt/walletframe -> qt/bitcoingui"
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel"
    "qt/sendcoinsdialog -> qt/walletmodel -> qt/sendcoinsdialog"
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel"
    "txmempool -> validation -> txmempool"
    "wallet/fees -> wallet/wallet -> wallet/fees"
    "wallet/wallet -> wallet/walletdb -> wallet/wallet"
    "policy/fees -> txmempool -> validation -> policy/fees"
    "omnicore/dbspinfo -> omnicore/omnicore -> omnicore/dbspinfo"
    "omnicore/dbtradelist -> omnicore/mdex -> omnicore/dbtradelist"
    "omnicore/dbtxlist -> omnicore/dex -> omnicore/dbtxlist"
    "omnicore/dbtxlist -> omnicore/omnicore -> omnicore/dbtxlist"
    "omnicore/dbtxlist -> omnicore/tx -> omnicore/dbtxlist"
    "omnicore/dex -> omnicore/omnicore -> omnicore/dex"
    "omnicore/dex -> omnicore/tx -> omnicore/dex"
    "omnicore/mdex -> omnicore/tx -> omnicore/mdex"
    "omnicore/omnicore -> omnicore/rules -> omnicore/omnicore"
    "omnicore/omnicore -> omnicore/sp -> omnicore/omnicore"
    "omnicore/omnicore -> omnicore/tally -> omnicore/omnicore"
    "omnicore/omnicore -> omnicore/tx -> omnicore/omnicore"
    "omnicore/omnicore -> omnicore/walletcache -> omnicore/omnicore"
    "omnicore/omnicore -> omnicore/walletutils -> omnicore/omnicore"
    "omnicore/consensushash -> omnicore/dbspinfo -> omnicore/omnicore -> omnicore/consensushash"
    "omnicore/consensushash -> omnicore/dex -> omnicore/rules -> omnicore/consensushash"
    "omnicore/dbfees -> omnicore/sto -> omnicore/omnicore -> omnicore/dbfees"
    "omnicore/dbspinfo -> omnicore/omnicore -> omnicore/sp -> omnicore/dbspinfo"
    "omnicore/dbspinfo -> omnicore/omnicore -> omnicore/tx -> omnicore/dbspinfo"
    "omnicore/dbtradelist -> omnicore/mdex -> omnicore/tx -> omnicore/dbtradelist"
    "omnicore/dbtransaction -> omnicore/errors -> omnicore/omnicore -> omnicore/dbtransaction"
    "omnicore/dbtxlist -> omnicore/omnicore -> omnicore/mdex -> omnicore/dbtxlist"
    "omnicore/dbtxlist -> omnicore/dex -> omnicore/rules -> omnicore/dbtxlist"
    "omnicore/dex -> omnicore/omnicore -> omnicore/persistence -> omnicore/dex"
    "omnicore/omnicore -> omnicore/tx -> omnicore/sto -> omnicore/omnicore"
    "init -> txmempool -> omnicore/omnicore -> init"
    "omnicore/omnicore -> omnicore/pending -> txmempool -> omnicore/omnicore"
    "txdb -> validation -> txdb"
    "omnicore/nftdb -> omnicore/omnicore -> omnicore/tx -> omnicore/nftdb"
)

EXIT_CODE=0

CIRCULAR_DEPENDENCIES=()

IFS=$'\n'
for CIRC in $(cd src && ../contrib/devtools/circular-dependencies.py {*,*/*,*/*/*}.{h,cpp} | sed -e 's/^Circular dependency: //'); do
    CIRCULAR_DEPENDENCIES+=( "$CIRC" )
    IS_EXPECTED_CIRC=0
    for EXPECTED_CIRC in "${EXPECTED_CIRCULAR_DEPENDENCIES[@]}"; do
        if [[ "${CIRC}" == "${EXPECTED_CIRC}" ]]; then
            IS_EXPECTED_CIRC=1
            break
        fi
    done
    if [[ ${IS_EXPECTED_CIRC} == 0 ]]; then
        echo "A new circular dependency in the form of \"${CIRC}\" appears to have been introduced."
        echo
        EXIT_CODE=1
    fi
done

for EXPECTED_CIRC in "${EXPECTED_CIRCULAR_DEPENDENCIES[@]}"; do
    IS_PRESENT_EXPECTED_CIRC=0
    for CIRC in "${CIRCULAR_DEPENDENCIES[@]}"; do
        if [[ "${CIRC}" == "${EXPECTED_CIRC}" ]]; then
            IS_PRESENT_EXPECTED_CIRC=1
            break
        fi
    done
    if [[ ${IS_PRESENT_EXPECTED_CIRC} == 0 ]]; then
        echo "Good job! The circular dependency \"${EXPECTED_CIRC}\" is no longer present."
        echo "Please remove it from EXPECTED_CIRCULAR_DEPENDENCIES in $0"
        echo "to make sure this circular dependency is not accidentally reintroduced."
        echo
        EXIT_CODE=1
    fi
done

exit ${EXIT_CODE}
