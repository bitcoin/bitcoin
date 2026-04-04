// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_SNAPSHOT_H
#define BITCOIN_INTERFACES_SNAPSHOT_H

#include <memory>
#include <optional>
#include <uint256.h>

class CBlockIndex;
namespace node {
class SnapshotMetadata;
}

namespace interfaces {

//! Interface for managing UTXO snapshots.
class Snapshot
{
public:
    virtual ~Snapshot() = default;

    //! Activate the snapshot, making it the active chainstate.
    virtual bool activate() = 0;

    //! Get the snapshot metadata.
    virtual const node::SnapshotMetadata& getMetadata() const = 0;

    //! Get the activation result (block index of the snapshot base).
    virtual std::optional<const CBlockIndex*> getActivationResult() const = 0;

    //! Get the last error message from activation attempt.
    virtual std::string getLastError() const = 0;
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_SNAPSHOT_H
