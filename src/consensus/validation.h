// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_VALIDATION_H
#define BITCOIN_CONSENSUS_VALIDATION_H

#include <string>
#include "version.h"
#include "consensus/consensus.h"
#include "primitives/transaction.h"
#include "primitives/block.h"

/** "reject" message codes */
static const unsigned char REJECT_MALFORMED = 0x01;
static const unsigned char REJECT_INVALID = 0x10;
static const unsigned char REJECT_OBSOLETE = 0x11;
static const unsigned char REJECT_DUPLICATE = 0x12;
static const unsigned char REJECT_NONSTANDARD = 0x40;
// static const unsigned char REJECT_DUST = 0x41; // part of BIP 61
static const unsigned char REJECT_INSUFFICIENTFEE = 0x42;
static const unsigned char REJECT_CHECKPOINT = 0x43;

/** Reject codes greater or equal to this can be returned by AcceptToMemPool
 * for transactions, to signal internal conditions. They cannot and should not
 * be sent over the P2P network.
 *
 * These error codes are not consensus, but consensus changes should avoid using them
 * unnecessarily so as not to cause needless churn in core-based clients.
 */
static const unsigned int REJECT_INTERNAL = 0x100;
/** Too high fee. Can not be triggered by P2P transactions */
static const unsigned int REJECT_HIGHFEE = 0x100;

enum class DoS_SEVERITY : int {
    NONE = 0,
    LOW = 1,
    MEDIUM = 10,
    ELEVATED = 20,
    HIGH = 50,
    CRITICAL = 100,
};
inline int ToBanScore(const DoS_SEVERITY x) {
    return (int) x;
}
inline bool operator>(const DoS_SEVERITY lhs, const DoS_SEVERITY rhs) {
    return ((int) lhs) > ((int) rhs);
}
enum class CORRUPTION_POSSIBLE : bool {
    False = 0,
    True = 1
};
/** Capture information about block/transaction validation */
class CValidationState {
private:
    enum mode_state {
        MODE_VALID,   //!< everything ok
        MODE_INVALID, //!< network rule violation (DoS value may be set)
        MODE_ERROR,   //!< run-time error
    } mode;
    DoS_SEVERITY nDoS;
    std::string strRejectReason;
    unsigned int chRejectCode;
    CORRUPTION_POSSIBLE corruptionPossible;
    std::string strDebugMessage;
    void DoS(DoS_SEVERITY level,
             unsigned int chRejectCodeIn=0, const std::string &strRejectReasonIn="",
             CORRUPTION_POSSIBLE corruptionIn=CORRUPTION_POSSIBLE::False,
             const std::string &strDebugMessageIn="") {
        chRejectCode = chRejectCodeIn;
        strRejectReason = strRejectReasonIn;
        corruptionPossible = corruptionIn;
        strDebugMessage = strDebugMessageIn;
        if (mode == MODE_ERROR)
            return;
        nDoS = (DoS_SEVERITY) (((unsigned int) nDoS) + ((unsigned int) level));
        mode = MODE_INVALID;
    }
public:
    CValidationState() : mode(MODE_VALID), nDoS(DoS_SEVERITY::NONE), chRejectCode(0), corruptionPossible(CORRUPTION_POSSIBLE::False) {}
    void BadBlockHeader(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL, unsigned int _chRejectCode=REJECT_INVALID) {
        DoS(level, _chRejectCode, _strRejectReason, CORRUPTION_POSSIBLE::False, _strDebugMessage);
    }
    void CorruptBlockHeader(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL) {
        DoS(level, REJECT_INVALID, _strRejectReason, CORRUPTION_POSSIBLE::True, _strDebugMessage);
    }
    void ForkingBlockHeaderDisallowed() {
        DoS(DoS_SEVERITY::CRITICAL, REJECT_CHECKPOINT, "bad-fork-prior-to-checkpoint");
    }
    void BadBlock(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL) {
        DoS(level, REJECT_INVALID, _strRejectReason, CORRUPTION_POSSIBLE::False, _strDebugMessage);
    }
    void CorruptBlock(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL) {
        DoS(level, REJECT_INVALID, _strRejectReason, CORRUPTION_POSSIBLE::True, _strDebugMessage);
    }
    void BadTx(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL, unsigned int _chRejectCode=REJECT_INVALID) {
        DoS(level, _chRejectCode, _strRejectReason, CORRUPTION_POSSIBLE::False, _strDebugMessage);
    }
    void CorruptTx(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", DoS_SEVERITY level=DoS_SEVERITY::CRITICAL) {
        DoS(level, REJECT_INVALID, _strRejectReason, CORRUPTION_POSSIBLE::True, _strDebugMessage);
    }
    void NonStandardTx(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="", CORRUPTION_POSSIBLE corrupted=CORRUPTION_POSSIBLE::False,
                 DoS_SEVERITY level=DoS_SEVERITY::NONE) {
        DoS(level, REJECT_NONSTANDARD, _strRejectReason, corrupted, _strDebugMessage);
    }
    void DuplicateData(const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="") {
        DoS(DoS_SEVERITY::NONE, REJECT_DUPLICATE, _strRejectReason, CORRUPTION_POSSIBLE::False, _strDebugMessage);
    }
    void RejectFee(unsigned int _chRejectCode, const std::string &_strRejectReason,
                 const std::string &_strDebugMessage="") {
        assert(_chRejectCode == REJECT_INSUFFICIENTFEE || _chRejectCode == REJECT_HIGHFEE);
        DoS(DoS_SEVERITY::NONE, _chRejectCode, _strRejectReason, CORRUPTION_POSSIBLE::False, _strDebugMessage);
    }
    void Error(const std::string& strRejectReasonIn) {
        if (mode == MODE_VALID)
            strRejectReason = strRejectReasonIn;
        mode = MODE_ERROR;
    }
    bool IsValid() const {
        return mode == MODE_VALID;
    }
    bool IsInvalid() const {
        return mode == MODE_INVALID;
    }
    bool IsError() const {
        return mode == MODE_ERROR;
    }
    bool IsInvalid(DoS_SEVERITY &nDoSOut) const {
        if (IsInvalid()) {
            nDoSOut = nDoS;
            return true;
        }
        return false;
    }
    bool CorruptionPossible() const {
        return corruptionPossible != CORRUPTION_POSSIBLE::False;
    }
    void SetCorruptionPossible() {
        corruptionPossible = CORRUPTION_POSSIBLE::True;
    }
    unsigned int GetRejectCode() const { return chRejectCode; }
    std::string GetRejectReason() const { return strRejectReason; }
    std::string GetDebugMessage() const { return strDebugMessage; }
    void SetDebugMessage(const std::string& msg){ strDebugMessage = msg; }
};

// These implement the weight = (stripped_size * 4) + witness_size formula,
// using only serialization with and without witness data. As witness_size
// is equal to total_size - stripped_size, this formula is identical to:
// weight = (stripped_size * 3) + total_size.
static inline int64_t GetTransactionWeight(const CTransaction& tx)
{
    return ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}
static inline int64_t GetBlockWeight(const CBlock& block)
{
    return ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
}

#endif // BITCOIN_CONSENSUS_VALIDATION_H
