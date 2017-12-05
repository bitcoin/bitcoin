// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <capnp/blob.h>
#include <capnp/common.h>
#include <capnp/list.h>
#include <common/args.h>
#include <common/settings.h>
#include <ipc/capnp/common-types.h>
#include <ipc/capnp/common.capnp.h>
#include <ipc/capnp/common.capnp.proxy-types.h>
#include <ipc/capnp/common.h>
#include <ipc/capnp/context.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <sync.h>
#include <univalue.h>

#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ipc {
namespace capnp {
void BuildGlobalArgs(mp::InvokeContext& invoke_context, messages::GlobalArgs::Builder&& builder)
{
    gArgs.LockSettings([&](const common::Settings& settings) {
        mp::BuildField(mp::TypeList<common::Settings>(), invoke_context,
                       mp::Make<mp::ValueField>(builder.initSettings()), settings);
    });
    builder.setConfigPath(fs::PathToString(gArgs.GetConfigFilePath()));
}

void ReadGlobalArgs(mp::InvokeContext& invoke_context, const messages::GlobalArgs::Reader& reader)
{
    gArgs.LockSettings([&](common::Settings& settings) {
        mp::ReadField(mp::TypeList<common::Settings>(), invoke_context, mp::Make<mp::ValueField>(reader.getSettings()),
                      mp::ReadDestUpdate(settings));
    });
    gArgs.SetConfigFilePath(fs::PathFromString(ipc::capnp::ToString(reader.getConfigPath())));
    SelectParams(gArgs.GetChainType());
}
} // namespace capnp
} // namespace ipc
