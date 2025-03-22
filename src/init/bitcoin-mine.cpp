// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <interfaces/ipc.h>

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-mine";

class BitcoinMineInit : public interfaces::Init
{
public:
    BitcoinMineInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this))
    {
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeMineInit(int argc, char* argv[])
{
    return std::make_unique<init::BitcoinMineInit>(argc > 0 ? argv[0] : "");
}
} // namespace interfaces
