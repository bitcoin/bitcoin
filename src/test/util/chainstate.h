// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#ifndef BITCOIN_TEST_UTIL_CHAINSTATE_H
#define BITCOIN_TEST_UTIL_CHAINSTATE_H

#include <clientversion.h>
#include <fs.h>
#include <node/context.h>
#include <node/utxo_snapshot.h>
#include <rpc/blockchain.h>
#include <validation.h>

#include <univalue.h>

#include <boost/test/unit_test.hpp>

const auto NoMalleation = [](CAutoFile& file, SnapshotMetadata& meta){};

/**
 * Create and activate a UTXO snapshot, optionally providing a function to
 * malleate the snapshot.
 */
template<typename F = decltype(NoMalleation)>
static bool
CreateAndActivateUTXOSnapshot(NodeContext& node, const fs::path root, F malleation = NoMalleation)
{
    // Write out a snapshot to the test's tempdir.
    //
    int height;
    WITH_LOCK(::cs_main, height = node.chainman->ActiveHeight());
    fs::path snapshot_path = root / tfm::format("test_snapshot.%d.dat", height);
    FILE* outfile{fsbridge::fopen(snapshot_path, "wb")};
    CAutoFile auto_outfile{outfile, SER_DISK, CLIENT_VERSION};

    UniValue result = CreateUTXOSnapshot(node, node.chainman->ActiveChainstate(), auto_outfile);
    BOOST_TEST_MESSAGE(
        "Wrote UTXO snapshot to " << fs::PathToString(snapshot_path.make_preferred()) << ": " << result.write());

    // Read the written snapshot in and then activate it.
    //
    FILE* infile{fsbridge::fopen(snapshot_path, "rb")};
    CAutoFile auto_infile{infile, SER_DISK, CLIENT_VERSION};
    SnapshotMetadata metadata;
    auto_infile >> metadata;

    malleation(auto_infile, metadata);

    return node.chainman->ActivateSnapshot(auto_infile, metadata, /*in_memory*/ true);
}


#endif // BITCOIN_TEST_UTIL_CHAINSTATE_H
