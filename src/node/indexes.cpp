// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txindex.h>
#include <node/context.h>

namespace node {

void TxIndexDeleter::operator()(TxIndex* index) const noexcept { delete index; }

} // namespace node
