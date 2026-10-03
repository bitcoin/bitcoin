// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_SNAPSHOT_H
#define BITCOIN_INTERFACES_SNAPSHOT_H

#include <util/result.h>

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

    //! Activate the snapshot, making it the active chainstate. On success,
    //! returns a pointer to the block index of the snapshot base. On failure,
    //! the returned result carries a descriptive error message.
    virtual util::Result<const CBlockIndex*> activate() = 0;

    //! Get the snapshot metadata. Only valid after activate() has been called
    //! (successfully or not), since metadata is populated as a side effect of
    //! reading the snapshot file.
    virtual const node::SnapshotMetadata& getMetadata() const = 0;
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_SNAPSHOT_H
