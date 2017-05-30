// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>

#include <util/system.h>

namespace interfaces {
namespace {

class ChainImpl : public Chain
{
};

} // namespace

std::unique_ptr<Chain> MakeChain() { return MakeUnique<ChainImpl>(); }

} // namespace interfaces
