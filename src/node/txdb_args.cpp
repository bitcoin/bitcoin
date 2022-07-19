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

void ApplyArgsManOptions(const ArgsManager& argsman, kernel::CoinsViewDBOpts& coinsviewdb_opts)
{
    coinsviewdb_opts.do_compact = argsman.GetBoolArg("-forcecompactdb", coinsviewdb_opts.do_compact);
    coinsviewdb_opts.batch_write_size = argsman.GetIntArg("-dbbatchsize", coinsviewdb_opts.batch_write_size);
    coinsviewdb_opts.simulate_write_crash_ratio = argsman.GetIntArg("-dbcrashratio", coinsviewdb_opts.simulate_write_crash_ratio);
}

} // namespace node
