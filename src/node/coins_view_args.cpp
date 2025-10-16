// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/coins_view_args.h>

#include <common/args.h>
#include <init_settings.h>
#include <txdb.h>

namespace node {
void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options)
{
    if (auto value = DbBatchSizeSetting::Get(args)) options.batch_write_bytes = *value;
    if (auto value = DbCrashRatioSettingHidden::Get(args)) options.simulate_crash_ratio = *value;
}
} // namespace node
