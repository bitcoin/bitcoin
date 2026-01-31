// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/ipc.h>

#include <memory>

namespace interfaces {
std::unique_ptr<Ipc> MakeIpc(const char* exe_name, const char* process_argv0, Init& init)
{
    return {};
}
} // namespace interfaces
