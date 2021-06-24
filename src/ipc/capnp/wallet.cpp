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
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/proxy.h>
#include <mp/util.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <scheduler.h>
#include <script/standard.h>
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

namespace mp {
void ProxyServerCustom<ipc::capnp::messages::WalletClient, interfaces::WalletClient>::invokeDestroy()
{
    if (m_scheduler) {
        m_scheduler->stop();
        m_result.get();
        m_scheduler.reset();
    }
    ProxyServerBase::invokeDestroy();
}

void ProxyServerMethodTraits<ipc::capnp::messages::ChainClient::StartParams>::invoke(WalletContext& context)
{
    if (!context.proxy_server.m_scheduler) {
        context.proxy_server.m_scheduler = std::make_unique<CScheduler>();
        CScheduler* scheduler = context.proxy_server.m_scheduler.get();
        context.proxy_server.m_result = std::async([scheduler]() {
            util::ThreadRename("schedqueue");
            scheduler->serviceQueue();
        });
    }
    context.proxy_server.m_impl->start(*context.proxy_server.m_scheduler);
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const CTxDestination& dest,
                        ipc::capnp::messages::TxDestination::Builder&& builder)
{
    if (const PKHash* pkHash = std::get_if<PKHash>(&dest)) {
        builder.setPkHash(ipc::capnp::ToArray(*pkHash));
    } else if (const ScriptHash* scriptHash = std::get_if<ScriptHash>(&dest)) {
        builder.setScriptHash(ipc::capnp::ToArray(*scriptHash));
    } else if (const WitnessV0ScriptHash* witnessV0ScriptHash = std::get_if<WitnessV0ScriptHash>(&dest)) {
        builder.setWitnessV0ScriptHash(ipc::capnp::ToArray(*witnessV0ScriptHash));
    } else if (const WitnessV0KeyHash* witnessV0KeyHash = std::get_if<WitnessV0KeyHash>(&dest)) {
        builder.setWitnessV0KeyHash(ipc::capnp::ToArray(*witnessV0KeyHash));
    } else if (const WitnessUnknown* witnessUnknown = std::get_if<WitnessUnknown>(&dest)) {
        BuildField(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(builder.initWitnessUnknown()),
                   *witnessUnknown);
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::TxDestination::Reader& reader,
                       CTxDestination& dest)
{
    if (reader.hasPkHash()) {
        dest = PKHash(ipc::capnp::ToBlob<uint160>(reader.getPkHash()));
    } else if (reader.hasScriptHash()) {
        dest = ScriptHash(ipc::capnp::ToBlob<uint160>(reader.getScriptHash()));
    } else if (reader.hasWitnessV0ScriptHash()) {
        dest = WitnessV0ScriptHash(ipc::capnp::ToBlob<uint256>(reader.getWitnessV0ScriptHash()));
    } else if (reader.hasWitnessV0KeyHash()) {
        dest = WitnessV0KeyHash(ipc::capnp::ToBlob<uint160>(reader.getWitnessV0KeyHash()));
    } else if (reader.hasWitnessUnknown()) {
        ReadField(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(reader.getWitnessUnknown()),
                  ReadDestValue(std::get<WitnessUnknown>(dest)));
    }
}

void CustomBuildMessage(InvokeContext& invoke_context, const CKey& key, ipc::capnp::messages::Key::Builder&& builder)
{
    builder.setSecret(ipc::capnp::FromBlob(key));
    builder.setIsCompressed(key.IsCompressed());
}

void CustomReadMessage(InvokeContext& invoke_context, const ipc::capnp::messages::Key::Reader& reader, CKey& key)
{
    auto secret = reader.getSecret();
    key.Set(secret.begin(), secret.end(), reader.getIsCompressed());
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const CCoinControl& coin_control,
                        ipc::capnp::messages::CoinControl::Builder&& builder)
{
    CustomBuildMessage(invoke_context, coin_control.destChange, builder.initDestChange());
    if (coin_control.m_change_type) {
        builder.setHasChangeType(true);
        builder.setChangeType(static_cast<int>(*coin_control.m_change_type));
    }
    builder.setAllowOtherInputs(coin_control.fAllowOtherInputs);
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
    builder.setFeeMode(int32_t(coin_control.m_fee_mode));
    builder.setMinDepth(coin_control.m_min_depth);
    std::vector<COutPoint> selected;
    coin_control.ListSelected(selected);
    auto builder_selected = builder.initSetSelected(selected.size());
    size_t i = 0;
    for (const COutPoint& output : selected) {
        builder_selected.set(i, ipc::capnp::ToArray(ipc::capnp::Serialize(output)));
        ++i;
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::CoinControl::Reader& reader,
                       CCoinControl& coin_control)
{
    CustomReadMessage(invoke_context, reader.getDestChange(), coin_control.destChange);
    if (reader.getHasChangeType()) {
        coin_control.m_change_type = OutputType(reader.getChangeType());
    }
    coin_control.fAllowOtherInputs = reader.getAllowOtherInputs();
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
    coin_control.m_fee_mode = FeeEstimateMode(reader.getFeeMode());
    coin_control.m_min_depth = reader.getMinDepth();
    for (const auto output : reader.getSetSelected()) {
        coin_control.Select(ipc::capnp::Unserialize<COutPoint>(output));
    }
}
} // namespace mp
