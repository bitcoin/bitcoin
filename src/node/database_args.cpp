// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/database_args.h>

#include <common/args.h>
#include <dbwrapper.h>
#include <init_settings.h>

namespace node {
void ReadDatabaseArgs(const ArgsManager& args, DBOptions& options)
{
    // Settings here apply to all databases (chainstate, blocks, and index
    // databases), but it'd be easy to parse database-specific options by adding
    // a database_type string or enum parameter to this function.
    if (auto value = ForcecompactdbSettingHidden::Get(args)) options.force_compact = *value;
}
} // namespace node
