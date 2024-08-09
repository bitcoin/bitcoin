// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <kernel/mempool_removal_reason.h>

#include <cassert>
#include <string>

std::string RemovalReasonToString(const MemPoolRemovalReason& r) noexcept
{
  return std::visit([](auto&& arg) {
    return arg.toString();
  }, r);
}
