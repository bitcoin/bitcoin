// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

namespace interfaces {
std::unique_ptr<Init> MakeBasicInit(const char* exe_name, const char* process_argv0)
{
    return std::make_unique<Init>();
}
} // namespace interfaces
