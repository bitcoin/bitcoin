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
    "policy/fees -> txmempool -> policy/fees",
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel",
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel",
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel",
    "wallet/wallet -> wallet/walletdb -> wallet/wallet",
    "kernel/coinstats -> validation -> kernel/coinstats",
    # Dash
    "active/context -> llmq/signing_shares -> net_processing -> active/context",
    "banman -> common/bloom -> evo/assetlocktx -> llmq/quorumsman -> net -> banman",
    "chainlock/handler -> instantsend/instantsend -> instantsend/signing -> llmq/signing_shares -> net_processing -> chainlock/handler",
    "chainlock/chainlock -> spork -> msg_result -> coinjoin/coinjoin -> chainlock/chainlock",
    "chainlock/chainlock -> spork -> net -> evo/deterministicmns -> evo/providertx -> validation -> chainlock/chainlock",
    "coinjoin/coinjoin -> instantsend/instantsend -> spork -> msg_result -> coinjoin/coinjoin",
    "coinjoin/coinjoin -> instantsend/instantsend -> instantsend/signing -> llmq/signing_shares -> net_processing -> coinjoin/coinjoin",
    "coinjoin/client -> coinjoin/coinjoin -> instantsend/instantsend -> instantsend/signing -> llmq/signing_shares -> net_processing -> coinjoin/walletman -> coinjoin/client",
    "common/bloom -> evo/assetlocktx -> llmq/commitment -> evo/deterministicmns -> evo/simplifiedmns -> merkleblock -> common/bloom",
    "common/bloom -> evo/assetlocktx -> llmq/quorumsman -> net -> common/bloom",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/commitment -> validation -> consensus/tx_verify",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/commitment -> validation -> txmempool -> consensus/tx_verify",
    "evo/assetlocktx -> llmq/commitment -> validation -> txmempool -> evo/assetlocktx",
    "evo/chainhelper -> evo/creditpool -> validation -> evo/chainhelper",
    "evo/deterministicmns -> evo/providertx -> validation -> evo/deterministicmns",
    "evo/deterministicmns -> evo/providertx -> validation -> txmempool -> evo/deterministicmns",
    "evo/netinfo -> evo/providertx -> evo/netinfo",
    "evo/netinfo -> evo/providertx -> validation -> txmempool -> evo/netinfo",
    "evo/providertx -> validation -> txmempool -> evo/providertx",
    "evo/smldiff -> llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> evo/smldiff",
    "evo/specialtxman -> validation -> evo/specialtxman",
    "governance/classes -> governance/object -> governance/governance -> governance/classes",
    "governance/governance -> governance/signing -> governance/object -> governance/governance",
    "instantsend/instantsend -> instantsend/signing -> llmq/signing_shares -> net_processing -> instantsend/instantsend",
    "instantsend/instantsend -> instantsend/signing -> llmq/signing_shares -> net_processing -> llmq/context -> instantsend/instantsend",
    "instantsend/instantsend -> txmempool -> instantsend/instantsend",
    "llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> llmq/blockprocessor",
    "llmq/commitment -> llmq/utils -> llmq/snapshot -> llmq/commitment",
    "llmq/dkgsessionhandler -> net_processing -> llmq/dkgsessionmgr -> llmq/dkgsessionhandler",
    "llmq/signing_shares -> net_processing -> llmq/signing_shares",
    "masternode/payments -> validation -> masternode/payments",
    "net -> netmessagemaker -> net",
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
        text=True,
    ).splitlines()

    command = [sys.executable, "../contrib/devtools/circular-dependencies.py", *files]
    dependencies_output = subprocess.run(
        command,
        stdout=subprocess.PIPE,
        text=True,
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
