// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <ipc/capnp/mining-types.h>
#include <ipc/capnp/mining.capnp.proxy-types.h>

#include <mp/proxy-types.h>

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const BlockValidationState& src,
                        ipc::capnp::messages::BlockValidationState::Builder&& builder)
{
    if (src.IsValid()) {
        builder.setMode(0);
    } else if (src.IsInvalid()) {
        builder.setMode(1);
    } else if (src.IsError()) {
        builder.setMode(2);
    } else {
        assert(false);
    }
    builder.setResult(static_cast<int>(src.GetResult()));
    builder.setRejectReason(src.GetRejectReason());
    builder.setDebugMessage(src.GetDebugMessage());
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::BlockValidationState::Reader& reader,
                       BlockValidationState& dest)
{
    if (reader.getMode() == 0) {
        assert(reader.getResult() == 0);
        assert(reader.getRejectReason().size() == 0);
        assert(reader.getDebugMessage().size() == 0);
    } else if (reader.getMode() == 1) {
        dest.Invalid(static_cast<BlockValidationResult>(reader.getResult()), reader.getRejectReason(), reader.getDebugMessage());
    } else if (reader.getMode() == 2) {
        assert(reader.getResult() == 0);
        dest.Error(reader.getRejectReason());
        assert(reader.getDebugMessage().size() == 0);
    } else {
        assert(false);
    }
}

void CustomBuildMessage(InvokeContext& invoke_context,
                        const std::unique_ptr<node::CBlockTemplate>& src,
                        ipc::capnp::messages::CBlockTemplate::Builder&& builder)
{
    if (src) {
        BuildField(TypeList<node::CBlockTemplate>(), invoke_context, ValueField(builder), *src);
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::CBlockTemplate::Reader& reader,
                       std::unique_ptr<node::CBlockTemplate>& dest)
{
    dest = std::make_unique<node::CBlockTemplate>();
    ReadField(TypeList<node::CBlockTemplate>(), invoke_context, ValueField(reader), ReadDestValue(*dest));
}
} // namespace mp
