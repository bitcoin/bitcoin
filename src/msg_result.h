// Copyright (c) 2024-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MSG_RESULT_H
#define BITCOIN_MSG_RESULT_H

#include <coinjoin/coinjoin.h>

#include <protocol.h>
#include <uint256.h>

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct MisbehavingError
{
    int score;
    std::string message;

    MisbehavingError(int s) : score{s} {}

    // Constructor does a perfect forwarding reference
    template <typename T>
    MisbehavingError(int s, T&& msg) :
        score{s},
        message{std::forward<T>(msg)}
    {}
};

/**
 * This struct is a helper to return values from handlers that are processing
 * network messages but implemented outside of net_processing.cpp,
 * for example llmq's messages.
 *
 * These handlers do not supposed to know anything about PeerManager to avoid
 * circular dependencies.
 *
 * See `PeerManagerImpl::PostProcessMessage` to see how each type of return code
 * is processed.
 */
struct MessageProcessingResult
{
    //! @m_error triggers Misbehaving error with score and optional message if not nullopt
    std::optional<MisbehavingError> m_error;

    //! @m_inventory will relay these inventories to connected peers
    std::vector<CInv> m_inventory;

    //! @m_dsq will relay DSQs to connected peers
    std::vector<CCoinJoinQueue> m_dsq;

    //! @m_transactions will relay transactions to peers which is ready to accept it (some peers does not accept transactions)
    std::vector<uint256> m_transactions;

    //! @m_to_erase triggers EraseObjectRequest from PeerManager for this inventory if not nullopt
    std::optional<CInv> m_to_erase;

    MessageProcessingResult() = default;
    MessageProcessingResult(CInv inv) :
        m_inventory({inv})
    {
    }
    MessageProcessingResult(MisbehavingError error) :
        m_error(error)
    {}

    bool empty() const { return !m_error.has_value() && m_inventory.empty() && m_dsq.empty() && m_transactions.empty() && !m_to_erase.has_value(); }
};

#endif // BITCOIN_MSG_RESULT_H
