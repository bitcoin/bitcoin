#!/usr/bin/env python3
#
# Copyright (c) 2020-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Check for circular dependencies

import os
import re
import subprocess
import sys

EXPECTED_CIRCULAR_DEPENDENCIES = (
    "chainparamsbase -> util/system -> chainparamsbase",
    "node/blockstorage -> validation -> node/blockstorage",
    "index/coinstatsindex -> node/coinstats -> index/coinstatsindex",
    "policy/fees -> txmempool -> policy/fees",
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel",
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel",
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel",
    "wallet/wallet -> wallet/walletdb -> wallet/wallet",
    "node/coinstats -> validation -> node/coinstats",
    # Dash
    "banman -> common/bloom -> evo/assetlocktx -> llmq/quorums -> net -> banman",
    "banman -> common/bloom -> evo/assetlocktx -> llmq/signing -> net_processing -> banman",
    "coinjoin/client -> net_processing -> coinjoin/client",
    "coinjoin/client -> net_processing -> coinjoin/context -> coinjoin/client",
    "coinjoin/context -> coinjoin/server -> net_processing -> coinjoin/context",
    "coinjoin/server -> net_processing -> coinjoin/server",
    "common/bloom -> evo/assetlocktx -> llmq/quorums -> net -> common/bloom",
    "common/bloom -> evo/assetlocktx -> llmq/signing -> net_processing -> merkleblock -> common/bloom",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/commitment -> validation -> consensus/tx_verify",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/signing -> net_processing -> txmempool -> consensus/tx_verify",
    "evo/assetlocktx -> llmq/signing -> net_processing -> txmempool -> evo/assetlocktx",
    "evo/chainhelper -> evo/specialtxman -> validation -> evo/chainhelper",
    "evo/deterministicmns -> index/txindex -> validation -> evo/deterministicmns",
    "evo/deterministicmns -> index/txindex -> validation -> txmempool -> evo/deterministicmns",
    "evo/netinfo -> evo/providertx -> evo/netinfo",
    "evo/smldiff -> llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> evo/smldiff",
    "core_io -> evo/assetlocktx -> llmq/signing -> net_processing -> evo/smldiff -> core_io",
    "evo/specialtxman -> validation -> evo/specialtxman",
    "governance/governance -> governance/object -> governance/governance",
    "governance/governance -> masternode/sync -> governance/governance",
    "governance/governance -> net_processing -> governance/governance",
    "instantsend/instantsend -> instantsend/signing -> instantsend/instantsend",
    "instantsend/instantsend -> llmq/chainlocks -> instantsend/instantsend",
    "instantsend/instantsend -> net_processing -> instantsend/instantsend",
    "instantsend/instantsend -> net_processing -> llmq/context -> instantsend/instantsend",
    "instantsend/instantsend -> txmempool -> instantsend/instantsend",
    "llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> llmq/blockprocessor",
    "llmq/chainlocks -> validation -> llmq/chainlocks",
    "llmq/commitment -> llmq/utils -> llmq/snapshot -> llmq/commitment",
    "llmq/chainlocks -> llmq/signing -> net_processing -> llmq/chainlocks",
    "llmq/context -> llmq/signing -> net_processing -> llmq/context",
    "llmq/dkgsession -> llmq/dkgsessionmgr -> llmq/dkgsessionhandler -> llmq/dkgsession",
    "llmq/dkgsessionhandler -> net_processing -> llmq/dkgsessionmgr -> llmq/dkgsessionhandler",
    "llmq/signing -> llmq/signing_shares -> llmq/signing",
    "llmq/signing -> net_processing -> llmq/signing",
    "llmq/signing_shares -> net_processing -> llmq/signing_shares",
    "masternode/payments -> validation -> masternode/payments",
    "net -> netmessagemaker -> net",
    "net_processing -> spork -> net_processing",
    "netaddress -> netbase -> netaddress",
    "qt/appearancewidget -> qt/guiutil -> qt/appearancewidget",
    "qt/bitcoinaddressvalidator -> qt/guiutil -> qt/bitcoinaddressvalidator",
    "qt/bitcoingui -> qt/guiutil -> qt/bitcoingui",
    "qt/guiutil -> qt/qvalidatedlineedit -> qt/guiutil",
    "wallet/coinjoin -> wallet/receive -> wallet/coinjoin",
)

CODE_DIR = "src"


def main():
    circular_dependencies = []
    exit_code = 0

    os.chdir(CODE_DIR)
    files = subprocess.check_output(
        ['git', 'ls-files', '--', '*.h', '*.cpp'],
        universal_newlines=True,
    ).splitlines()

    command = [sys.executable, "../contrib/devtools/circular-dependencies.py", *files]
    dependencies_output = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        universal_newlines=True,
    )

    for dependency_str in dependencies_output.stdout.rstrip().split("\n"):
        circular_dependencies.append(
            re.sub("^Circular dependency: ", "", dependency_str)
        )

    # Check for an unexpected dependencies
    for dependency in circular_dependencies:
        if dependency not in EXPECTED_CIRCULAR_DEPENDENCIES:
            exit_code = 1
            print(
                f'A new circular dependency in the form of "{dependency}" appears to have been introduced.\n',
                file=sys.stderr,
            )

    # Check for missing expected dependencies
    for expected_dependency in EXPECTED_CIRCULAR_DEPENDENCIES:
        if expected_dependency not in circular_dependencies:
            exit_code = 1
            print(
                f'Good job! The circular dependency "{expected_dependency}" is no longer present.',
            )
            print(
                f"Please remove it from EXPECTED_CIRCULAR_DEPENDENCIES in {__file__}",
            )
            print(
                "to make sure this circular dependency is not accidentally reintroduced.\n",
            )

    sys.exit(exit_code)


if __name__ == "__main__":
    main()
