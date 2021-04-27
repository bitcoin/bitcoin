// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/echo.h>

#include <memory>

namespace interfaces {
namespace {
class EchoImpl : public Echo
{
public:
    std::string echo(const std::string& echo) override { return echo; }
};
} // namespace
std::unique_ptr<Echo> MakeEcho() { return std::make_unique<EchoImpl>(); }
} // namespace interfaces
