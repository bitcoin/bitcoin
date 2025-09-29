// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_TYPES_H
#define BITCOIN_LLMQ_TYPES_H

#include <memory>

namespace llmq {
class CFinalCommitment;
class CQuorum;

using CFinalCommitmentPtr = std::unique_ptr<CFinalCommitment>;
using CQuorumPtr = std::shared_ptr<CQuorum>;
using CQuorumCPtr = std::shared_ptr<const CQuorum>;
} // namespace llmq

#endif // BITCOIN_LLMQ_TYPES_H
