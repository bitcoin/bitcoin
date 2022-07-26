// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/txdb_args.h>

#include <kernel/txdb_options.h>

#include <util/system.h>

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, kernel::BlockTreeDBOpts& opts)
{
    opts.do_compact = argsman.GetBoolArg("-forcecompactdb", false);
}

} // namespace node
