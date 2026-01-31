// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/private_broadcast_persist_args.h>

#include <common/args.h>
#include <util/fs.h>

namespace node {

fs::path PrivateBroadcastPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / "privatebroadcast.dat";
}

} // namespace node
