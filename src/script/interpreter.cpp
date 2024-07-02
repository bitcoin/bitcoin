// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/interpreter.h>

#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>
#include <uint256.h>

typedef std::vector<unsigned char> valtype;

namespace {

inline bool set_success(ScriptError* ret)
{
    if (ret)
        *ret = SCRIPT_ERR_OK;
    return true;
}

inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}

} // namespace

bool CastToBool(const valtype& vch)
{
    for (unsigned int i = 0; i < vch.size(); i++)
    {
        if (vch[i] != 0)
        {
            // Can be negative zero
            if (i == vch.size()-1 && vch[i] == 0x80)
                return false;
            return true;
        }
    }
    return false;
}

/**
 * Script is a stack machine (like Forth) that evaluates a predicate
 * returning a bool indicating valid or not.  There are no loops.
 */
#define stacktop(i)  (stack.at(stack.size()+(i)))
#define altstacktop(i)  (altstack.at(altstack.size()+(i)))
static inline void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

bool static IsCompressedOrUncompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() < CPubKey::COMPRESSED_SIZE) {
        //  Non-canonical public key: too short
        return false;
    }
    if (vchPubKey[0] == 0x04) {
        if (vchPubKey.size() != CPubKey::SIZE) {
            //  Non-canonical public key: invalid length for uncompressed key
            return false;
        }
    } else if (vchPubKey[0] == 0x02 || vchPubKey[0] == 0x03) {
        if (vchPubKey.size() != CPubKey::COMPRESSED_SIZE) {
            //  Non-canonical public key: invalid length for compressed key
            return false;
        }
    } else {
        //  Non-canonical public key: neither compressed nor uncompressed
        return false;
    }
    return true;
}

bool static IsCompressedPubKey(const valtype &vchPubKey) {
    if (vchPubKey.size() != CPubKey::COMPRESSED_SIZE) {
        //  Non-canonical public key: invalid length for compressed key
        return false;
    }
    if (vchPubKey[0] != 0x02 && vchPubKey[0] != 0x03) {
        //  Non-canonical public key: invalid prefix for compressed key
        return false;
    }
    return true;
}

/**
 * A canonical signature exists of: <30> <total len> <02> <len R> <R> <02> <len S> <S> <hashtype>
 * Where R and S are not negative (their first byte has its highest bit not set), and not
 * excessively padded (do not start with a 0 byte, unless an otherwise negative number follows,
 * in which case a single 0 byte is necessary and even required).
 *
 * See https://bitcointalk.org/index.php?topic=8392.msg127623#msg127623
 *
 * This function is consensus-critical since BIP66.
 */
bool static IsValidSignatureEncoding(const std::vector<unsigned char> &sig) {
    // Format: 0x30 [total-length] 0x02 [R-length] [R] 0x02 [S-length] [S] [sighash]
    // * total-length: 1-byte length descriptor of everything that follows,
    //   excluding the sighash byte.
    // * R-length: 1-byte length descriptor of the R value that follows.
    // * R: arbitrary-length big-endian encoded R value. It must use the shortest
    //   possible encoding for a positive integer (which means no null bytes at
    //   the start, except a single one when the next byte has its highest bit set).
    // * S-length: 1-byte length descriptor of the S value that follows.
    // * S: arbitrary-length big-endian encoded S value. The same rules apply.
    // * sighash: 1-byte value indicating what data is hashed (not part of the DER
    //   signature)

    // Minimum and maximum size constraints.
    if (sig.size() < 9) return false;
    if (sig.size() > 73) return false;

    // A signature is of type 0x30 (compound).
    if (sig[0] != 0x30) return false;

    // Make sure the length covers the entire signature.
    if (sig[1] != sig.size() - 3) return false;

    // Extract the length of the R element.
    unsigned int lenR = sig[3];

    // Make sure the length of the S element is still inside the signature.
    if (5 + lenR >= sig.size()) return false;

    // Extract the length of the S element.
    unsigned int lenS = sig[5 + lenR];

    // Verify that the length of the signature matches the sum of the length
    // of the elements.
    if ((size_t)(lenR + lenS + 7) != sig.size()) return false;

    // Check whether the R element is an integer.
    if (sig[2] != 0x02) return false;

    // Zero-length integers are not allowed for R.
    if (lenR == 0) return false;

    // Negative numbers are not allowed for R.
    if (sig[4] & 0x80) return false;

    // Null bytes at the start of R are not allowed, unless R would
    // otherwise be interpreted as a negative number.
    if (lenR > 1 && (sig[4] == 0x00) && !(sig[5] & 0x80)) return false;

    // Check whether the S element is an integer.
    if (sig[lenR + 4] != 0x02) return false;

    // Zero-length integers are not allowed for S.
    if (lenS == 0) return false;

    // Negative numbers are not allowed for S.
    if (sig[lenR + 6] & 0x80) return false;

    // Null bytes at the start of S are not allowed, unless S would otherwise be
    // interpreted as a negative number.
    if (lenS > 1 && (sig[lenR + 6] == 0x00) && !(sig[lenR + 7] & 0x80)) return false;

    return true;
}

bool static IsLowDERSignature(const valtype &vchSig, ScriptError* serror) {
    if (!IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    }
    // https://bitcoin.stackexchange.com/a/12556:
    //     Also note that inside transaction signatures, an extra hashtype byte
    //     follows the actual signature data.
    std::vector<unsigned char> vchSigCopy(vchSig.begin(), vchSig.begin() + vchSig.size() - 1);
    // If the S value is above the order of the curve divided by two, its
    // complement modulo the order could have been used instead, which is
    // one byte shorter when encoded correctly.
    if (!CPubKey::CheckLowS(vchSigCopy)) {
        return set_error(serror, SCRIPT_ERR_SIG_HIGH_S);
    }
    return true;
}

bool static IsDefinedHashtypeSignature(const valtype &vchSig) {
    if (vchSig.size() == 0) {
        return false;
    }
    unsigned char nHashType = vchSig[vchSig.size() - 1] & (~(SIGHASH_ANYONECANPAY));
    if (nHashType < SIGHASH_ALL || nHashType > SIGHASH_SINGLE)
        return false;

    return true;
}

bool CheckSignatureEncoding(const std::vector<unsigned char> &vchSig, unsigned int flags, ScriptError* serror) {
    // Empty signature. Not strictly DER encoded, but allowed to provide a
    // compact way to provide an invalid signature for use with CHECK(MULTI)SIG
    if (vchSig.size() == 0) {
        return true;
    }
    if ((flags & (SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_LOW_S | SCRIPT_VERIFY_STRICTENC)) != 0 && !IsValidSignatureEncoding(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_DER);
    } else if ((flags & SCRIPT_VERIFY_LOW_S) != 0 && !IsLowDERSignature(vchSig, serror)) {
        // serror is set
        return false;
    } else if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsDefinedHashtypeSignature(vchSig)) {
        return set_error(serror, SCRIPT_ERR_SIG_HASHTYPE);
    }
    return true;
}

bool static CheckPubKeyEncoding(const valtype &vchPubKey, unsigned int flags, const SigVersion &sigversion, ScriptError* serror) {
    if ((flags & SCRIPT_VERIFY_STRICTENC) != 0 && !IsCompressedOrUncompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    }
    // Only compressed keys are accepted in segwit
    if ((flags & SCRIPT_VERIFY_WITNESS_PUBKEYTYPE) != 0 && sigversion == SigVersion::WITNESS_V0 && !IsCompressedPubKey(vchPubKey)) {
        return set_error(serror, SCRIPT_ERR_WITNESS_PUBKEYTYPE);
    }
    return true;
}

int FindAndDelete(CScript& script, const CScript& b)
{
    int nFound = 0;
    if (b.empty())
        return nFound;
    CScript result;
    CScript::const_iterator pc = script.begin(), pc2 = script.begin(), end = script.end();
    opcodetype opcode;
    do
    {
        result.insert(result.end(), pc2, pc);
        while (static_cast<size_t>(end - pc) >= b.size() && std::equal(b.begin(), b.end(), pc))
        {
            pc = pc + b.size();
            ++nFound;
        }
        pc2 = pc;
    }
    while (script.GetOp(pc, opcode));

    if (nFound > 0) {
        result.insert(result.end(), pc2, end);
        script = std::move(result);
    }

    return nFound;
}

namespace {
/** A data type to abstract out the condition stack during script execution.
 *
 * Conceptually it acts like a vector of booleans, one for each level of nested
 * IF/THEN/ELSE, indicating whether we're in the active or inactive branch of
 * each.
 *
 * The elements on the stack cannot be observed individually; we only need to
 * expose whether the stack is empty and whether or not any false values are
 * present at all. To implement OP_ELSE, a toggle_top modifier is added, which
 * flips the last value without returning it.
 *
 * This uses an optimized implementation that does not materialize the
 * actual stack. Instead, it just stores the size of the would-be stack,
 * and the position of the first false value in it.
 */
class ConditionStack {
private:
    //! A constant for m_first_false_pos to indicate there are no falses.
    static constexpr uint32_t NO_FALSE = std::numeric_limits<uint32_t>::max();

    //! The size of the implied stack.
    uint32_t m_stack_size = 0;
    //! The position of the first false value on the implied stack, or NO_FALSE if all true.
    uint32_t m_first_false_pos = NO_FALSE;

public:
    bool empty() const { return m_stack_size == 0; }
    bool all_true() const { return m_first_false_pos == NO_FALSE; }
    void push_back(bool f)
    {
        if (m_first_false_pos == NO_FALSE && !f) {
            // The stack consists of all true values, and a false is added.
            // The first false value will appear at the current size.
            m_first_false_pos = m_stack_size;
        }
        ++m_stack_size;
    }
    void pop_back()
    {
        assert(m_stack_size > 0);
        --m_stack_size;
        if (m_first_false_pos == m_stack_size) {
            // When popping off the first false value, everything becomes true.
            m_first_false_pos = NO_FALSE;
        }
    }
    void toggle_top()
    {
        assert(m_stack_size > 0);
        if (m_first_false_pos == NO_FALSE) {
            // The current stack is all true values; the first false will be the top.
            m_first_false_pos = m_stack_size - 1;
        } else if (m_first_false_pos == m_stack_size - 1) {
            // The top is the first false value; toggling it will make everything true.
            m_first_false_pos = NO_FALSE;
        } else {
            // There is a false value, but not on top. No action is needed as toggling
            // anything but the first false value is unobservable.
        }
    }
};
}

static bool EvalChecksigPreTapscript(const valtype& vchSig, const valtype& vchPubKey, CScript::const_iterator pbegincodehash, CScript::const_iterator pend, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& fSuccess)
{
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0);

    // Subset of script starting at the most recent codeseparator
    CScript scriptCode(pbegincodehash, pend);

    // Drop the signature in pre-segwit scripts but not segwit scripts
    if (sigversion == SigVersion::BASE) {
        int found = FindAndDelete(scriptCode, CScript() << vchSig);
        if (found > 0 && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
            return set_error(serror, SCRIPT_ERR_SIG_FINDANDDELETE);
    }

    if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
        //serror is set
        return false;
    }
    fSuccess = checker.CheckECDSASignature(vchSig, vchPubKey, scriptCode, sigversion);

    if (!fSuccess && (flags & SCRIPT_VERIFY_NULLFAIL) && vchSig.size())
        return set_error(serror, SCRIPT_ERR_SIG_NULLFAIL);

    return true;
}

static bool EvalChecksigFromStack(const valtype& sig, const valtype& msg, const valtype& pubkey_in, ScriptExecutionData& execdata, unsigned int flags, SigVersion sigversion, ScriptError* serror, bool& success)
{
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0 || sigversion == SigVersion::TAPSCRIPT);
    /*
     *  The following validation sequence is consensus critical. Please note how --
     *    upgradable public key versions precede other rules;
     *    the script execution fails when using empty signature with invalid public key;
     *    the script execution fails when using non-empty invalid signature.
     */
    success = !sig.empty();
    if (success && sigversion == SigVersion::TAPSCRIPT) {
        // Implement the sigops/witnesssize ratio test.
        // Passing with an upgradable public key version is also counted.
        assert(execdata.m_validation_weight_left_init);
        execdata.m_validation_weight_left -= VALIDATION_WEIGHT_PER_SIGOP_PASSED;
        if (execdata.m_validation_weight_left < 0) {
            return set_error(serror, SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT);
        }
    }
    if (pubkey_in.size() == 0) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    } else if (pubkey_in.size() == 32) {
        if (!success) return true;
        if (sig.size() != 64) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_SIZE);

        XOnlyPubKey pubkey{pubkey_in};

        if (!pubkey.VerifySchnorr(msg, sig)) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG);
    } else {
        /*
         *  New public key version softforks should be defined before this `else` block.
         *  Generally, the new code should not do anything but failing the script execution. To avoid
         *  consensus bugs, it should not modify any existing values (including `success`).
         */
        if ((flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE) != 0) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE);
        }
    }

    return true;
}

static bool EvalChecksigTapscript(const valtype& sig, const valtype& pubkey, ScriptExecutionData& execdata, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& success)
{
    assert(sigversion == SigVersion::TAPSCRIPT);

    /*
     *  The following validation sequence is consensus critical. Please note how --
     *    upgradable public key versions precede other rules;
     *    the script execution fails when using empty signature with invalid public key;
     *    the script execution fails when using non-empty invalid signature.
     */
    success = !sig.empty();
    if (success) {
        // Implement the sigops/witnesssize ratio test.
        // Passing with an upgradable public key version is also counted.
        assert(execdata.m_validation_weight_left_init);
        execdata.m_validation_weight_left -= VALIDATION_WEIGHT_PER_SIGOP_PASSED;
        if (execdata.m_validation_weight_left < 0) {
            return set_error(serror, SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT);
        }
    }
    if (pubkey.size() == 0) {
        return set_error(serror, SCRIPT_ERR_PUBKEYTYPE);
    } else if (pubkey.size() == 32) {
        if (success && !checker.CheckSchnorrSignature(sig, pubkey, sigversion, execdata, serror)) {
            return false; // serror is set
        }
    } else {
        /*
         *  New public key version softforks should be defined before this `else` block.
         *  Generally, the new code should not do anything but failing the script execution. To avoid
         *  consensus bugs, it should not modify any existing values (including `success`).
         */
        if ((flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_PUBKEYTYPE) != 0) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE);
        }
    }

    return true;
}

/** Helper for OP_CHECKSIG, OP_CHECKSIGVERIFY, and (in Tapscript) OP_CHECKSIGADD.
 *
 * A return value of false means the script fails entirely. When true is returned, the
 * success variable indicates whether the signature check itself succeeded.
 */
static bool EvalChecksig(const valtype& sig, const valtype& pubkey, CScript::const_iterator pbegincodehash, CScript::const_iterator pend, ScriptExecutionData& execdata, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror, bool& success)
{
    switch (sigversion) {
    case SigVersion::BASE:
    case SigVersion::WITNESS_V0:
        return EvalChecksigPreTapscript(sig, pubkey, pbegincodehash, pend, flags, checker, sigversion, serror, success);
    case SigVersion::TAPSCRIPT:
        return EvalChecksigTapscript(sig, pubkey, execdata, flags, checker, sigversion, serror, success);
    case SigVersion::TAPROOT:
        // Key path spending in Taproot has no script, so this is unreachable.
        break;
    }
    assert(false);
}

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror)
{
    static const CScriptNum bnZero(0);
    static const CScriptNum bnOne(1);
    // static const CScriptNum bnFalse(0);
    // static const CScriptNum bnTrue(1);
    static const valtype vchFalse(0);
    // static const valtype vchZero(0);
    static const valtype vchTrue(1, 1);

    // sigversion cannot be TAPROOT here, as it admits no script execution.
    assert(sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0 || sigversion == SigVersion::TAPSCRIPT);

    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    CScript::const_iterator pbegincodehash = script.begin();
    opcodetype opcode;
    valtype vchPushValue;
    ConditionStack vfExec;
    std::vector<valtype> altstack;
    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    if ((sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) && script.size() > MAX_SCRIPT_SIZE) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }
    int nOpCount = 0;
    bool fRequireMinimal = (flags & SCRIPT_VERIFY_MINIMALDATA) != 0;
    uint32_t opcode_pos = 0;
    execdata.m_codeseparator_pos = 0xFFFFFFFFUL;
    execdata.m_codeseparator_pos_init = true;

    try
    {
        for (; pc < pend; ++opcode_pos) {
            bool fExec = vfExec.all_true();

            //
            // Read instruction
            //
            if (!script.GetOp(pc, opcode, vchPushValue))
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            if (vchPushValue.size() > MAX_SCRIPT_ELEMENT_SIZE)
                return set_error(serror, SCRIPT_ERR_PUSH_SIZE);

            if (sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) {
                // Note how OP_RESERVED does not count towards the opcode limit.
                if (opcode > OP_16 && ++nOpCount > MAX_OPS_PER_SCRIPT) {
                    return set_error(serror, SCRIPT_ERR_OP_COUNT);
                }
            }

            if (opcode == OP_CAT ||
                opcode == OP_SUBSTR ||
                opcode == OP_LEFT ||
                opcode == OP_RIGHT ||
                opcode == OP_INVERT ||
                opcode == OP_AND ||
                opcode == OP_OR ||
                opcode == OP_XOR ||
                opcode == OP_2MUL ||
                opcode == OP_2DIV ||
                opcode == OP_MUL ||
                opcode == OP_DIV ||
                opcode == OP_MOD ||
                opcode == OP_LSHIFT ||
                opcode == OP_RSHIFT)
                return set_error(serror, SCRIPT_ERR_DISABLED_OPCODE); // Disabled opcodes (CVE-2010-5137).

            // With SCRIPT_VERIFY_CONST_SCRIPTCODE, OP_CODESEPARATOR in non-segwit script is rejected even in an unexecuted branch
            if (opcode == OP_CODESEPARATOR && sigversion == SigVersion::BASE && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
                return set_error(serror, SCRIPT_ERR_OP_CODESEPARATOR);

            if (fExec && 0 <= opcode && opcode <= OP_PUSHDATA4) {
                if (fRequireMinimal && !CheckMinimalPush(vchPushValue, opcode)) {
                    return set_error(serror, SCRIPT_ERR_MINIMALDATA);
                }
                stack.push_back(vchPushValue);
            } else if (fExec || (OP_IF <= opcode && opcode <= OP_ENDIF))
            switch (opcode)
            {
                //
                // Push value
                //
                case OP_1NEGATE:
                case OP_1:
                case OP_2:
                case OP_3:
                case OP_4:
                case OP_5:
                case OP_6:
                case OP_7:
                case OP_8:
                case OP_9:
                case OP_10:
                case OP_11:
                case OP_12:
                case OP_13:
                case OP_14:
                case OP_15:
                case OP_16:
                {
                    // ( -- value)
                    CScriptNum bn((int)opcode - (int)(OP_1 - 1));
                    stack.push_back(bn.getvch());
                    // The result of these opcodes should always be the minimal way to push the data
                    // they push, so no need for a CheckMinimalPush here.
                }
                break;


                //
                // Control
                //
                case OP_NOP:
                    break;

                case OP_CHECKLOCKTIMEVERIFY:
                {
                    if (!(flags & SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY)) {
                        // not enabled; treat as a NOP2
                        break;
                    }

                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // Note that elsewhere numeric opcodes are limited to
                    // operands in the range -2**31+1 to 2**31-1, however it is
                    // legal for opcodes to produce results exceeding that
                    // range. This limitation is implemented by CScriptNum's
                    // default 4-byte limit.
                    //
                    // If we kept to that limit we'd have a year 2038 problem,
                    // even though the nLockTime field in transactions
                    // themselves is uint32 which only becomes meaningless
                    // after the year 2106.
                    //
                    // Thus as a special case we tell CScriptNum to accept up
                    // to 5-byte bignums, which are good until 2**39-1, well
                    // beyond the 2**32-1 limit of the nLockTime field itself.
                    const CScriptNum nLockTime(stacktop(-1), fRequireMinimal, 5);

                    // In the rare event that the argument may be < 0 due to
                    // some arithmetic being done first, you can always use
                    // 0 MAX CHECKLOCKTIMEVERIFY.
                    if (nLockTime < 0)
                        return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);

                    // Actually compare the specified lock time with the transaction.
                    if (!checker.CheckLockTime(nLockTime))
                        return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);

                    break;
                }

                case OP_CHECKSEQUENCEVERIFY:
                {
                    if (!(flags & SCRIPT_VERIFY_CHECKSEQUENCEVERIFY)) {
                        // not enabled; treat as a NOP3
                        break;
                    }

                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // nSequence, like nLockTime, is a 32-bit unsigned integer
                    // field. See the comment in CHECKLOCKTIMEVERIFY regarding
                    // 5-byte numeric operands.
                    const CScriptNum nSequence(stacktop(-1), fRequireMinimal, 5);

                    // In the rare event that the argument may be < 0 due to
                    // some arithmetic being done first, you can always use
                    // 0 MAX CHECKSEQUENCEVERIFY.
                    if (nSequence < 0)
                        return set_error(serror, SCRIPT_ERR_NEGATIVE_LOCKTIME);

                    // To provide for future soft-fork extensibility, if the
                    // operand has the disabled lock-time flag set,
                    // CHECKSEQUENCEVERIFY behaves as a NOP.
                    if ((nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) != 0)
                        break;

                    // Compare the specified sequence number with the input.
                    if (!checker.CheckSequence(nSequence))
                        return set_error(serror, SCRIPT_ERR_UNSATISFIED_LOCKTIME);

                    break;
                }

                case OP_NOP1: case OP_NOP4:
                case OP_NOP6: case OP_NOP7: case OP_NOP8: case OP_NOP9: case OP_NOP10:
                {
                    if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_NOPS)
                        return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                }
                break;

                case OP_IF:
                case OP_NOTIF:
                {
                    // <expression> if [statements] [else [statements]] endif
                    bool fValue = false;
                    if (fExec)
                    {
                        if (stack.size() < 1)
                            return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                        valtype& vch = stacktop(-1);
                        // Tapscript requires minimal IF/NOTIF inputs as a consensus rule.
                        if (sigversion == SigVersion::TAPSCRIPT) {
                            // The input argument to the OP_IF and OP_NOTIF opcodes must be either
                            // exactly 0 (the empty vector) or exactly 1 (the one-byte vector with value 1).
                            if (vch.size() > 1 || (vch.size() == 1 && vch[0] != 1)) {
                                return set_error(serror, SCRIPT_ERR_TAPSCRIPT_MINIMALIF);
                            }
                        }
                        // Under witness v0 rules it is only a policy rule, enabled through SCRIPT_VERIFY_MINIMALIF.
                        if (sigversion == SigVersion::WITNESS_V0 && (flags & SCRIPT_VERIFY_MINIMALIF)) {
                            if (vch.size() > 1)
                                return set_error(serror, SCRIPT_ERR_MINIMALIF);
                            if (vch.size() == 1 && vch[0] != 1)
                                return set_error(serror, SCRIPT_ERR_MINIMALIF);
                        }
                        fValue = CastToBool(vch);
                        if (opcode == OP_NOTIF)
                            fValue = !fValue;
                        popstack(stack);
                    }
                    vfExec.push_back(fValue);
                }
                break;

                case OP_ELSE:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.toggle_top();
                }
                break;

                case OP_ENDIF:
                {
                    if (vfExec.empty())
                        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);
                    vfExec.pop_back();
                }
                break;

                case OP_VERIFY:
                {
                    // (true -- ) or
                    // (false -- false) and return
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    bool fValue = CastToBool(stacktop(-1));
                    if (fValue)
                        popstack(stack);
                    else
                        return set_error(serror, SCRIPT_ERR_VERIFY);
                }
                break;

                case OP_RETURN:
                {
                    return set_error(serror, SCRIPT_ERR_OP_RETURN);
                }
                break;


                //
                // Stack ops
                //
                case OP_TOALTSTACK:
                {
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    altstack.push_back(stacktop(-1));
                    popstack(stack);
                }
                break;

                case OP_FROMALTSTACK:
                {
                    if (altstack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_ALTSTACK_OPERATION);
                    stack.push_back(altstacktop(-1));
                    popstack(altstack);
                }
                break;

                case OP_2DROP:
                {
                    // (x1 x2 -- )
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                    popstack(stack);
                }
                break;

                case OP_2DUP:
                {
                    // (x1 x2 -- x1 x2 x1 x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-2);
                    valtype vch2 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_3DUP:
                {
                    // (x1 x2 x3 -- x1 x2 x3 x1 x2 x3)
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-3);
                    valtype vch2 = stacktop(-2);
                    valtype vch3 = stacktop(-1);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                    stack.push_back(vch3);
                }
                break;

                case OP_2OVER:
                {
                    // (x1 x2 x3 x4 -- x1 x2 x3 x4 x1 x2)
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-4);
                    valtype vch2 = stacktop(-3);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_2ROT:
                {
                    // (x1 x2 x3 x4 x5 x6 -- x3 x4 x5 x6 x1 x2)
                    if (stack.size() < 6)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch1 = stacktop(-6);
                    valtype vch2 = stacktop(-5);
                    stack.erase(stack.end()-6, stack.end()-4);
                    stack.push_back(vch1);
                    stack.push_back(vch2);
                }
                break;

                case OP_2SWAP:
                {
                    // (x1 x2 x3 x4 -- x3 x4 x1 x2)
                    if (stack.size() < 4)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-4), stacktop(-2));
                    swap(stacktop(-3), stacktop(-1));
                }
                break;

                case OP_IFDUP:
                {
                    // (x - 0 | x x)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    if (CastToBool(vch))
                        stack.push_back(vch);
                }
                break;

                case OP_DEPTH:
                {
                    // -- stacksize
                    CScriptNum bn(stack.size());
                    stack.push_back(bn.getvch());
                }
                break;

                case OP_DROP:
                {
                    // (x -- )
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    popstack(stack);
                }
                break;

                case OP_DUP:
                {
                    // (x -- x x)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.push_back(vch);
                }
                break;

                case OP_NIP:
                {
                    // (x1 x2 -- x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    stack.erase(stack.end() - 2);
                }
                break;

                case OP_OVER:
                {
                    // (x1 x2 -- x1 x2 x1)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-2);
                    stack.push_back(vch);
                }
                break;

                case OP_PICK:
                case OP_ROLL:
                {
                    // (xn ... x2 x1 x0 n - xn ... x2 x1 x0 xn)
                    // (xn ... x2 x1 x0 n - ... x2 x1 x0 xn)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    int n = CScriptNum(stacktop(-1), fRequireMinimal).getint();
                    popstack(stack);
                    if (n < 0 || n >= (int)stack.size())
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-n-1);
                    if (opcode == OP_ROLL)
                        stack.erase(stack.end()-n-1);
                    stack.push_back(vch);
                }
                break;

                case OP_ROT:
                {
                    // (x1 x2 x3 -- x2 x3 x1)
                    //  x2 x1 x3  after first swap
                    //  x2 x3 x1  after second swap
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-3), stacktop(-2));
                    swap(stacktop(-2), stacktop(-1));
                }
                break;

                case OP_SWAP:
                {
                    // (x1 x2 -- x2 x1)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    swap(stacktop(-2), stacktop(-1));
                }
                break;

                case OP_TUCK:
                {
                    // (x1 x2 -- x2 x1 x2)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype vch = stacktop(-1);
                    stack.insert(stack.end()-2, vch);
                }
                break;


                case OP_SIZE:
                {
                    // (in -- in size)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1).size());
                    stack.push_back(bn.getvch());
                }
                break;


                //
                // Bitwise logic
                //
                case OP_EQUAL:
                case OP_EQUALVERIFY:
                //case OP_NOTEQUAL: // use OP_NUMNOTEQUAL
                {
                    // (x1 x2 - bool)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch1 = stacktop(-2);
                    valtype& vch2 = stacktop(-1);
                    bool fEqual = (vch1 == vch2);
                    // OP_NOTEQUAL is disabled because it would be too easy to say
                    // something like n != 1 and have some wiseguy pass in 1 with extra
                    // zero bytes after it (numerically, 0x01 == 0x0001 == 0x000001)
                    //if (opcode == OP_NOTEQUAL)
                    //    fEqual = !fEqual;
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fEqual ? vchTrue : vchFalse);
                    if (opcode == OP_EQUALVERIFY)
                    {
                        if (fEqual)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_EQUALVERIFY);
                    }
                }
                break;


                //
                // Numeric
                //
                case OP_1ADD:
                case OP_1SUB:
                case OP_NEGATE:
                case OP_ABS:
                case OP_NOT:
                case OP_0NOTEQUAL:
                {
                    // (in -- out)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn(stacktop(-1), fRequireMinimal);
                    switch (opcode)
                    {
                    case OP_1ADD:       bn += bnOne; break;
                    case OP_1SUB:       bn -= bnOne; break;
                    case OP_NEGATE:     bn = -bn; break;
                    case OP_ABS:        if (bn < bnZero) bn = -bn; break;
                    case OP_NOT:        bn = (bn == bnZero); break;
                    case OP_0NOTEQUAL:  bn = (bn != bnZero); break;
                    default:            assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    stack.push_back(bn.getvch());
                }
                break;

                case OP_ADD:
                case OP_SUB:
                case OP_BOOLAND:
                case OP_BOOLOR:
                case OP_NUMEQUAL:
                case OP_NUMEQUALVERIFY:
                case OP_NUMNOTEQUAL:
                case OP_LESSTHAN:
                case OP_GREATERTHAN:
                case OP_LESSTHANOREQUAL:
                case OP_GREATERTHANOREQUAL:
                case OP_MIN:
                case OP_MAX:
                {
                    // (x1 x2 -- out)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-2), fRequireMinimal);
                    CScriptNum bn2(stacktop(-1), fRequireMinimal);
                    CScriptNum bn(0);
                    switch (opcode)
                    {
                    case OP_ADD:
                        bn = bn1 + bn2;
                        break;

                    case OP_SUB:
                        bn = bn1 - bn2;
                        break;

                    case OP_BOOLAND:             bn = (bn1 != bnZero && bn2 != bnZero); break;
                    case OP_BOOLOR:              bn = (bn1 != bnZero || bn2 != bnZero); break;
                    case OP_NUMEQUAL:            bn = (bn1 == bn2); break;
                    case OP_NUMEQUALVERIFY:      bn = (bn1 == bn2); break;
                    case OP_NUMNOTEQUAL:         bn = (bn1 != bn2); break;
                    case OP_LESSTHAN:            bn = (bn1 < bn2); break;
                    case OP_GREATERTHAN:         bn = (bn1 > bn2); break;
                    case OP_LESSTHANOREQUAL:     bn = (bn1 <= bn2); break;
                    case OP_GREATERTHANOREQUAL:  bn = (bn1 >= bn2); break;
                    case OP_MIN:                 bn = (bn1 < bn2 ? bn1 : bn2); break;
                    case OP_MAX:                 bn = (bn1 > bn2 ? bn1 : bn2); break;
                    default:                     assert(!"invalid opcode"); break;
                    }
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(bn.getvch());

                    if (opcode == OP_NUMEQUALVERIFY)
                    {
                        if (CastToBool(stacktop(-1)))
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_NUMEQUALVERIFY);
                    }
                }
                break;

                case OP_WITHIN:
                {
                    // (x min max -- out)
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    CScriptNum bn1(stacktop(-3), fRequireMinimal);
                    CScriptNum bn2(stacktop(-2), fRequireMinimal);
                    CScriptNum bn3(stacktop(-1), fRequireMinimal);
                    bool fValue = (bn2 <= bn1 && bn1 < bn3);
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fValue ? vchTrue : vchFalse);
                }
                break;


                //
                // Crypto
                //
                case OP_RIPEMD160:
                case OP_SHA1:
                case OP_SHA256:
                case OP_HASH160:
                case OP_HASH256:
                {
                    // (in -- hash)
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    valtype& vch = stacktop(-1);
                    valtype vchHash((opcode == OP_RIPEMD160 || opcode == OP_SHA1 || opcode == OP_HASH160) ? 20 : 32);
                    if (opcode == OP_RIPEMD160)
                        CRIPEMD160().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA1)
                        CSHA1().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_SHA256)
                        CSHA256().Write(vch.data(), vch.size()).Finalize(vchHash.data());
                    else if (opcode == OP_HASH160)
                        CHash160().Write(vch).Finalize(vchHash);
                    else if (opcode == OP_HASH256)
                        CHash256().Write(vch).Finalize(vchHash);
                    popstack(stack);
                    stack.push_back(vchHash);
                }
                break;

                case OP_CODESEPARATOR:
                {
                    // If SCRIPT_VERIFY_CONST_SCRIPTCODE flag is set, use of OP_CODESEPARATOR is rejected in pre-segwit
                    // script, even in an unexecuted branch (this is checked above the opcode case statement).

                    // Hash starts after the code separator
                    pbegincodehash = pc;
                    execdata.m_codeseparator_pos = opcode_pos;
                }
                break;

                case OP_CHECKSIG:
                case OP_CHECKSIGVERIFY:
                {
                    // (sig pubkey -- bool)
                    if (stack.size() < 2)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    valtype& vchSig    = stacktop(-2);
                    valtype& vchPubKey = stacktop(-1);

                    bool fSuccess = true;
                    if (!EvalChecksig(vchSig, vchPubKey, pbegincodehash, pend, execdata, flags, checker, sigversion, serror, fSuccess)) return false;
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fSuccess ? vchTrue : vchFalse);
                    if (opcode == OP_CHECKSIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKSIGVERIFY);
                    }
                }
                break;

                case OP_CHECKSIGADD:
                {
                    // OP_CHECKSIGADD is only available in Tapscript
                    if (sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0) return set_error(serror, SCRIPT_ERR_BAD_OPCODE);

                    // (sig num pubkey -- num)
                    if (stack.size() < 3) return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    const valtype& sig = stacktop(-3);
                    const CScriptNum num(stacktop(-2), fRequireMinimal);
                    const valtype& pubkey = stacktop(-1);

                    bool success = true;
                    if (!EvalChecksig(sig, pubkey, pbegincodehash, pend, execdata, flags, checker, sigversion, serror, success)) return false;
                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back((num + (success ? 1 : 0)).getvch());
                }
                break;

                case OP_CHECKMULTISIG:
                case OP_CHECKMULTISIGVERIFY:
                {
                    if (sigversion == SigVersion::TAPSCRIPT) return set_error(serror, SCRIPT_ERR_TAPSCRIPT_CHECKMULTISIG);

                    // ([sig ...] num_of_signatures [pubkey ...] num_of_pubkeys -- bool)

                    int i = 1;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    int nKeysCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nKeysCount < 0 || nKeysCount > MAX_PUBKEYS_PER_MULTISIG)
                        return set_error(serror, SCRIPT_ERR_PUBKEY_COUNT);
                    nOpCount += nKeysCount;
                    if (nOpCount > MAX_OPS_PER_SCRIPT)
                        return set_error(serror, SCRIPT_ERR_OP_COUNT);
                    int ikey = ++i;
                    // ikey2 is the position of last non-signature item in the stack. Top stack item = 1.
                    // With SCRIPT_VERIFY_NULLFAIL, this is used for cleanup if operation fails.
                    int ikey2 = nKeysCount + 2;
                    i += nKeysCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    int nSigsCount = CScriptNum(stacktop(-i), fRequireMinimal).getint();
                    if (nSigsCount < 0 || nSigsCount > nKeysCount)
                        return set_error(serror, SCRIPT_ERR_SIG_COUNT);
                    int isig = ++i;
                    i += nSigsCount;
                    if ((int)stack.size() < i)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    // Subset of script starting at the most recent codeseparator
                    CScript scriptCode(pbegincodehash, pend);

                    // Drop the signature in pre-segwit scripts but not segwit scripts
                    for (int k = 0; k < nSigsCount; k++)
                    {
                        valtype& vchSig = stacktop(-isig-k);
                        if (sigversion == SigVersion::BASE) {
                            int found = FindAndDelete(scriptCode, CScript() << vchSig);
                            if (found > 0 && (flags & SCRIPT_VERIFY_CONST_SCRIPTCODE))
                                return set_error(serror, SCRIPT_ERR_SIG_FINDANDDELETE);
                        }
                    }

                    bool fSuccess = true;
                    while (fSuccess && nSigsCount > 0)
                    {
                        valtype& vchSig    = stacktop(-isig);
                        valtype& vchPubKey = stacktop(-ikey);

                        // Note how this makes the exact order of pubkey/signature evaluation
                        // distinguishable by CHECKMULTISIG NOT if the STRICTENC flag is set.
                        // See the script_(in)valid tests for details.
                        if (!CheckSignatureEncoding(vchSig, flags, serror) || !CheckPubKeyEncoding(vchPubKey, flags, sigversion, serror)) {
                            // serror is set
                            return false;
                        }

                        // Check signature
                        bool fOk = checker.CheckECDSASignature(vchSig, vchPubKey, scriptCode, sigversion);

                        if (fOk) {
                            isig++;
                            nSigsCount--;
                        }
                        ikey++;
                        nKeysCount--;

                        // If there are more signatures left than keys left,
                        // then too many signatures have failed. Exit early,
                        // without checking any further signatures.
                        if (nSigsCount > nKeysCount)
                            fSuccess = false;
                    }

                    // Clean up stack of actual arguments
                    while (i-- > 1) {
                        // If the operation failed, we require that all signatures must be empty vector
                        if (!fSuccess && (flags & SCRIPT_VERIFY_NULLFAIL) && !ikey2 && stacktop(-1).size())
                            return set_error(serror, SCRIPT_ERR_SIG_NULLFAIL);
                        if (ikey2 > 0)
                            ikey2--;
                        popstack(stack);
                    }

                    // A bug causes CHECKMULTISIG to consume one extra argument
                    // whose contents were not checked in any way.
                    //
                    // Unfortunately this is a potential source of mutability,
                    // so optionally verify it is exactly equal to zero prior
                    // to removing it from the stack.
                    if (stack.size() < 1)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                    if ((flags & SCRIPT_VERIFY_NULLDUMMY) && stacktop(-1).size())
                        return set_error(serror, SCRIPT_ERR_SIG_NULLDUMMY);
                    popstack(stack);

                    stack.push_back(fSuccess ? vchTrue : vchFalse);

                    if (opcode == OP_CHECKMULTISIGVERIFY)
                    {
                        if (fSuccess)
                            popstack(stack);
                        else
                            return set_error(serror, SCRIPT_ERR_CHECKMULTISIGVERIFY);
                    }
                }
                break;

                case OP_CHECKSIGFROMSTACK:
                case OP_CHECKSIGFROMSTACKVERIFY: {
                    if (opcode == OP_CHECKSIGFROMSTACKVERIFY) {
                        if (flags & SCRIPT_VERIFY_DISCOURAGE_CHECKSIGFROMSTACK)
                            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS);
                        // if flag not enabled; treat OP_CHECKSIGFROMSTACKVERIFY as a NOP5
                        if (!(flags & SCRIPT_VERIFY_CHECKSIGFROMSTACK)) break;
                    }
                    // DISCOURAGE for OP_CHECKSIGFROMSTACK is handled in OP_SUCCESS handling
                    // OP_CHECKSIGFROMSTACK is only available in Tapscript
                    if (opcode == OP_CHECKSIGFROMSTACK && (sigversion == SigVersion::BASE || sigversion == SigVersion::WITNESS_V0)) {
                        return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
                    }

                    // sig message pubkey
                    if (stack.size() < 3)
                        return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);

                    const valtype& vchSigIn = stacktop(-3);
                    const valtype& vchMsg = stacktop(-2);
                    const valtype& vchPubKey = stacktop(-1);

                    bool fSuccess = true;
                    if (!EvalChecksigFromStack(vchSigIn, vchMsg, vchPubKey, execdata, flags, sigversion, serror, fSuccess)) return false;

                    if (opcode == OP_CHECKSIGFROMSTACKVERIFY) {
                        if (!fSuccess) return set_error(serror, SCRIPT_ERR_CHECKSIGVERIFY);
                        break;
                    }

                    popstack(stack);
                    popstack(stack);
                    popstack(stack);
                    stack.push_back(fSuccess ? vchTrue : vchFalse);
                    break;
                }

                default:
                    return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }

            // Size limits
            if (stack.size() + altstack.size() > MAX_STACK_SIZE)
                return set_error(serror, SCRIPT_ERR_STACK_SIZE);
        }
    }
    catch (...)
    {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    if (!vfExec.empty())
        return set_error(serror, SCRIPT_ERR_UNBALANCED_CONDITIONAL);

    return set_success(serror);
}

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, unsigned int flags, const BaseSignatureChecker& checker, SigVersion sigversion, ScriptError* serror)
{
    ScriptExecutionData execdata;
    return EvalScript(stack, script, flags, checker, sigversion, execdata, serror);
}

namespace {

/**
 * Wrapper that serializes like CTransaction, but with the modifications
 *  required for the signature hash done in-place
 */
template <class T>
class CTransactionSignatureSerializer
{
private:
    const T& txTo;             //!< reference to the spending transaction (the one being serialized)
    const CScript& scriptCode; //!< output script being consumed
    const unsigned int nIn;    //!< input index of txTo being signed
    const bool fAnyoneCanPay;  //!< whether the hashtype has the SIGHASH_ANYONECANPAY flag set
    const bool fHashSingle;    //!< whether the hashtype is SIGHASH_SINGLE
    const bool fHashNone;      //!< whether the hashtype is SIGHASH_NONE

public:
    CTransactionSignatureSerializer(const T& txToIn, const CScript& scriptCodeIn, unsigned int nInIn, int nHashTypeIn) :
        txTo(txToIn), scriptCode(scriptCodeIn), nIn(nInIn),
        fAnyoneCanPay(!!(nHashTypeIn & SIGHASH_ANYONECANPAY)),
        fHashSingle((nHashTypeIn & 0x1f) == SIGHASH_SINGLE),
        fHashNone((nHashTypeIn & 0x1f) == SIGHASH_NONE) {}

    /** Serialize the passed scriptCode, skipping OP_CODESEPARATORs */
    template<typename S>
    void SerializeScriptCode(S &s) const {
        CScript::const_iterator it = scriptCode.begin();
        CScript::const_iterator itBegin = it;
        opcodetype opcode;
        unsigned int nCodeSeparators = 0;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR)
                nCodeSeparators++;
        }
        ::WriteCompactSize(s, scriptCode.size() - nCodeSeparators);
        it = itBegin;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR) {
                s.write(AsBytes(Span{&itBegin[0], size_t(it - itBegin - 1)}));
                itBegin = it;
            }
        }
        if (itBegin != scriptCode.end())
            s.write(AsBytes(Span{&itBegin[0], size_t(it - itBegin)}));
    }

    /** Serialize an input of txTo */
    template<typename S>
    void SerializeInput(S &s, unsigned int nInput) const {
        // In case of SIGHASH_ANYONECANPAY, only the input being signed is serialized
        if (fAnyoneCanPay)
            nInput = nIn;
        // Serialize the prevout
        ::Serialize(s, txTo.vin[nInput].prevout);
        // Serialize the script
        if (nInput != nIn)
            // Blank out other inputs' signatures
            ::Serialize(s, CScript());
        else
            SerializeScriptCode(s);
        // Serialize the nSequence
        if (nInput != nIn && (fHashSingle || fHashNone))
            // let the others update at will
            ::Serialize(s, int{0});
        else
            ::Serialize(s, txTo.vin[nInput].nSequence);
    }

    /** Serialize an output of txTo */
    template<typename S>
    void SerializeOutput(S &s, unsigned int nOutput) const {
        if (fHashSingle && nOutput != nIn)
            // Do not lock-in the txout payee at other indices as txin
            ::Serialize(s, CTxOut());
        else
            ::Serialize(s, txTo.vout[nOutput]);
    }

    /** Serialize txTo */
    template<typename S>
    void Serialize(S &s) const {
        // Serialize version
        ::Serialize(s, txTo.version);
        // Serialize vin
        unsigned int nInputs = fAnyoneCanPay ? 1 : txTo.vin.size();
        ::WriteCompactSize(s, nInputs);
        for (unsigned int nInput = 0; nInput < nInputs; nInput++)
             SerializeInput(s, nInput);
        // Serialize vout
        unsigned int nOutputs = fHashNone ? 0 : (fHashSingle ? nIn+1 : txTo.vout.size());
        ::WriteCompactSize(s, nOutputs);
        for (unsigned int nOutput = 0; nOutput < nOutputs; nOutput++)
             SerializeOutput(s, nOutput);
        // Serialize nLockTime
        ::Serialize(s, txTo.nLockTime);
    }
};

/** Compute the (single) SHA256 of the concatenation of all prevouts of a tx. */
template <class T>
uint256 GetPrevoutsSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txin : txTo.vin) {
        ss << txin.prevout;
    }
    return ss.GetSHA256();
}

/** Compute the (single) SHA256 of the concatenation of all nSequences of a tx. */
template <class T>
uint256 GetSequencesSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txin : txTo.vin) {
        ss << txin.nSequence;
    }
    return ss.GetSHA256();
}

/** Compute the (single) SHA256 of the concatenation of all txouts of a tx. */
template <class T>
uint256 GetOutputsSHA256(const T& txTo)
{
    HashWriter ss{};
    for (const auto& txout : txTo.vout) {
        ss << txout;
    }
    return ss.GetSHA256();
}

/** Compute the (single) SHA256 of the concatenation of all amounts spent by a tx. */
uint256 GetSpentAmountsSHA256(const std::vector<CTxOut>& outputs_spent)
{
    HashWriter ss{};
    for (const auto& txout : outputs_spent) {
        ss << txout.nValue;
    }
    return ss.GetSHA256();
}

/** Compute the (single) SHA256 of the concatenation of all scriptPubKeys spent by a tx. */
uint256 GetSpentScriptsSHA256(const std::vector<CTxOut>& outputs_spent)
{
    HashWriter ss{};
    for (const auto& txout : outputs_spent) {
        ss << txout.scriptPubKey;
    }
    return ss.GetSHA256();
}


} // namespace

template <class T>
void PrecomputedTransactionData::Init(const T& txTo, std::vector<CTxOut>&& spent_outputs, bool force)
{
    assert(!m_spent_outputs_ready);

    m_spent_outputs = std::move(spent_outputs);
    if (!m_spent_outputs.empty()) {
        assert(m_spent_outputs.size() == txTo.vin.size());
        m_spent_outputs_ready = true;
    }

    // Determine which precomputation-impacting features this transaction uses.
    bool uses_bip143_segwit = force;
    bool uses_bip341_taproot = force;
    for (size_t inpos = 0; inpos < txTo.vin.size() && !(uses_bip143_segwit && uses_bip341_taproot); ++inpos) {
        if (!txTo.vin[inpos].scriptWitness.IsNull()) {
            if (m_spent_outputs_ready && m_spent_outputs[inpos].scriptPubKey.size() == 2 + WITNESS_V1_TAPROOT_SIZE &&
                m_spent_outputs[inpos].scriptPubKey[0] == OP_1) {
                // Treat every witness-bearing spend with 34-byte scriptPubKey that starts with OP_1 as a Taproot
                // spend. This only works if spent_outputs was provided as well, but if it wasn't, actual validation
                // will fail anyway. Note that this branch may trigger for scriptPubKeys that aren't actually segwit
                // but in that case validation will fail as SCRIPT_ERR_WITNESS_UNEXPECTED anyway.
                uses_bip341_taproot = true;
            } else {
                // Treat every spend that's not known to native witness v1 as a Witness v0 spend. This branch may
                // also be taken for unknown witness versions, but it is harmless, and being precise would require
                // P2SH evaluation to find the redeemScript.
                uses_bip143_segwit = true;
            }
        }
        if (uses_bip341_taproot && uses_bip143_segwit) break; // No need to scan further if we already need all.
    }

    if (uses_bip143_segwit || uses_bip341_taproot) {
        // Computations shared between both sighash schemes.
        m_prevouts_single_hash = GetPrevoutsSHA256(txTo);
        m_sequences_single_hash = GetSequencesSHA256(txTo);
        m_outputs_single_hash = GetOutputsSHA256(txTo);
    }
    if (uses_bip143_segwit) {
        hashPrevouts = SHA256Uint256(m_prevouts_single_hash);
        hashSequence = SHA256Uint256(m_sequences_single_hash);
        hashOutputs = SHA256Uint256(m_outputs_single_hash);
        m_bip143_segwit_ready = true;
    }
    if (uses_bip341_taproot && m_spent_outputs_ready) {
        m_spent_amounts_single_hash = GetSpentAmountsSHA256(m_spent_outputs);
        m_spent_scripts_single_hash = GetSpentScriptsSHA256(m_spent_outputs);
        m_bip341_taproot_ready = true;
    }
}

template <class T>
PrecomputedTransactionData::PrecomputedTransactionData(const T& txTo)
{
    Init(txTo, {});
}

// explicit instantiation
template void PrecomputedTransactionData::Init(const CTransaction& txTo, std::vector<CTxOut>&& spent_outputs, bool force);
template void PrecomputedTransactionData::Init(const CMutableTransaction& txTo, std::vector<CTxOut>&& spent_outputs, bool force);
template PrecomputedTransactionData::PrecomputedTransactionData(const CTransaction& txTo);
template PrecomputedTransactionData::PrecomputedTransactionData(const CMutableTransaction& txTo);

const HashWriter HASHER_TAPSIGHASH{TaggedHash("TapSighash")};
const HashWriter HASHER_TAPLEAF{TaggedHash("TapLeaf")};
const HashWriter HASHER_TAPBRANCH{TaggedHash("TapBranch")};

static bool HandleMissingData(MissingDataBehavior mdb)
{
    switch (mdb) {
    case MissingDataBehavior::ASSERT_FAIL:
        assert(!"Missing data");
        break;
    case MissingDataBehavior::FAIL:
        return false;
    }
    assert(!"Unknown MissingDataBehavior value");
}

template<typename T>
bool SignatureHashSchnorr(uint256& hash_out, ScriptExecutionData& execdata, const T& tx_to, uint32_t in_pos, uint8_t hash_type, SigVersion sigversion, const PrecomputedTransactionData& cache, MissingDataBehavior mdb)
{
    uint8_t ext_flag, key_version;
    switch (sigversion) {
    case SigVersion::TAPROOT:
        ext_flag = 0;
        // key_version is not used and left uninitialized.
        break;
    case SigVersion::TAPSCRIPT:
        ext_flag = 1;
        // key_version must be 0 for now, representing the current version of
        // 32-byte public keys in the tapscript signature opcode execution.
        // An upgradable public key version (with a size not 32-byte) may
        // request a different key_version with a new sigversion.
        key_version = 0;
        break;
    default:
        assert(false);
    }
    assert(in_pos < tx_to.vin.size());
    if (!(cache.m_bip341_taproot_ready && cache.m_spent_outputs_ready)) {
        return HandleMissingData(mdb);
    }

    HashWriter ss{HASHER_TAPSIGHASH};

    // Epoch
    static constexpr uint8_t EPOCH = 0;
    ss << EPOCH;

    // Hash type
    const uint8_t output_type = (hash_type == SIGHASH_DEFAULT) ? SIGHASH_ALL : (hash_type & SIGHASH_OUTPUT_MASK); // Default (no sighash byte) is equivalent to SIGHASH_ALL
    const uint8_t input_type = hash_type & SIGHASH_INPUT_MASK;
    if (!(hash_type <= 0x03 || (hash_type >= 0x81 && hash_type <= 0x83))) return false;
    ss << hash_type;

    // Transaction level data
    ss << tx_to.version;
    ss << tx_to.nLockTime;
    if (input_type != SIGHASH_ANYONECANPAY) {
        ss << cache.m_prevouts_single_hash;
        ss << cache.m_spent_amounts_single_hash;
        ss << cache.m_spent_scripts_single_hash;
        ss << cache.m_sequences_single_hash;
    }
    if (output_type == SIGHASH_ALL) {
        ss << cache.m_outputs_single_hash;
    }

    // Data about the input/prevout being spent
    assert(execdata.m_annex_init);
    const bool have_annex = execdata.m_annex_present;
    const uint8_t spend_type = (ext_flag << 1) + (have_annex ? 1 : 0); // The low bit indicates whether an annex is present.
    ss << spend_type;
    if (input_type == SIGHASH_ANYONECANPAY) {
        ss << tx_to.vin[in_pos].prevout;
        ss << cache.m_spent_outputs[in_pos];
        ss << tx_to.vin[in_pos].nSequence;
    } else {
        ss << in_pos;
    }
    if (have_annex) {
        ss << execdata.m_annex_hash;
    }

    // Data about the output (if only one).
    if (output_type == SIGHASH_SINGLE) {
        if (in_pos >= tx_to.vout.size()) return false;
        if (!execdata.m_output_hash) {
            HashWriter sha_single_output{};
            sha_single_output << tx_to.vout[in_pos];
            execdata.m_output_hash = sha_single_output.GetSHA256();
        }
        ss << execdata.m_output_hash.value();
    }

    // Additional data for BIP 342 signatures
    if (sigversion == SigVersion::TAPSCRIPT) {
        assert(execdata.m_tapleaf_hash_init);
        ss << execdata.m_tapleaf_hash;
        ss << key_version;
        assert(execdata.m_codeseparator_pos_init);
        ss << execdata.m_codeseparator_pos;
    }

    hash_out = ss.GetSHA256();
    return true;
}

template <class T>
uint256 SignatureHash(const CScript& scriptCode, const T& txTo, unsigned int nIn, int nHashType, const CAmount& amount, SigVersion sigversion, const PrecomputedTransactionData* cache)
{
    assert(nIn < txTo.vin.size());

    if (sigversion == SigVersion::WITNESS_V0) {
        uint256 hashPrevouts;
        uint256 hashSequence;
        uint256 hashOutputs;
        const bool cacheready = cache && cache->m_bip143_segwit_ready;

        if (!(nHashType & SIGHASH_ANYONECANPAY)) {
            hashPrevouts = cacheready ? cache->hashPrevouts : SHA256Uint256(GetPrevoutsSHA256(txTo));
        }

        if (!(nHashType & SIGHASH_ANYONECANPAY) && (nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashSequence = cacheready ? cache->hashSequence : SHA256Uint256(GetSequencesSHA256(txTo));
        }


        if ((nHashType & 0x1f) != SIGHASH_SINGLE && (nHashType & 0x1f) != SIGHASH_NONE) {
            hashOutputs = cacheready ? cache->hashOutputs : SHA256Uint256(GetOutputsSHA256(txTo));
        } else if ((nHashType & 0x1f) == SIGHASH_SINGLE && nIn < txTo.vout.size()) {
            HashWriter ss{};
            ss << txTo.vout[nIn];
            hashOutputs = ss.GetHash();
        }

        HashWriter ss{};
        // Version
        ss << txTo.version;
        // Input prevouts/nSequence (none/all, depending on flags)
        ss << hashPrevouts;
        ss << hashSequence;
        // The input being signed (replacing the scriptSig with scriptCode + amount)
        // The prevout may already be contained in hashPrevout, and the nSequence
        // may already be contain in hashSequence.
        ss << txTo.vin[nIn].prevout;
        ss << scriptCode;
        ss << amount;
        ss << txTo.vin[nIn].nSequence;
        // Outputs (none/one/all, depending on flags)
        ss << hashOutputs;
        // Locktime
        ss << txTo.nLockTime;
        // Sighash type
        ss << nHashType;

        return ss.GetHash();
    }

    // Check for invalid use of SIGHASH_SINGLE
    if ((nHashType & 0x1f) == SIGHASH_SINGLE) {
        if (nIn >= txTo.vout.size()) {
            //  nOut out of range
            return uint256::ONE;
        }
    }

    // Wrapper to serialize only the necessary parts of the transaction being signed
    CTransactionSignatureSerializer<T> txTmp(txTo, scriptCode, nIn, nHashType);

    // Serialize and hash
    HashWriter ss{};
    ss << txTmp << nHashType;
    return ss.GetHash();
}

template <class T>
bool GenericTransactionSignatureChecker<T>::VerifyECDSASignature(const std::vector<unsigned char>& vchSig, const CPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.Verify(sighash, vchSig);
}

template <class T>
bool GenericTransactionSignatureChecker<T>::VerifySchnorrSignature(Span<const unsigned char> sig, const XOnlyPubKey& pubkey, const uint256& sighash) const
{
    return pubkey.VerifySchnorr(sighash, sig);
}

template <class T>
bool GenericTransactionSignatureChecker<T>::CheckECDSASignature(const std::vector<unsigned char>& vchSigIn, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const
{
    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid())
        return false;

    // Hash type is one byte tacked on to the end of the signature
    std::vector<unsigned char> vchSig(vchSigIn);
    if (vchSig.empty())
        return false;
    int nHashType = vchSig.back();
    vchSig.pop_back();

    // Witness sighashes need the amount.
    if (sigversion == SigVersion::WITNESS_V0 && amount < 0) return HandleMissingData(m_mdb);

    uint256 sighash = SignatureHash(scriptCode, *txTo, nIn, nHashType, amount, sigversion, this->txdata);

    if (!VerifyECDSASignature(vchSig, pubkey, sighash))
        return false;

    return true;
}

template <class T>
bool GenericTransactionSignatureChecker<T>::CheckSchnorrSignature(Span<const unsigned char> sig, Span<const unsigned char> pubkey_in, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror) const
{
    assert(sigversion == SigVersion::TAPROOT || sigversion == SigVersion::TAPSCRIPT);
    // Schnorr signatures have 32-byte public keys. The caller is responsible for enforcing this.
    assert(pubkey_in.size() == 32);
    // Note that in Tapscript evaluation, empty signatures are treated specially (invalid signature that does not
    // abort script execution). This is implemented in EvalChecksigTapscript, which won't invoke
    // CheckSchnorrSignature in that case. In other contexts, they are invalid like every other signature with
    // size different from 64 or 65.
    if (sig.size() != 64 && sig.size() != 65) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_SIZE);

    XOnlyPubKey pubkey{pubkey_in};

    uint8_t hashtype = SIGHASH_DEFAULT;
    if (sig.size() == 65) {
        hashtype = SpanPopBack(sig);
        if (hashtype == SIGHASH_DEFAULT) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
    }
    uint256 sighash;
    if (!this->txdata) return HandleMissingData(m_mdb);
    if (!SignatureHashSchnorr(sighash, execdata, *txTo, nIn, hashtype, sigversion, *this->txdata, m_mdb)) {
        return set_error(serror, SCRIPT_ERR_SCHNORR_SIG_HASHTYPE);
    }
    if (!VerifySchnorrSignature(sig, pubkey, sighash)) return set_error(serror, SCRIPT_ERR_SCHNORR_SIG);
    return true;
}

template <class T>
bool GenericTransactionSignatureChecker<T>::CheckLockTime(const CScriptNum& nLockTime) const
{
    // There are two kinds of nLockTime: lock-by-blockheight
    // and lock-by-blocktime, distinguished by whether
    // nLockTime < LOCKTIME_THRESHOLD.
    //
    // We want to compare apples to apples, so fail the script
    // unless the type of nLockTime being tested is the same as
    // the nLockTime in the transaction.
    if (!(
        (txTo->nLockTime <  LOCKTIME_THRESHOLD && nLockTime <  LOCKTIME_THRESHOLD) ||
        (txTo->nLockTime >= LOCKTIME_THRESHOLD && nLockTime >= LOCKTIME_THRESHOLD)
    ))
        return false;

    // Now that we know we're comparing apples-to-apples, the
    // comparison is a simple numeric one.
    if (nLockTime > (int64_t)txTo->nLockTime)
        return false;

    // Finally the nLockTime feature can be disabled in IsFinalTx()
    // and thus CHECKLOCKTIMEVERIFY bypassed if every txin has
    // been finalized by setting nSequence to maxint. The
    // transaction would be allowed into the blockchain, making
    // the opcode ineffective.
    //
    // Testing if this vin is not final is sufficient to
    // prevent this condition. Alternatively we could test all
    // inputs, but testing just this input minimizes the data
    // required to prove correct CHECKLOCKTIMEVERIFY execution.
    if (CTxIn::SEQUENCE_FINAL == txTo->vin[nIn].nSequence)
        return false;

    return true;
}

template <class T>
bool GenericTransactionSignatureChecker<T>::CheckSequence(const CScriptNum& nSequence) const
{
    // Relative lock times are supported by comparing the passed
    // in operand to the sequence number of the input.
    const int64_t txToSequence = (int64_t)txTo->vin[nIn].nSequence;

    // Fail if the transaction's version number is not set high
    // enough to trigger BIP 68 rules.
    if (txTo->version < 2)
        return false;

    // Sequence numbers with their most significant bit set are not
    // consensus constrained. Testing that the transaction's sequence
    // number do not have this bit set prevents using this property
    // to get around a CHECKSEQUENCEVERIFY check.
    if (txToSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG)
        return false;

    // Mask off any bits that do not have consensus-enforced meaning
    // before doing the integer comparisons
    const uint32_t nLockTimeMask = CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG | CTxIn::SEQUENCE_LOCKTIME_MASK;
    const int64_t txToSequenceMasked = txToSequence & nLockTimeMask;
    const CScriptNum nSequenceMasked = nSequence & nLockTimeMask;

    // There are two kinds of nSequence: lock-by-blockheight
    // and lock-by-blocktime, distinguished by whether
    // nSequenceMasked < CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG.
    //
    // We want to compare apples to apples, so fail the script
    // unless the type of nSequenceMasked being tested is the same as
    // the nSequenceMasked in the transaction.
    if (!(
        (txToSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked <  CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) ||
        (txToSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG && nSequenceMasked >= CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG)
    )) {
        return false;
    }

    // Now that we know we're comparing apples-to-apples, the
    // comparison is a simple numeric one.
    if (nSequenceMasked > txToSequenceMasked)
        return false;

    return true;
}

// explicit instantiation
template class GenericTransactionSignatureChecker<CTransaction>;
template class GenericTransactionSignatureChecker<CMutableTransaction>;

static bool ExecuteWitnessScript(const Span<const valtype>& stack_span, const CScript& exec_script, unsigned int flags, SigVersion sigversion, const BaseSignatureChecker& checker, ScriptExecutionData& execdata, ScriptError* serror)
{
    std::vector<valtype> stack{stack_span.begin(), stack_span.end()};

    if (sigversion == SigVersion::TAPSCRIPT) {
        // OP_SUCCESSx processing overrides everything, including stack element size limits
        CScript::const_iterator pc = exec_script.begin();
        while (pc < exec_script.end()) {
            opcodetype opcode;
            if (!exec_script.GetOp(pc, opcode)) {
                // Note how this condition would not be reached if an unknown OP_SUCCESSx was found
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
            if (opcode == OP_CHECKSIGFROMSTACK) {
                if (flags & SCRIPT_VERIFY_DISCOURAGE_CHECKSIGFROMSTACK)
                    return set_error(serror, SCRIPT_ERR_DISCOURAGE_OP_SUCCESS);
                if (flags & SCRIPT_VERIFY_CHECKSIGFROMSTACK) continue;
                return set_success(serror);
            }
            // New opcodes will be listed here. May use a different sigversion to modify existing opcodes.
            if (IsOpSuccess(opcode)) {
                if (flags & SCRIPT_VERIFY_DISCOURAGE_OP_SUCCESS) {
                    return set_error(serror, SCRIPT_ERR_DISCOURAGE_OP_SUCCESS);
                }
                return set_success(serror);
            }
        }

        // Tapscript enforces initial stack size limits (altstack is empty here)
        if (stack.size() > MAX_STACK_SIZE) return set_error(serror, SCRIPT_ERR_STACK_SIZE);
    }

    // Disallow stack item size > MAX_SCRIPT_ELEMENT_SIZE in witness stack
    for (const valtype& elem : stack) {
        if (elem.size() > MAX_SCRIPT_ELEMENT_SIZE) return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
    }

    // Run the script interpreter.
    if (!EvalScript(stack, exec_script, flags, checker, sigversion, execdata, serror)) return false;

    // Scripts inside witness implicitly require cleanstack behaviour
    if (stack.size() != 1) return set_error(serror, SCRIPT_ERR_CLEANSTACK);
    if (!CastToBool(stack.back())) return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    return true;
}

uint256 ComputeTapleafHash(uint8_t leaf_version, Span<const unsigned char> script)
{
    return (HashWriter{HASHER_TAPLEAF} << leaf_version << CompactSizeWriter(script.size()) << script).GetSHA256();
}

uint256 ComputeTapbranchHash(Span<const unsigned char> a, Span<const unsigned char> b)
{
    HashWriter ss_branch{HASHER_TAPBRANCH};
    if (std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end())) {
        ss_branch << a << b;
    } else {
        ss_branch << b << a;
    }
    return ss_branch.GetSHA256();
}

uint256 ComputeTaprootMerkleRoot(Span<const unsigned char> control, const uint256& tapleaf_hash)
{
    assert(control.size() >= TAPROOT_CONTROL_BASE_SIZE);
    assert(control.size() <= TAPROOT_CONTROL_MAX_SIZE);
    assert((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE == 0);

    const int path_len = (control.size() - TAPROOT_CONTROL_BASE_SIZE) / TAPROOT_CONTROL_NODE_SIZE;
    uint256 k = tapleaf_hash;
    for (int i = 0; i < path_len; ++i) {
        Span node{Span{control}.subspan(TAPROOT_CONTROL_BASE_SIZE + TAPROOT_CONTROL_NODE_SIZE * i, TAPROOT_CONTROL_NODE_SIZE)};
        k = ComputeTapbranchHash(k, node);
    }
    return k;
}

static bool VerifyTaprootCommitment(const std::vector<unsigned char>& control, const std::vector<unsigned char>& program, const uint256& tapleaf_hash)
{
    assert(control.size() >= TAPROOT_CONTROL_BASE_SIZE);
    assert(program.size() >= uint256::size());
    //! The internal pubkey (x-only, so no Y coordinate parity).
    const XOnlyPubKey p{Span{control}.subspan(1, TAPROOT_CONTROL_BASE_SIZE - 1)};
    //! The output pubkey (taken from the scriptPubKey).
    const XOnlyPubKey q{program};
    // Compute the Merkle root from the leaf and the provided path.
    const uint256 merkle_root = ComputeTaprootMerkleRoot(control, tapleaf_hash);
    // Verify that the output pubkey matches the tweaked internal pubkey, after correcting for parity.
    return q.CheckTapTweak(p, merkle_root, control[0] & 1);
}

static bool VerifyWitnessProgram(const CScriptWitness& witness, int witversion, const std::vector<unsigned char>& program, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror, bool is_p2sh)
{
    CScript exec_script; //!< Actually executed script (last stack item in P2WSH; implied P2PKH script in P2WPKH; leaf script in P2TR)
    Span stack{witness.stack};
    ScriptExecutionData execdata;

    if (witversion == 0) {
        if (program.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            // BIP141 P2WSH: 32-byte witness v0 program (which encodes SHA256(script))
            if (stack.size() == 0) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
            }
            const valtype& script_bytes = SpanPopBack(stack);
            exec_script = CScript(script_bytes.begin(), script_bytes.end());
            uint256 hash_exec_script;
            CSHA256().Write(exec_script.data(), exec_script.size()).Finalize(hash_exec_script.begin());
            if (memcmp(hash_exec_script.begin(), program.data(), 32)) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::WITNESS_V0, checker, execdata, serror);
        } else if (program.size() == WITNESS_V0_KEYHASH_SIZE) {
            // BIP141 P2WPKH: 20-byte witness v0 program (which encodes Hash160(pubkey))
            if (stack.size() != 2) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH); // 2 items in witness
            }
            exec_script << OP_DUP << OP_HASH160 << program << OP_EQUALVERIFY << OP_CHECKSIG;
            return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::WITNESS_V0, checker, execdata, serror);
        } else {
            return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH);
        }
    } else if (witversion == 1 && program.size() == WITNESS_V1_TAPROOT_SIZE && !is_p2sh) {
        // BIP341 Taproot: 32-byte non-P2SH witness v1 program (which encodes a P2C-tweaked pubkey)
        if (!(flags & SCRIPT_VERIFY_TAPROOT)) return set_success(serror);
        if (stack.size() == 0) return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY);
        if (stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
            // Drop annex (this is non-standard; see IsWitnessStandard)
            const valtype& annex = SpanPopBack(stack);
            execdata.m_annex_hash = (HashWriter{} << annex).GetSHA256();
            execdata.m_annex_present = true;
        } else {
            execdata.m_annex_present = false;
        }
        execdata.m_annex_init = true;
        if (stack.size() == 1) {
            // Key path spending (stack size is 1 after removing optional annex)
            if (!checker.CheckSchnorrSignature(stack.front(), program, SigVersion::TAPROOT, execdata, serror)) {
                return false; // serror is set
            }
            return set_success(serror);
        } else {
            // Script path spending (stack size is >1 after removing optional annex)
            const valtype& control = SpanPopBack(stack);
            const valtype& script = SpanPopBack(stack);
            if (control.size() < TAPROOT_CONTROL_BASE_SIZE || control.size() > TAPROOT_CONTROL_MAX_SIZE || ((control.size() - TAPROOT_CONTROL_BASE_SIZE) % TAPROOT_CONTROL_NODE_SIZE) != 0) {
                return set_error(serror, SCRIPT_ERR_TAPROOT_WRONG_CONTROL_SIZE);
            }
            execdata.m_tapleaf_hash = ComputeTapleafHash(control[0] & TAPROOT_LEAF_MASK, script);
            if (!VerifyTaprootCommitment(control, program, execdata.m_tapleaf_hash)) {
                return set_error(serror, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);
            }
            execdata.m_tapleaf_hash_init = true;
            if ((control[0] & TAPROOT_LEAF_MASK) == TAPROOT_LEAF_TAPSCRIPT) {
                // Tapscript (leaf version 0xc0)
                exec_script = CScript(script.begin(), script.end());
                execdata.m_validation_weight_left = ::GetSerializeSize(witness.stack) + VALIDATION_WEIGHT_OFFSET;
                execdata.m_validation_weight_left_init = true;
                return ExecuteWitnessScript(stack, exec_script, flags, SigVersion::TAPSCRIPT, checker, execdata, serror);
            }
            if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION) {
                return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION);
            }
            return set_success(serror);
        }
    } else {
        if (flags & SCRIPT_VERIFY_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM) {
            return set_error(serror, SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM);
        }
        // Other version/size/p2sh combinations return true for future softfork compatibility
        return true;
    }
    // There is intentionally no return statement here, to be able to use "control reaches end of non-void function" warnings to detect gaps in the logic above.
}

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags, const BaseSignatureChecker& checker, ScriptError* serror)
{
    static const CScriptWitness emptyWitness;
    if (witness == nullptr) {
        witness = &emptyWitness;
    }
    bool hadWitness = false;

    set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);

    if ((flags & SCRIPT_VERIFY_SIGPUSHONLY) != 0 && !scriptSig.IsPushOnly()) {
        return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);
    }

    // scriptSig and scriptPubKey must be evaluated sequentially on the same stack
    // rather than being simply concatenated (see CVE-2010-5141)
    std::vector<std::vector<unsigned char> > stack, stackCopy;
    if (!EvalScript(stack, scriptSig, flags, checker, SigVersion::BASE, serror))
        // serror is set
        return false;
    if (flags & SCRIPT_VERIFY_P2SH)
        stackCopy = stack;
    if (!EvalScript(stack, scriptPubKey, flags, checker, SigVersion::BASE, serror))
        // serror is set
        return false;
    if (stack.empty())
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
    if (CastToBool(stack.back()) == false)
        return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

    // Bare witness programs
    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (flags & SCRIPT_VERIFY_WITNESS) {
        if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            hadWitness = true;
            if (scriptSig.size() != 0) {
                // The scriptSig must be _exactly_ CScript(), otherwise we reintroduce malleability.
                return set_error(serror, SCRIPT_ERR_WITNESS_MALLEATED);
            }
            if (!VerifyWitnessProgram(*witness, witnessversion, witnessprogram, flags, checker, serror, /*is_p2sh=*/false)) {
                return false;
            }
            // Bypass the cleanstack check at the end. The actual stack is obviously not clean
            // for witness programs.
            stack.resize(1);
        }
    }

    // Additional validation for spend-to-script-hash transactions:
    if ((flags & SCRIPT_VERIFY_P2SH) && scriptPubKey.IsPayToScriptHash())
    {
        // scriptSig must be literals-only or validation fails
        if (!scriptSig.IsPushOnly())
            return set_error(serror, SCRIPT_ERR_SIG_PUSHONLY);

        // Restore stack.
        swap(stack, stackCopy);

        // stack cannot be empty here, because if it was the
        // P2SH  HASH <> EQUAL  scriptPubKey would be evaluated with
        // an empty stack and the EvalScript above would return false.
        assert(!stack.empty());

        const valtype& pubKeySerialized = stack.back();
        CScript pubKey2(pubKeySerialized.begin(), pubKeySerialized.end());
        popstack(stack);

        if (!EvalScript(stack, pubKey2, flags, checker, SigVersion::BASE, serror))
            // serror is set
            return false;
        if (stack.empty())
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
        if (!CastToBool(stack.back()))
            return set_error(serror, SCRIPT_ERR_EVAL_FALSE);

        // P2SH witness program
        if (flags & SCRIPT_VERIFY_WITNESS) {
            if (pubKey2.IsWitnessProgram(witnessversion, witnessprogram)) {
                hadWitness = true;
                if (scriptSig != CScript() << std::vector<unsigned char>(pubKey2.begin(), pubKey2.end())) {
                    // The scriptSig must be _exactly_ a single push of the redeemScript. Otherwise we
                    // reintroduce malleability.
                    return set_error(serror, SCRIPT_ERR_WITNESS_MALLEATED_P2SH);
                }
                if (!VerifyWitnessProgram(*witness, witnessversion, witnessprogram, flags, checker, serror, /*is_p2sh=*/true)) {
                    return false;
                }
                // Bypass the cleanstack check at the end. The actual stack is obviously not clean
                // for witness programs.
                stack.resize(1);
            }
        }
    }

    // The CLEANSTACK check is only performed after potential P2SH evaluation,
    // as the non-P2SH evaluation of a P2SH script will obviously not result in
    // a clean stack (the P2SH inputs remain). The same holds for witness evaluation.
    if ((flags & SCRIPT_VERIFY_CLEANSTACK) != 0) {
        // Disallow CLEANSTACK without P2SH, as otherwise a switch CLEANSTACK->P2SH+CLEANSTACK
        // would be possible, which is not a softfork (and P2SH should be one).
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        assert((flags & SCRIPT_VERIFY_WITNESS) != 0);
        if (stack.size() != 1) {
            return set_error(serror, SCRIPT_ERR_CLEANSTACK);
        }
    }

    if (flags & SCRIPT_VERIFY_WITNESS) {
        // We can't check for correct unexpected witness data if P2SH was off, so require
        // that WITNESS implies P2SH. Otherwise, going from WITNESS->P2SH+WITNESS would be
        // possible, which is not a softfork.
        assert((flags & SCRIPT_VERIFY_P2SH) != 0);
        if (!hadWitness && !witness->IsNull()) {
            return set_error(serror, SCRIPT_ERR_WITNESS_UNEXPECTED);
        }
    }

    return set_success(serror);
}

size_t static WitnessSigOps(int witversion, const std::vector<unsigned char>& witprogram, const CScriptWitness& witness)
{
    if (witversion == 0) {
        if (witprogram.size() == WITNESS_V0_KEYHASH_SIZE)
            return 1;

        if (witprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE && witness.stack.size() > 0) {
            CScript subscript(witness.stack.back().begin(), witness.stack.back().end());
            return subscript.GetSigOpCount(true);
        }
    }

    // Future flags may be implemented here.
    return 0;
}

size_t CountWitnessSigOps(const CScript& scriptSig, const CScript& scriptPubKey, const CScriptWitness* witness, unsigned int flags)
{
    static const CScriptWitness witnessEmpty;

    if ((flags & SCRIPT_VERIFY_WITNESS) == 0) {
        return 0;
    }
    assert((flags & SCRIPT_VERIFY_P2SH) != 0);

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        return WitnessSigOps(witnessversion, witnessprogram, witness ? *witness : witnessEmpty);
    }

    if (scriptPubKey.IsPayToScriptHash() && scriptSig.IsPushOnly()) {
        CScript::const_iterator pc = scriptSig.begin();
        std::vector<unsigned char> data;
        while (pc < scriptSig.end()) {
            opcodetype opcode;
            scriptSig.GetOp(pc, opcode, data);
        }
        CScript subscript(data.begin(), data.end());
        if (subscript.IsWitnessProgram(witnessversion, witnessprogram)) {
            return WitnessSigOps(witnessversion, witnessprogram, witness ? *witness : witnessEmpty);
        }
    }

    return 0;
}
