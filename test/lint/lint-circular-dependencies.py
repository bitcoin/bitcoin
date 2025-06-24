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
    "wallet/fees -> wallet/wallet -> wallet/fees",
    "wallet/wallet -> wallet/walletdb -> wallet/wallet",
    "node/coinstats -> validation -> node/coinstats",
    # Dash
    "banman -> common/bloom -> evo/assetlocktx -> llmq/quorums -> net -> banman",
    "banman -> common/bloom -> evo/assetlocktx -> llmq/signing -> net_processing -> banman",
    "coinjoin/client -> net_processing -> coinjoin/client",
    "coinjoin/client -> net_processing -> coinjoin/context -> coinjoin/client",
    "coinjoin/coinjoin -> llmq/chainlocks -> net -> coinjoin/coinjoin",
    "coinjoin/context -> coinjoin/server -> net_processing -> coinjoin/context",
    "coinjoin/server -> net_processing -> coinjoin/server",
    "common/bloom -> evo/assetlocktx -> llmq/quorums -> net -> common/bloom",
    "common/bloom -> evo/assetlocktx -> llmq/signing -> net_processing -> merkleblock -> common/bloom",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/commitment -> validation -> consensus/tx_verify",
    "consensus/tx_verify -> evo/assetlocktx -> llmq/signing -> net_processing -> txmempool -> consensus/tx_verify",
    "core_io -> evo/cbtx -> evo/simplifiedmns -> core_io",
    "dsnotificationinterface -> llmq/chainlocks -> node/blockstorage -> dsnotificationinterface",
    "evo/assetlocktx -> llmq/signing -> net_processing -> txmempool -> evo/assetlocktx",
    "evo/cbtx -> evo/simplifiedmns -> evo/cbtx",
    "evo/chainhelper -> evo/specialtxman -> validation -> evo/chainhelper",
    "evo/deterministicmns -> index/txindex -> node/blockstorage -> evo/deterministicmns",
    "evo/deterministicmns -> index/txindex -> node/blockstorage -> masternode/node -> evo/deterministicmns",
    "evo/deterministicmns -> index/txindex -> validation -> evo/deterministicmns",
    "evo/deterministicmns -> index/txindex -> validation -> txmempool -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/commitment -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/utils -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/utils -> llmq/snapshot -> evo/simplifiedmns -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/utils -> net -> evo/deterministicmns",
    "evo/deterministicmns -> validationinterface -> evo/deterministicmns",
    "evo/deterministicmns -> validationinterface -> governance/vote -> evo/deterministicmns",
    "evo/simplifiedmns -> llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> evo/simplifiedmns",
    "evo/specialtxman -> validation -> evo/specialtxman",
    "governance/governance -> governance/object -> governance/governance",
    "governance/governance -> masternode/sync -> governance/governance",
    "governance/governance -> net_processing -> governance/governance",
    "governance/vote -> masternode/node -> validationinterface -> governance/vote",
    "llmq/blockprocessor -> llmq/utils -> llmq/snapshot -> llmq/blockprocessor",
    "llmq/chainlocks -> llmq/instantsend -> llmq/chainlocks",
    "llmq/chainlocks -> llmq/instantsend -> net_processing -> llmq/chainlocks",
    "llmq/chainlocks -> validation -> llmq/chainlocks",
    "llmq/commitment -> llmq/utils -> llmq/snapshot -> llmq/commitment",
    "llmq/context -> llmq/instantsend -> net_processing -> llmq/context",
    "llmq/dkgsession -> llmq/dkgsessionmgr -> llmq/dkgsessionhandler -> llmq/dkgsession",
    "llmq/dkgsessionhandler -> net_processing -> llmq/dkgsessionmgr -> llmq/dkgsessionhandler",
    "llmq/instantsend -> net_processing -> llmq/instantsend",
    "llmq/instantsend -> txmempool -> llmq/instantsend",
    "llmq/signing -> llmq/signing_shares -> llmq/signing",
    "llmq/signing -> masternode/node -> validationinterface -> llmq/signing",
    "llmq/signing -> net_processing -> llmq/signing",
    "llmq/signing_shares -> net_processing -> llmq/signing_shares",
    "logging -> util/system -> logging",
    "logging -> util/system -> stacktraces -> logging",
    "logging -> util/system -> sync -> logging",
    "logging -> util/system -> sync -> logging/timer -> logging",
    "logging -> util/system -> util/getuniquepath -> random -> logging",
    "masternode/payments -> validation -> masternode/payments",
    "net -> netmessagemaker -> net",
    "net_processing -> spork -> net_processing",
    "netaddress -> netbase -> netaddress",
    "qt/appearancewidget -> qt/guiutil -> qt/appearancewidget",
    "qt/appearancewidget -> qt/guiutil -> qt/optionsdialog -> qt/appearancewidget",
    "qt/bitcoinaddressvalidator -> qt/guiutil -> qt/bitcoinaddressvalidator",
    "qt/bitcoingui -> qt/guiutil -> qt/bitcoingui",
    "qt/guiutil -> qt/optionsdialog -> qt/guiutil",
    "qt/guiutil -> qt/optionsdialog -> qt/optionsmodel -> qt/guiutil",
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
