// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <fs.h>
#include <init/common.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/wallet.h>
#include <ipc/context.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <util/system.h>
#include <util/translation.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace interfaces {
class Chain;
} // namespace interfaces

namespace ipc {
namespace capnp {
std::string GlobalArgsNetwork();
} // namespace capnp
} // namespace ipc

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-wallet";

class BitcoinWalletInit : public interfaces::Init
{
public:
    BitcoinWalletInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this))
    {
        // Extra initialization code that runs when a bitcoin-wallet process is
        // spawned by a bitcoin-node process, after the ArgsManager
        // configuration is transferred from the parent process to the child
        // process.
        m_ipc->context().init_process = [] {
            init::SetGlobals();
            if (!init::SanityChecks()) {
                throw std::runtime_error("Initial sanity checks failure");
            }
            SelectParams(ipc::capnp::GlobalArgsNetwork());
            init::SetLoggingOptions(gArgs);
            init::SetLoggingCategories(gArgs);
            if (!init::StartLogging(gArgs)) {
                throw std::runtime_error("Logging start failure");
            }
        };
    }
    std::unique_ptr<interfaces::WalletClient> makeWalletClient(interfaces::Chain& chain) override
    {
        return MakeWalletClient(chain, gArgs);
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status)
{
    auto init = std::make_unique<init::BitcoinWalletInit>(argc > 0 ? argv[0] : "");
    // Check if bitcoin-wallet is being invoked as an IPC server. If so, then
    // bypass normal execution and just respond to requests over the IPC
    // channel and finally return null.
    if (init->m_ipc->startSpawnedProcess(argc, argv, exit_status)) {
        return nullptr;
    }
    return init;
}
} // namespace interfaces
