#include <node/peerman_args.h>

#include <common/args.h>
#include <init_settings.h>
#include <net_processing.h>

#include <algorithm>
#include <limits>

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options)
{
    if (auto value{TxreconciliationSetting::Get(argsman)}) options.reconcile_txs = *value;

    if (auto value{MaxorphantxSetting::Get(argsman)}) {
        options.max_orphan_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{BlockreconstructionextratxnSetting::Get(argsman)}) {
        options.max_extra_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{CapturemessagesSetting::Get(argsman)}) options.capture_messages = *value;

    if (auto value{BlocksonlySetting::Get(argsman)}) options.ignore_incoming_txs = *value;
}

} // namespace node

