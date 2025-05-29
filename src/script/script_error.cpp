// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script_error.h>

#include <string>
extern "C" {
#include <simplicity/errorCodes.h>
}

std::string ScriptErrorString(const ScriptError serror)
{
    switch (serror)
    {
        case SCRIPT_ERR_OK:
            return "No error";
        case SCRIPT_ERR_EVAL_FALSE:
            return "Script evaluated without error but finished with a false/empty top stack element";
        case SCRIPT_ERR_VERIFY:
            return "Script failed an OP_VERIFY operation";
        case SCRIPT_ERR_EQUALVERIFY:
            return "Script failed an OP_EQUALVERIFY operation";
        case SCRIPT_ERR_CHECKMULTISIGVERIFY:
            return "Script failed an OP_CHECKMULTISIGVERIFY operation";
        case SCRIPT_ERR_CHECKSIGVERIFY:
            return "Script failed an OP_CHECKSIGVERIFY operation";
        case SCRIPT_ERR_NUMEQUALVERIFY:
            return "Script failed an OP_NUMEQUALVERIFY operation";
        case SCRIPT_ERR_SCRIPT_SIZE:
            return "Script is too big";
        case SCRIPT_ERR_PUSH_SIZE:
            return "Push value size limit exceeded";
        case SCRIPT_ERR_OP_COUNT:
            return "Operation limit exceeded";
        case SCRIPT_ERR_STACK_SIZE:
            return "Stack size limit exceeded";
        case SCRIPT_ERR_SIG_COUNT:
            return "Signature count negative or greater than pubkey count";
        case SCRIPT_ERR_PUBKEY_COUNT:
            return "Pubkey count negative or limit exceeded";
        case SCRIPT_ERR_BAD_OPCODE:
            return "Opcode missing or not understood";
        case SCRIPT_ERR_DISABLED_OPCODE:
            return "Attempted to use a disabled opcode";
        case SCRIPT_ERR_INVALID_STACK_OPERATION:
            return "Operation not valid with the current stack size";
        case SCRIPT_ERR_INVALID_ALTSTACK_OPERATION:
            return "Operation not valid with the current altstack size";
        case SCRIPT_ERR_OP_RETURN:
            return "OP_RETURN was encountered";
        case SCRIPT_ERR_UNBALANCED_CONDITIONAL:
            return "Invalid OP_IF construction";
        case SCRIPT_ERR_NEGATIVE_LOCKTIME:
            return "Negative locktime";
        case SCRIPT_ERR_UNSATISFIED_LOCKTIME:
            return "Locktime requirement not satisfied";
        case SCRIPT_ERR_SIG_HASHTYPE:
            return "Signature hash type missing or not understood";
        case SCRIPT_ERR_SIG_DER:
            return "Non-canonical DER signature";
        case SCRIPT_ERR_MINIMALDATA:
            return "Data push larger than necessary";
        case SCRIPT_ERR_SIG_PUSHONLY:
            return "Only push operators allowed in signatures";
        case SCRIPT_ERR_SIG_HIGH_S:
            return "Non-canonical signature: S value is unnecessarily high";
        case SCRIPT_ERR_SIG_NULLDUMMY:
            return "Dummy CHECKMULTISIG argument must be zero";
        case SCRIPT_ERR_MINIMALIF:
            return "OP_IF/NOTIF argument must be minimal";
        case SCRIPT_ERR_SIG_NULLFAIL:
            return "Signature must be zero for failed CHECK(MULTI)SIG operation";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_NOPS:
            return "NOPx reserved for soft-fork upgrades";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_WITNESS_PROGRAM:
            return "Witness version reserved for soft-fork upgrades";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_TAPROOT_VERSION:
            return "Taproot version reserved for soft-fork upgrades";
        case SCRIPT_ERR_DISCOURAGE_OP_SUCCESS:
            return "OP_SUCCESSx reserved for soft-fork upgrades";
        case SCRIPT_ERR_DISCOURAGE_UPGRADABLE_PUBKEYTYPE:
            return "Public key version reserved for soft-fork upgrades";
        case SCRIPT_ERR_PUBKEYTYPE:
            return "Public key is neither compressed or uncompressed";
        case SCRIPT_ERR_CLEANSTACK:
            return "Stack size must be exactly one after execution";
        case SCRIPT_ERR_WITNESS_PROGRAM_WRONG_LENGTH:
            return "Witness program has incorrect length";
        case SCRIPT_ERR_WITNESS_PROGRAM_WITNESS_EMPTY:
            return "Witness program was passed an empty witness";
        case SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH:
            return "Witness program hash mismatch";
        case SCRIPT_ERR_WITNESS_MALLEATED:
            return "Witness requires empty scriptSig";
        case SCRIPT_ERR_WITNESS_MALLEATED_P2SH:
            return "Witness requires only-redeemscript scriptSig";
        case SCRIPT_ERR_WITNESS_UNEXPECTED:
            return "Witness provided for non-witness script";
        case SCRIPT_ERR_WITNESS_PUBKEYTYPE:
            return "Using non-compressed keys in segwit";
        case SCRIPT_ERR_SCHNORR_SIG_SIZE:
            return "Invalid Schnorr signature size";
        case SCRIPT_ERR_SCHNORR_SIG_HASHTYPE:
            return "Invalid Schnorr signature hash type";
        case SCRIPT_ERR_SCHNORR_SIG:
            return "Invalid Schnorr signature";
        case SCRIPT_ERR_TAPROOT_WRONG_CONTROL_SIZE:
            return "Invalid Taproot control block size";
        case SCRIPT_ERR_TAPSCRIPT_VALIDATION_WEIGHT:
            return "Too much signature validation relative to witness weight";
        case SCRIPT_ERR_TAPSCRIPT_CHECKMULTISIG:
            return "OP_CHECKMULTISIG(VERIFY) is not available in tapscript";
        case SCRIPT_ERR_TAPSCRIPT_MINIMALIF:
            return "OP_IF/NOTIF argument must be minimal in tapscript";
        case SCRIPT_ERR_TAPSCRIPT_EMPTY_PUBKEY:
            return "Empty public key in tapscript";
        case SCRIPT_ERR_OP_CODESEPARATOR:
            return "Using OP_CODESEPARATOR in non-witness script";
        case SCRIPT_ERR_SIG_FINDANDDELETE:
            return "Signature is found in scriptCode";
        case SCRIPT_ERR_SIMPLICITY_WRONG_LENGTH:
            return "Simplicity witness has incorrect length";
        case SCRIPT_ERR_SIMPLICITY_DATA_OUT_OF_RANGE:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_DATA_OUT_OF_RANGE);
        case SCRIPT_ERR_SIMPLICITY_DATA_OUT_OF_ORDER:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_DATA_OUT_OF_ORDER);
        case SCRIPT_ERR_SIMPLICITY_FAIL_CODE:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_FAIL_CODE);
        case SCRIPT_ERR_SIMPLICITY_RESERVED_CODE:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_RESERVED_CODE);
        case SCRIPT_ERR_SIMPLICITY_HIDDEN:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_HIDDEN);
        case SCRIPT_ERR_SIMPLICITY_BITSTREAM_EOF:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_BITSTREAM_EOF);
        case SCRIPT_ERR_SIMPLICITY_BITSTREAM_TRAILING_BYTES:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES);
        case SCRIPT_ERR_SIMPLICITY_BITSTREAM_ILLEGAL_PADDING:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING);
        case SCRIPT_ERR_SIMPLICITY_TYPE_INFERENCE_UNIFICATION:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_TYPE_INFERENCE_UNIFICATION);
        case SCRIPT_ERR_SIMPLICITY_TYPE_INFERENCE_OCCURS_CHECK:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_TYPE_INFERENCE_OCCURS_CHECK);
        case SCRIPT_ERR_SIMPLICITY_TYPE_INFERENCE_NOT_PROGRAM:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_TYPE_INFERENCE_NOT_PROGRAM);
        case SCRIPT_ERR_SIMPLICITY_WITNESS_EOF:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_WITNESS_EOF);
        case SCRIPT_ERR_SIMPLICITY_WITNESS_TRAILING_BYTES:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_WITNESS_TRAILING_BYTES);
        case SCRIPT_ERR_SIMPLICITY_WITNESS_ILLEGAL_PADDING:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_WITNESS_ILLEGAL_PADDING);
        case SCRIPT_ERR_SIMPLICITY_UNSHARED_SUBEXPRESSION:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_UNSHARED_SUBEXPRESSION);
        case SCRIPT_ERR_SIMPLICITY_CMR:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_CMR);
        case SCRIPT_ERR_SIMPLICITY_EXEC_BUDGET:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_EXEC_BUDGET);
        case SCRIPT_ERR_SIMPLICITY_EXEC_MEMORY:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_EXEC_MEMORY);
        case SCRIPT_ERR_SIMPLICITY_EXEC_JET:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_EXEC_JET);
        case SCRIPT_ERR_SIMPLICITY_EXEC_ASSERT:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_EXEC_ASSERT);
        case SCRIPT_ERR_SIMPLICITY_ANTIDOS:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_ANTIDOS);
        case SCRIPT_ERR_SIMPLICITY_HIDDEN_ROOT:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_HIDDEN_ROOT);
        case SCRIPT_ERR_SIMPLICITY_AMR:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_AMR);
        case SCRIPT_ERR_SIMPLICITY_OVERWEIGHT:
            return SIMPLICITY_ERR_MSG(SIMPLICITY_ERR_OVERWEIGHT);
        case SCRIPT_ERR_UNKNOWN_ERROR:
        case SCRIPT_ERR_ERROR_COUNT:
        default: break;
    }
    return "unknown error";
}
