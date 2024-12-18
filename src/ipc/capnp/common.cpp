// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
                      mp::ReadDestValue(settings));
    });
    gArgs.SetConfigFilePath(fs::PathFromString(ipc::capnp::ToString(reader.getConfigPath())));
    SelectParams(gArgs.GetChainType());
    Context& ipc_context = *static_cast<Context*>(invoke_context.connection.m_loop.m_context);
    ipc_context.init_process();
}
} // namespace capnp
} // namespace ipc
