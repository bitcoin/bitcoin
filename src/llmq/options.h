// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_OPTIONS_H
#define BITCOIN_LLMQ_OPTIONS_H

#include <gsl/pointers.h>

#include <map>
#include <optional>
#include <vector>

class ArgsManager;
class CBlockIndex;
class ChainstateManager;
class CSporkManager;
namespace Consensus {
struct LLMQParams;
enum class LLMQType : uint8_t;
} // namespace Consensus

namespace llmq {
enum class QvvecSyncMode : int8_t {
    Invalid = -1,
    Always = 0,
    OnlyIfTypeMember = 1,
};

/** Maximum number of dedicated script-checking threads allowed */
static const int8_t MAX_BLSCHECK_THREADS{33};
/** -parbls default (number of bls-checking threads, 0 = auto) */
static const int8_t DEFAULT_BLSCHECK_THREADS{0};
/** -llmq-data-recovery default */
static constexpr bool DEFAULT_ENABLE_QUORUM_DATA_RECOVERY{true};
/** -watchquorums default, if true, we will connect to all new quorums and watch their communication */
static constexpr bool DEFAULT_WATCH_QUORUMS{false};

bool IsAllMembersConnectedEnabled(const Consensus::LLMQType llmqType, const CSporkManager& sporkman);
bool IsQuorumPoseEnabled(const Consensus::LLMQType llmqType, const CSporkManager& sporkman);
bool IsQuorumRotationEnabled(const Consensus::LLMQParams& llmqParams, gsl::not_null<const CBlockIndex*> pindex);

/// Returns the parsed entries given by `-llmq-qvvec-sync`
using QvvecSyncModeMap = std::map<Consensus::LLMQType, QvvecSyncMode>;
QvvecSyncModeMap GetEnabledQuorumVvecSyncEntries(const ArgsManager& args);

std::vector<Consensus::LLMQType> GetEnabledQuorumTypes(const ChainstateManager& chainman,
                                                       gsl::not_null<const CBlockIndex*> pindex);
std::vector<std::reference_wrapper<const Consensus::LLMQParams>> GetEnabledQuorumParams(
    const ChainstateManager& chainman, gsl::not_null<const CBlockIndex*> pindex);
} // namespace llmq

#endif // BITCOIN_LLMQ_OPTIONS_H
