// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <init/common.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <interfaces/wallet.h>
#include <ipc/context.h>
#include <kernel/context.h>
#include <key.h>
#include <logging.h>
#include <pubkey.h>
#include <random.h>
#include <util/fs.h>
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
    BitcoinWalletInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, ".wallet", arg0, *this))
    {
        // Extra initialization code that runs when a bitcoin-wallet process is
        // spawned by a bitcoin-node process, after the ArgsManager
        // configuration is transferred from the parent process to the child
        // process. This is a subset of the intialization done in bitcoind
        // AppInit() function, doing only initialization needed by the wallet.
        m_ipc->context().init_process = [this] {
            // Fix: get rid of kernel dependency. Wallet process should not be
            // linked against kernel code. Everything in kernel constructor
            // should be moved to a util::SetGlobals function, everything in
            // destructor should be moved to a util::UnsetGlobals function and
            // that should be called instead. Alternately there can be a
            // util::Globals class that becomes a memberof kernel::Context
            m_kernel.emplace();
            m_ecc_context.emplace();
            init::SetLoggingOptions(gArgs, m_ipc->logSuffix());
            if (auto result = init::SetLoggingCategories(gArgs); !result) {
                throw std::runtime_error(util::ErrorString(result).original);
            }
            if (!init::StartLogging(gArgs)) {
                throw std::runtime_error("Logging start failure");
            }
        };
    }
    std::unique_ptr<interfaces::WalletLoader> makeWalletLoader(interfaces::Chain& chain) override
    {
        return MakeWalletLoader(chain, gArgs);
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    std::unique_ptr<interfaces::Ipc> m_ipc;
    std::optional<kernel::Context> m_kernel;
    std::optional<ECC_Context> m_ecc_context;
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
