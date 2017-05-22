// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees_args.h>

#include <common/args.h>

namespace {
const char* FEE_ESTIMATES_FILENAME = "fee_estimates.dat";
} // namespace

fs::path FeeestPath(const ArgsManager& argsman)
{
    return argsman.GetDataDirNet() / FEE_ESTIMATES_FILENAME;
}

fs::path FeeestLogPath(const ArgsManager& args)
{
    fs::path log_filename = args.GetPathArg("-estlog");
    if (log_filename.empty()) return {};
    return args.GetDataDirNet() / log_filename;
}
