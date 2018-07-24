#!/usr/bin/env bash
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for circular dependencies

export LC_ALL=C

EXPECTED_CIRCULAR_DEPENDENCIES=(
    "chainparamsbase -> util -> chainparamsbase"
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
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel"
    "qt/walletmodel -> qt/walletmodeltransaction -> qt/walletmodel"
    "rpc/rawtransaction -> wallet/rpcwallet -> rpc/rawtransaction"
    "txmempool -> validation -> txmempool"
    "validation -> validationinterface -> validation"
    "wallet/fees -> wallet/wallet -> wallet/fees"
    "wallet/wallet -> wallet/walletdb -> wallet/wallet"
    "wallet/coincontrol -> wallet/wallet -> wallet/coincontrol"
    "policy/fees -> policy/policy -> validation -> policy/fees"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/addressbookpage"
    "txmempool -> validation -> validationinterface -> txmempool"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/receivecoinsdialog -> qt/addressbookpage"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/signverifymessagedialog -> qt/addressbookpage"
    "qt/addressbookpage -> qt/bitcoingui -> qt/walletview -> qt/sendcoinsdialog -> qt/sendcoinsentry -> qt/addressbookpage"
    # Dash
    "coinjoin/coinjoin-server -> init -> coinjoin/coinjoin-server"
    "coinjoin/coinjoin-server -> net_processing -> coinjoin/coinjoin-server"
    "coinjoin/coinjoin-client -> init -> dsnotificationinterface -> coinjoin/coinjoin-client"
    "coinjoin/coinjoin-client -> init -> masternode/masternode-utils -> coinjoin/coinjoin-client"
    "evo/cbtx -> evo/simplifiedmns -> evo/cbtx"
    "evo/cbtx -> evo/specialtx -> evo/cbtx"
    "evo/deterministicmns -> evo/providertx -> evo/deterministicmns"
    "evo/deterministicmns -> evo/simplifiedmns -> evo/deterministicmns"
    "evo/deterministicmns -> evo/specialtx -> evo/deterministicmns"
    "evo/deterministicmns -> llmq/quorums_commitment -> evo/deterministicmns"
    "evo/deterministicmns -> llmq/quorums_utils -> evo/deterministicmns"
    "evo/deterministicmns -> validation -> evo/deterministicmns"
    "evo/mnauth -> net_processing -> evo/mnauth"
    "evo/specialtx -> llmq/quorums_blockprocessor -> evo/specialtx"
    "evo/specialtx -> llmq/quorums_commitment -> evo/specialtx"
    "evo/specialtx -> validation -> evo/specialtx"
    "governance/governance -> governance/governance-classes -> governance/governance"
    "governance/governance -> governance/governance-object -> governance/governance"
    "governance/governance -> init -> governance/governance"
    "governance/governance -> masternode/masternode-sync -> governance/governance"
    "governance/governance -> net_processing -> governance/governance"
    "governance/governance-object -> governance/governance-validators -> governance/governance-object"
    "init -> masternode/masternode-sync -> init"
    "init -> masternode/masternode-utils -> init"
    "init -> net_processing -> init"
    "init -> netfulfilledman -> init"
    "init -> rpc/server -> init"
    "init -> txdb -> init"
    "init -> validation -> init"
    "init -> validationinterface -> init"
    "llmq/quorums -> llmq/quorums_utils -> llmq/quorums"
    "llmq/quorums_blockprocessor -> net_processing -> llmq/quorums_blockprocessor"
    "llmq/quorums_chainlocks -> llmq/quorums_instantsend -> llmq/quorums_chainlocks"
    "llmq/quorums_chainlocks -> net_processing -> llmq/quorums_chainlocks"
    "llmq/quorums_dkgsessionmgr -> net_processing -> llmq/quorums_dkgsessionmgr"
    "llmq/quorums_instantsend -> net_processing -> llmq/quorums_instantsend"
    "llmq/quorums_instantsend -> txmempool -> llmq/quorums_instantsend"
    "llmq/quorums_instantsend -> validation -> llmq/quorums_instantsend"
    "llmq/quorums_signing -> llmq/quorums_signing_shares -> llmq/quorums_signing"
    "llmq/quorums_signing -> net_processing -> llmq/quorums_signing"
    "llmq/quorums_signing_shares -> net_processing -> llmq/quorums_signing_shares"
    "logging -> util -> logging"
    "masternode/masternode-payments -> validation -> masternode/masternode-payments"
    "net -> netmessagemaker -> net"
    "net_processing -> spork -> net_processing"
    "netaddress -> netbase -> netaddress"
    "qt/appearancewidget -> qt/guiutil -> qt/appearancewidget"
    "qt/bitcoinaddressvalidator -> qt/guiutil -> qt/bitcoinaddressvalidator"
    "qt/bitcoingui -> qt/guiutil -> qt/bitcoingui"
    "qt/guiutil -> qt/optionsdialog -> qt/guiutil"
    "qt/guiutil -> qt/qvalidatedlineedit -> qt/guiutil"
    "core_io -> evo/cbtx -> evo/deterministicmns -> core_io"
    "core_io -> evo/cbtx -> evo/simplifiedmns -> core_io"
    "dsnotificationinterface -> governance/governance -> init -> dsnotificationinterface"
    "evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> evo/simplifiedmns"
    "httprpc -> httpserver -> init -> httprpc"
    "httpserver -> init -> httpserver"
    "init -> llmq/quorums_init -> llmq/quorums_signing_shares -> init"
    "llmq/quorums_dkgsession -> llmq/quorums_dkgsessionmgr -> llmq/quorums_dkgsessionhandler -> llmq/quorums_dkgsession"
    "logging -> util -> random -> logging"
    "logging -> util -> sync -> logging"
    "logging -> util -> stacktraces -> logging"
    "coinjoin/coinjoin-client -> coinjoin/coinjoin-util -> wallet/wallet -> coinjoin/coinjoin-client"
    "qt/appearancewidget -> qt/guiutil -> qt/optionsdialog -> qt/appearancewidget"
    "qt/bitcoinaddressvalidator -> qt/guiutil -> qt/qvalidatedlineedit -> qt/bitcoinaddressvalidator"
    "qt/guiutil -> qt/optionsdialog -> qt/optionsmodel -> qt/guiutil"
    "bloom -> evo/cbtx -> evo/simplifiedmns -> merkleblock -> bloom"
    "bloom -> evo/cbtx -> llmq/quorums_blockprocessor -> net -> bloom"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> llmq/quorums_debug -> evo/deterministicmns"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> evo/deterministicmns"
    "evo/specialtx -> llmq/quorums_blockprocessor -> net_processing -> txmempool -> evo/specialtx"
    "evo/providertx -> evo/specialtx -> llmq/quorums_blockprocessor -> net_processing -> txmempool -> evo/providertx"

    "coinjoin/coinjoin-client -> net_processing -> coinjoin/coinjoin-client"
    "llmq/quorums -> net_processing -> llmq/quorums"
    "llmq/quorums_commitment -> llmq/quorums_utils -> llmq/quorums_commitment"
    "llmq/quorums_dkgsession -> llmq/quorums_dkgsessionmgr -> llmq/quorums_dkgsession"
    "coinjoin/coinjoin -> masternode/activemasternode -> net -> coinjoin/coinjoin"
    "evo/deterministicmns -> validationinterface -> txmempool -> evo/deterministicmns"
    "llmq/quorums -> validation -> llmq/quorums_chainlocks -> llmq/quorums"
    "llmq/quorums_chainlocks -> llmq/quorums_instantsend -> validation -> llmq/quorums_chainlocks"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> evo/deterministicmns"
    "core_io -> evo/cbtx -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> governance/governance-classes -> core_io"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> masternode/activemasternode -> evo/deterministicmns"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net -> masternode/masternode-sync -> evo/deterministicmns"
    "governance/governance -> net_processing -> masternode/masternode-payments -> governance/governance"
    "core_io -> evo/cbtx -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> governance/governance-classes -> governance/governance-object -> core_io"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> governance/governance-classes -> governance/governance-object -> evo/deterministicmns"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net -> evo/deterministicmns"
    "evo/deterministicmns -> evo/simplifiedmns -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternode-payments -> governance/governance-classes -> governance/governance-object -> governance/governance-vote -> evo/deterministicmns"
    "index/txindex -> init -> index/txindex"
    "index/txindex -> init -> llmq/quorums_init -> llmq/quorums_instantsend -> index/txindex"
    "index/txindex -> init -> net_processing -> index/txindex"
    "index/txindex -> init -> rpc/blockchain -> index/txindex"
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
