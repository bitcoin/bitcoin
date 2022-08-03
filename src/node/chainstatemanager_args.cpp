// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/chainstatemanager_args.h>

#include <kernel/chainstatemanager_opts.h>

#include <node/txdb_args.h>

using kernel::ChainstateManagerOpts;

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, ChainstateManagerOpts& chainman_opts)
{
    ApplyArgsManOptions(argsman, chainman_opts.block_tree_db_opts);
}

} // namespace node
