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
    "chainparamsbase -> common/args -> chainparamsbase",
    "node/blockstorage -> validation -> node/blockstorage",
    "node/utxo_snapshot -> validation -> node/utxo_snapshot",
    "qt/addresstablemodel -> qt/walletmodel -> qt/addresstablemodel",
    "qt/recentrequeststablemodel -> qt/walletmodel -> qt/recentrequeststablemodel",
    "qt/sendcoinsdialog -> qt/walletmodel -> qt/sendcoinsdialog",
    "qt/transactiontablemodel -> qt/walletmodel -> qt/transactiontablemodel",
    "wallet/wallet -> wallet/walletdb -> wallet/wallet",
    "kernel/coinstats -> validation -> kernel/coinstats",
    "kernel/mempool_persist -> validation -> kernel/mempool_persist",

    # Temporary, removed in followup https://github.com/bitcoin/bitcoin/pull/24230
    # solved
    # "index/base -> node/context -> net_processing -> index/blockfilterindex -> index/base",

    # Syscoin
    "auxpow -> primitives/block -> auxpow",
    "chain -> node/blockstorage -> chain",
    "chain -> node/blockstorage -> kernel/chain -> chain",
    "chain -> validation -> chain",
    "chainparams -> kernel/chainparams -> chainparams",
    "common/args -> logging -> common/args",
    "core_io -> evo/providertx -> core_io",
    "evo/cbtx -> evo/specialtx -> evo/cbtx",
    "evo/cbtx -> llmq/quorums_commitment -> evo/cbtx",
    "evo/deterministicmns -> evo/providertx -> evo/deterministicmns",
    "evo/deterministicmns -> evo/specialtx -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/quorums_commitment -> evo/deterministicmns",
    "evo/deterministicmns -> llmq/quorums_utils -> evo/deterministicmns",
    "evo/deterministicmns -> node/interface_ui -> evo/deterministicmns",
    "evo/mnauth -> net_processing -> evo/mnauth",
    "evo/specialtx -> llmq/quorums_blockprocessor -> evo/specialtx",
    "evo/specialtx -> llmq/quorums_commitment -> evo/specialtx",
    "evo/specialtx -> validation -> evo/specialtx",
    "governance/governance -> governance/governanceclasses -> governance/governance",
    "governance/governance -> governance/governanceobject -> governance/governance",
    "governance/governance -> init -> governance/governance",
    "governance/governance -> masternode/masternodesync -> governance/governance",
    "governance/governance -> net_processing -> governance/governance",
    "init -> llmq/quorums -> init",
    "init -> masternode/masternodesync -> init",
    "init -> masternode/masternodeutils -> init",
    "init -> netfulfilledman -> init",
    "llmq/quorums -> llmq/quorums_init -> llmq/quorums",
    "llmq/quorums -> llmq/quorums_utils -> llmq/quorums",
    "llmq/quorums_blockprocessor -> net_processing -> llmq/quorums_blockprocessor",
    "llmq/quorums_chainlocks -> net_processing -> llmq/quorums_chainlocks",
    "llmq/quorums_chainlocks -> validation -> llmq/quorums_chainlocks",
    "llmq/quorums_commitment -> llmq/quorums_utils -> llmq/quorums_commitment",
    "llmq/quorums_dkgsession -> llmq/quorums_dkgsessionmgr -> llmq/quorums_dkgsession",
    "llmq/quorums_dkgsessionhandler -> llmq/quorums_dkgsessionmgr -> llmq/quorums_dkgsessionhandler",
    "llmq/quorums_dkgsessionmgr -> net_processing -> llmq/quorums_dkgsessionmgr",
    "llmq/quorums_signing -> llmq/quorums_signing_shares -> llmq/quorums_signing",
    "llmq/quorums_signing -> net_processing -> llmq/quorums_signing",
    "llmq/quorums_signing_shares -> net_processing -> llmq/quorums_signing_shares",
    "masternode/masternodepayments -> validation -> masternode/masternodepayments",
    "net -> netmessagemaker -> net",
    "net_processing -> spork -> net_processing",
    "services/nevmconsensus -> validation -> services/nevmconsensus",
    "chain -> validation -> consensus/tx_verify -> chain",
    "chain -> validation -> deploymentstatus -> chain",
    "chain -> node/blockstorage -> pow -> chain",
    "chain -> validation -> txmempool -> chain",
    "chain -> validation -> validationinterface -> chain",
    "chain -> validation -> versionbits -> chain",
    "consensus/tx_verify -> services/nevmconsensus -> validation -> consensus/tx_verify",
    "core_io -> evo/cbtx -> evo/deterministicmns -> core_io",
    "dsnotificationinterface -> governance/governance -> init -> dsnotificationinterface",
    "dsnotificationinterface -> net_processing -> node/blockstorage -> dsnotificationinterface",
    "evo/deterministicmns -> validation -> txmempool -> evo/deterministicmns",
    "evo/providertx -> validation -> txmempool -> evo/providertx",
    "evo/specialtx -> validation -> txmempool -> evo/specialtx",
    "governance/governance -> init -> node/chainstate -> governance/governance",
    "init -> llmq/quorums_init -> llmq/quorums_signing -> init",
    "init -> llmq/quorums_init -> llmq/quorums_signing_shares -> init",
    "llmq/quorums -> llmq/quorums_init -> llmq/quorums_chainlocks -> llmq/quorums",
    "llmq/quorums -> llmq/quorums_init -> llmq/quorums_signing -> llmq/quorums",
    "llmq/quorums -> llmq/quorums_init -> llmq/quorums_signing_shares -> llmq/quorums",
    "llmq/quorums -> llmq/quorums_blockprocessor -> net_processing -> llmq/quorums",
    "llmq/quorums_blockprocessor -> llmq/quorums_utils -> llmq/quorums_init -> llmq/quorums_blockprocessor",
    "llmq/quorums_chainlocks -> llmq/quorums_utils -> llmq/quorums_init -> llmq/quorums_chainlocks",
    "llmq/quorums_commitment -> llmq/quorums_utils -> llmq/quorums_init -> llmq/quorums_commitment",
    "llmq/quorums_debug -> llmq/quorums_utils -> llmq/quorums_init -> llmq/quorums_debug",
    "llmq/quorums_dkgsessionmgr -> llmq/quorums_utils -> llmq/quorums_init -> llmq/quorums_dkgsessionmgr",
    "llmq/quorums_init -> llmq/quorums_signing -> llmq/quorums_utils -> llmq/quorums_init",
    "llmq/quorums_init -> llmq/quorums_signing -> net_processing -> llmq/quorums_init",
    "masternode/activemasternode -> validationinterface -> node/blockstorage -> masternode/activemasternode",
    "net_processing -> node/blockstorage -> node/context -> net_processing",
    "nevm/commondata -> nevm/exceptions -> nevm/fixedhash -> nevm/commondata",
    "node/blockstorage -> validation -> validationinterface -> node/blockstorage",
    "chain -> node/blockstorage -> node/context -> node/kernel_notifications -> chain",
    "chain -> validation -> zmq/zmqnotificationinterface -> zmq/zmqpublishnotifier -> chain",
    "common/bloom -> evo/cbtx -> llmq/quorums_blockprocessor -> net -> common/bloom",
    "consensus/tx_verify -> services/nevmconsensus -> validation -> txmempool -> consensus/tx_verify",
    "core_io -> llmq/quorums_commitment -> node/blockstorage -> signet -> core_io",
    "net -> rpc/server -> rpc/util -> node/transaction -> net",
    "node/blockstorage -> validation -> zmq/zmqnotificationinterface -> zmq/zmqpublishnotifier -> node/blockstorage",
    "node/transaction -> validation -> zmq/zmqrpc -> rpc/util -> node/transaction",
    "banman -> common/bloom -> evo/cbtx -> llmq/quorums_blockprocessor -> net -> banman",
    "banman -> common/bloom -> evo/cbtx -> llmq/quorums_blockprocessor -> net_processing -> banman",
    "banman -> common/bloom -> llmq/quorums_commitment -> node/blockstorage -> node/context -> banman",
    "core_io -> evo/cbtx -> validation -> zmq/zmqrpc -> rpc/util -> core_io",
    "core_io -> evo/cbtx -> llmq/quorums_blockprocessor -> net_processing -> masternode/masternodepayments -> governance/governanceclasses -> core_io"
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
