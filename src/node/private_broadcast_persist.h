// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_H
#define BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_H

#include <util/fs.h>

class PrivateBroadcast;

namespace node {

/** Dump the private broadcast state to a file. */
void DumpPrivateBroadcast(const PrivateBroadcast& pb, const fs::path& dump_path,
                          fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen,
                          bool skip_file_commit = false);

/** Import the file and attempt to add its contents to the private broadcast state. */
void LoadPrivateBroadcast(PrivateBroadcast& pb, const fs::path& load_path,
                          fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen);

} // namespace node

#endif // BITCOIN_NODE_PRIVATE_BROADCAST_PERSIST_H
