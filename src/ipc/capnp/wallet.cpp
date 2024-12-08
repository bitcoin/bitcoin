// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/blob.h>
#include <capnp/list.h>
#include <interfaces/wallet.h>
#include <ipc/capnp/chain-types.h>
#include <ipc/capnp/chain.capnp.h>
#include <ipc/capnp/common-types.h>
#include <ipc/capnp/wallet-types.h>
#include <ipc/capnp/wallet.capnp.h>
#include <ipc/capnp/wallet.capnp.proxy-types.h>
#include <ipc/capnp/wallet.capnp.proxy.h>
#include <ipc/capnp/wallet.h>
#include <key.h>
#include <key_io.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/proxy.h>
#include <mp/util.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <scheduler.h>
#include <script/solver.h>
#include <streams.h>
#include <uint256.h>
#include <util/threadnames.h>
#include <wallet/coincontrol.h>
#include <wallet/context.h>

#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string.h>
#include <system_error>
#include <variant>
#include <vector>

namespace ipc {
namespace capnp {
namespace messages {
struct WalletLoader;

//! Wrapper around CScheduler that creates a worker thread in its constructor
//! and shuts it down in its destructor.
class SchedulerThread
{
public:
    SchedulerThread()
    {
        m_result = std::async([this]() {
            util::ThreadRename("schedqueue");
            m_scheduler.serviceQueue();
        });
    }
    ~SchedulerThread()
    {
       m_scheduler.stop();
       m_result.get();
    }
    CScheduler& scheduler() { return m_scheduler; }
private:
    CScheduler m_scheduler;
    std::future<void> m_result;
};
} // namespace messages
} // namespace capnp
} // namespace ipc

namespace mp {
void ProxyServerMethodTraits<ipc::capnp::messages::ChainClient::StartParams>::invoke(WalletLoaderContext& context)
{
    if (!context.proxy_server.m_scheduler) {
        auto scheduler = std::make_unique<ipc::capnp::messages::SchedulerThread>();
        context.proxy_server.m_scheduler = scheduler.get();
        context.proxy_server.m_context.cleanup.emplace_back(MakeAsyncCallable([s=std::move(scheduler)]() mutable { s.reset(); }));
    }
    context.proxy_server.m_impl->start(context.proxy_server.m_scheduler->scheduler());
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const CTxDestination& dest,
                        ipc::capnp::messages::TxDestination::Builder&& builder)
{
    if (std::get_if<CNoDestination>(&dest)) {
    } else if (const PubKeyDestination* data = std::get_if<PubKeyDestination>(&dest)) {
        builder.setPubKey(ipc::capnp::ToArray(data->GetPubKey()));
    } else if (const PKHash* data = std::get_if<PKHash>(&dest)) {
        builder.setPkHash(ipc::capnp::ToArray(*data));
    } else if (const ScriptHash* data = std::get_if<ScriptHash>(&dest)) {
        builder.setScriptHash(ipc::capnp::ToArray(*data));
    } else if (const WitnessV0ScriptHash* data = std::get_if<WitnessV0ScriptHash>(&dest)) {
        builder.setWitnessV0ScriptHash(ipc::capnp::ToArray(*data));
    } else if (const WitnessV0KeyHash* data = std::get_if<WitnessV0KeyHash>(&dest)) {
        builder.setWitnessV0KeyHash(ipc::capnp::ToArray(*data));
    } else if (const WitnessV1Taproot* data = std::get_if<WitnessV1Taproot>(&dest)) {
        builder.setWitnessV1Taproot(ipc::capnp::ToArray(*data));
    } else if (const WitnessUnknown* data = std::get_if<WitnessUnknown>(&dest)) {
        BuildField(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(builder.initWitnessUnknown()), *data);
    } else {
        throw std::logic_error(strprintf("Unrecognized address type. Serialization not implemented for %s", EncodeDestination(dest)));
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::TxDestination::Reader& reader,
                       CTxDestination& dest)
{
    if (reader.hasPubKey()) {
        auto data = reader.getPubKey();
        dest = PubKeyDestination(CPubKey{data.begin(), data.end()});
    } else if (reader.hasPkHash()) {
        dest = PKHash(ipc::capnp::ToBlob<uint160>(reader.getPkHash()));
    } else if (reader.hasScriptHash()) {
        dest = ScriptHash(ipc::capnp::ToBlob<uint160>(reader.getScriptHash()));
    } else if (reader.hasWitnessV0ScriptHash()) {
        dest = WitnessV0ScriptHash(ipc::capnp::ToBlob<uint256>(reader.getWitnessV0ScriptHash()));
    } else if (reader.hasWitnessV0KeyHash()) {
        dest = WitnessV0KeyHash(ipc::capnp::ToBlob<uint160>(reader.getWitnessV0KeyHash()));
    } else if (reader.hasWitnessV1Taproot()) {
        const auto& data = reader.getWitnessV1Taproot();
        dest = WitnessV1Taproot{XOnlyPubKey{Span{data.begin(), data.size()}}};
    } else if (reader.hasWitnessUnknown()) {
        ReadField(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(reader.getWitnessUnknown()),
                  ReadDestValue(std::get<WitnessUnknown>(dest)));
    }
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const WitnessUnknown& dest,
                        ipc::capnp::messages::WitnessUnknown::Builder&& builder)
{
    builder.setVersion(dest.GetWitnessVersion());
    builder.setProgram(ipc::capnp::ToArray(dest.GetWitnessProgram()));
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::WitnessUnknown::Reader& reader,
                       WitnessUnknown& dest)
{
    auto data = reader.getProgram();
    dest = WitnessUnknown{reader.getVersion(), {data.begin(), data.end()}};
}

void CustomBuildMessage(InvokeContext& invoke_context, const CKey& key, ipc::capnp::messages::Key::Builder&& builder)
{
    builder.setSecret(ipc::capnp::ToArray(key));
    builder.setIsCompressed(key.IsCompressed());
}

void CustomReadMessage(InvokeContext& invoke_context, const ipc::capnp::messages::Key::Reader& reader, CKey& key)
{
    auto secret = reader.getSecret();
    key.Set(secret.begin(), secret.end(), reader.getIsCompressed());
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const wallet::CCoinControl& coin_control,
                        ipc::capnp::messages::CoinControl::Builder&& builder)
{
    CustomBuildMessage(invoke_context, coin_control.destChange, builder.initDestChange());
    if (coin_control.m_change_type) {
        builder.setHasChangeType(true);
        builder.setChangeType(static_cast<int>(*coin_control.m_change_type));
    }
    builder.setIncludeUnsafeInputs(coin_control.m_include_unsafe_inputs);
    builder.setAllowOtherInputs(coin_control.m_allow_other_inputs);
    builder.setAllowWatchOnly(coin_control.fAllowWatchOnly);
    builder.setOverrideFeeRate(coin_control.fOverrideFeeRate);
    if (coin_control.m_feerate) {
        builder.setFeeRate(ipc::capnp::ToArray(ipc::capnp::Serialize(*coin_control.m_feerate)));
    }
    if (coin_control.m_confirm_target) {
        builder.setHasConfirmTarget(true);
        builder.setConfirmTarget(*coin_control.m_confirm_target);
    }
    if (coin_control.m_signal_bip125_rbf) {
        builder.setHasSignalRbf(true);
        builder.setSignalRbf(*coin_control.m_signal_bip125_rbf);
    }
    builder.setAvoidPartialSpends(coin_control.m_avoid_partial_spends);
    builder.setAvoidAddressReuse(coin_control.m_avoid_address_reuse);
    builder.setFeeMode(int32_t(coin_control.m_fee_mode));
    builder.setMinDepth(coin_control.m_min_depth);
    builder.setMaxDepth(coin_control.m_min_depth);
    if (coin_control.m_locktime) {
        builder.setLockTime(*coin_control.m_locktime);
    }
    if (coin_control.m_version) {
        builder.setVersion(*coin_control.m_version);
    }
    if (coin_control.m_max_tx_weight) {
        builder.setMaxTxWeight(*coin_control.m_max_tx_weight);
    }
    std::vector<COutPoint> selected = coin_control.ListSelected();
    auto builder_selected = builder.initSetSelected(selected.size());
    size_t i = 0;
    for (const COutPoint& output : selected) {
        builder_selected.set(i, ipc::capnp::ToArray(ipc::capnp::Serialize(output)));
        ++i;
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::CoinControl::Reader& reader,
                       wallet::CCoinControl& coin_control)
{
    CustomReadMessage(invoke_context, reader.getDestChange(), coin_control.destChange);
    if (reader.getHasChangeType()) {
        coin_control.m_change_type = OutputType(reader.getChangeType());
    }
    coin_control.m_include_unsafe_inputs = reader.getIncludeUnsafeInputs();
    coin_control.m_allow_other_inputs = reader.getAllowOtherInputs();
    coin_control.fAllowWatchOnly = reader.getAllowWatchOnly();
    coin_control.fOverrideFeeRate = reader.getOverrideFeeRate();
    if (reader.hasFeeRate()) {
        coin_control.m_feerate = ipc::capnp::Unserialize<CFeeRate>(reader.getFeeRate());
    }
    if (reader.getHasConfirmTarget()) {
        coin_control.m_confirm_target = reader.getConfirmTarget();
    }
    if (reader.getHasSignalRbf()) {
        coin_control.m_signal_bip125_rbf = reader.getSignalRbf();
    }
    coin_control.m_avoid_partial_spends = reader.getAvoidPartialSpends();
    coin_control.m_avoid_address_reuse = reader.getAvoidAddressReuse();
    coin_control.m_fee_mode = FeeEstimateMode(reader.getFeeMode());
    coin_control.m_min_depth = reader.getMinDepth();
    coin_control.m_max_depth = reader.getMinDepth();
    if (reader.getHasLockTime()) {
        coin_control.m_locktime = reader.getLockTime();
    }
    if (reader.getHasVersion()) {
        coin_control.m_version = reader.getVersion();
    }
    if (reader.getHasMaxTxWeight()) {
        coin_control.m_max_tx_weight = reader.getMaxTxWeight();
    }
    for (const auto output : reader.getSetSelected()) {
        coin_control.Select(ipc::capnp::Unserialize<COutPoint>(output));
    }
}
} // namespace mp
