// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VALIDATION_H
#define BITCOIN_CONSENSUS_VALIDATION_H

#include <string>

/** A "reason" why a transaction was invalid, suitable for determining whether the
  * provider of the transaction should be banned/ignored/disconnected/etc.
  */
enum class TxValidationResult {
    TX_RESULT_UNSET = 0,     //!< initial value. Tx has not yet been rejected
    TX_CONSENSUS,            //!< invalid by consensus rules
    /**
     * Invalid by a change to consensus rules more recent than some major deployment.
     */
    // Only loose txn:
    TX_RECENT_CONSENSUS_CHANGE,
    TX_INPUTS_NOT_STANDARD,   //!< inputs (covered by txid) failed policy rules
    TX_NOT_STANDARD,          //!< otherwise didn't meet our local policy rules
    TX_MISSING_INPUTS,        //!< transaction was missing some of its inputs
    TX_PREMATURE_SPEND,       //!< transaction spends a coinbase too early, or violates locktime/sequence locks
    TX_BAD_SPECIAL,           //!< special transaction violates some rules that are not enough for insta-ban
    /**
     * Tx already in mempool or conflicts with a tx in the chain
     * Currently this is only used if the transaction already exists in the mempool or on chain.
     */
    TX_CONFLICT,
    TX_CONFLICT_LOCK,         //!< conflicts with InstantSend lock
    TX_MEMPOOL_POLICY,        //!< violated mempool's fee/size/descendant/etc limits
};

/** A "reason" why a block was invalid, suitable for determining whether the
  * provider of the block should be banned/ignored/disconnected/etc.
  * These are much more granular than the rejection codes, which may be more
  * useful for some other use-cases.
  */
enum class BlockValidationResult {
    BLOCK_RESULT_UNSET = 0,  //!< initial value. Block has not yet been rejected
    BLOCK_CONSENSUS,         //!< invalid by consensus rules (excluding any below reasons)
    /**
     * Invalid by a change to consensus rules more recent than SegWit.
     * Currently unused as there are no such consensus rule changes, and any download
     * sources realistically need to support SegWit in order to provide useful data,
     * so differentiating between always-invalid and invalid-by-pre-SegWit-soft-fork
     * is uninteresting.
     */
    BLOCK_RECENT_CONSENSUS_CHANGE,
    BLOCK_CACHED_INVALID,    //!< this block was cached as being invalid and we didn't store the reason why
    BLOCK_INVALID_HEADER,    //!< invalid proof of work or time too old
    BLOCK_MUTATED,           //!< the block's data didn't match the data committed to by the PoW
    BLOCK_MISSING_PREV,      //!< We don't have the previous block the checked one is built on
    BLOCK_INVALID_PREV,      //!< A block this one builds on is invalid
    BLOCK_TIME_FUTURE,       //!< block timestamp was > 2 hours in the future (or our clock is bad)
    BLOCK_CHECKPOINT,        //!< the block failed to meet one of our checkpoints
    BLOCK_CHAINLOCK,         //!< the block conflicts with the ChainLock
};



/** Template for capturing information about block/transaction validation. This is instantiated
 *  by TxValidationState and BlockValidationState for validation information on transactions
 *  and blocks respectively. */
template <typename Result>
class ValidationState
{
private:
    enum class ModeState {
        M_VALID,   //!< everything ok
        M_INVALID, //!< network rule violation (DoS value may be set)
        M_ERROR,   //!< run-time error
    } m_mode{ModeState::M_VALID};
    Result m_result{};
    std::string m_reject_reason;
    std::string m_debug_message;

public:
    bool Invalid(Result result,
                 const std::string& reject_reason = "",
                 const std::string& debug_message = "")
    {
        m_result = result;
        m_reject_reason = reject_reason;
        m_debug_message = debug_message;
        if (m_mode != ModeState::M_ERROR) m_mode = ModeState::M_INVALID;
        return false;
    }
    bool Error(const std::string& reject_reason)
    {
        if (m_mode == ModeState::M_VALID)
            m_reject_reason = reject_reason;
        m_mode = ModeState::M_ERROR;
        return false;
    }
    bool IsValid() const { return m_mode == ModeState::M_VALID; }
    bool IsInvalid() const { return m_mode == ModeState::M_INVALID; }
    bool IsError() const { return m_mode == ModeState::M_ERROR; }
    Result GetResult() const { return m_result; }
    std::string GetRejectReason() const { return m_reject_reason; }
    std::string GetDebugMessage() const { return m_debug_message; }
    std::string ToString() const
    {
        if (IsValid()) {
            return "Valid";
        }

        if (!m_debug_message.empty()) {
            return m_reject_reason + ", " + m_debug_message;
        }

        return m_reject_reason;
    }
};

class TxValidationState : public ValidationState<TxValidationResult> {};
class BlockValidationState : public ValidationState<BlockValidationResult> {};

#endif // BITCOIN_CONSENSUS_VALIDATION_H
