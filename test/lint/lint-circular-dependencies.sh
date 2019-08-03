#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for circular dependencies

export LC_ALL=C

EXPECTED_CIRCULAR_DEPENDENCIES=(
    "chainparamsbase -> util/system -> chainparamsbase"
    "checkpoints -> validation -> checkpoints"
    "index/txindex -> validation -> index/txindex"
    "policy/fees -> txmempool -> policy/fees"
    "policy/policy -> validation -> policy/policy"
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel"
    "qt/bantablemodel -> qt/clientmodel -> qt/bantablemodel"
    "qt/bitcoingui -> qt/utilitydialog -> qt/bitcoingui"
    "qt/bitcoingui -> qt/walletframe -> qt/bitcoingui"
    "qt/bitcoingui -> qt/walletview -> qt/bitcoingui"
    "qt/clientmodel -> qt/peertablemodel -> qt/clientmodel"
    "qt/paymentserver -> qt/walletmodel -> qt/paymentserver"
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel"
    "qt/sendcoinsdialog -> qt/walletmodel -> qt/sendcoinsdialog"
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel"
    "qt/walletmodel -> qt/walletmodeltransaction -> qt/walletmodel"
    "txmempool -> validation -> txmempool"
    "validation -> validationinterface -> validation"
    "wallet/coincontrol -> wallet/wallet -> wallet/coincontrol"
    "wallet/fees -> wallet/wallet -> wallet/fees"
    "wallet/wallet -> wallet/walletdb -> wallet/wallet"
    "policy/fees -> policy/policy -> validation -> policy/fees"
    "policy/rbf -> txmempool -> validation -> policy/rbf"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/addressbookpage"
    "qt/guiutil -> qt/walletmodel -> qt/optionsmodel -> qt/guiutil"
    "txmempool -> validation -> validationinterface -> txmempool"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/receivecoinsdialog -> qt/addressbookpage"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/signverifymessagedialog -> qt/addressbookpage"
    "qt/guiutil -> qt/walletmodel -> qt/optionsmodel -> qt/intro -> qt/guiutil"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/sendcoinsdialog -> qt/sendcoinsentry -> qt/addressbookpage"
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
    "rpc/rawtransaction -> wallet/rpcwallet -> rpc/rawtransaction"
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
    "omnicore/errors -> omnicore/omnicore -> omnicore/mdex -> omnicore/errors"
    "omnicore/omnicore -> omnicore/tx -> omnicore/sto -> omnicore/omnicore"
)

EXIT_CODE=0

CIRCULAR_DEPENDENCIES=()

IFS=$'\n'
for CIRC in $(cd src && ../contrib/devtools/circular-dependencies.py {*,*/*,*/*/*}.{h,cpp} | sed -e 's/^Circular dependency: //'); do
    CIRCULAR_DEPENDENCIES+=($CIRC)
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
