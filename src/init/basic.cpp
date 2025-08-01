// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <interfaces/ipc.h>

namespace init {
namespace {
class BitcoinBasicInit : public interfaces::Init
{
public:
    BitcoinBasicInit(const char* exe_name, const char* process_argv0) : m_ipc(interfaces::MakeIpc(exe_name, process_argv0, *this)) {}
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
private:
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeBasicInit(const char* exe_name, const char* process_argv0)
{
    return std::make_unique<init::BitcoinBasicInit>(exe_name, process_argv0);
}
} // namespace interfaces
