// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script.h"

#include "tinyformat.h"
#include "utilstrencodings.h"

namespace {
inline std::string ValueString(const std::vector<unsigned char>& vch)
{
    if (vch.size() <= 4)
        return strprintf("%d", CScriptNum(vch, false).getint());
    else
        return HexStr(vch);
}
} // anon namespace

using namespace std;

const char* GetOpName(opcodetype opcode)
{
    switch (opcode)
    {
    // push value
    case OP_0                      : return "0";
    case OP_PUSHDATA1              : return "PUSHDATA1";
    case OP_PUSHDATA2              : return "PUSHDATA2";
    case OP_PUSHDATA4              : return "PUSHDATA4";
    case OP_1NEGATE                : return "-1";
    case OP_RESERVED               : return "RESERVED";
    case OP_1                      : return "1";
    case OP_2                      : return "2";
    case OP_3                      : return "3";
    case OP_4                      : return "4";
    case OP_5                      : return "5";
    case OP_6                      : return "6";
    case OP_7                      : return "7";
    case OP_8                      : return "8";
    case OP_9                      : return "9";
    case OP_10                     : return "10";
    case OP_11                     : return "11";
    case OP_12                     : return "12";
    case OP_13                     : return "13";
    case OP_14                     : return "14";
    case OP_15                     : return "15";
    case OP_16                     : return "16";

    // control
    case OP_NOP                    : return "NOP";
    case OP_VER                    : return "VER";
    case OP_IF                     : return "IF";
    case OP_NOTIF                  : return "NOTIF";
    case OP_VERIF                  : return "VERIF";
    case OP_VERNOTIF               : return "VERNOTIF";
    case OP_ELSE                   : return "ELSE";
    case OP_ENDIF                  : return "ENDIF";
    case OP_VERIFY                 : return "VERIFY";
    case OP_RETURN                 : return "RETURN";

    // stack ops
    case OP_TOALTSTACK             : return "TOALTSTACK";
    case OP_FROMALTSTACK           : return "FROMALTSTACK";
    case OP_2DROP                  : return "2DROP";
    case OP_2DUP                   : return "2DUP";
    case OP_3DUP                   : return "3DUP";
    case OP_2OVER                  : return "2OVER";
    case OP_2ROT                   : return "2ROT";
    case OP_2SWAP                  : return "2SWAP";
    case OP_IFDUP                  : return "IFDUP";
    case OP_DEPTH                  : return "DEPTH";
    case OP_DROP                   : return "DROP";
    case OP_DUP                    : return "DUP";
    case OP_NIP                    : return "NIP";
    case OP_OVER                   : return "OVER";
    case OP_PICK                   : return "PICK";
    case OP_ROLL                   : return "ROLL";
    case OP_ROT                    : return "ROT";
    case OP_SWAP                   : return "SWAP";
    case OP_TUCK                   : return "TUCK";

    // splice ops
    case OP_CAT                    : return "CAT";
    case OP_SUBSTR                 : return "SUBSTR";
    case OP_LEFT                   : return "LEFT";
    case OP_RIGHT                  : return "RIGHT";
    case OP_SIZE                   : return "SIZE";

    // bit logic
    case OP_INVERT                 : return "INVERT";
    case OP_AND                    : return "AND";
    case OP_OR                     : return "OR";
    case OP_XOR                    : return "XOR";
    case OP_EQUAL                  : return "EQUAL";
    case OP_EQUALVERIFY            : return "EQUALVERIFY";
    case OP_RESERVED1              : return "RESERVED1";
    case OP_RESERVED2              : return "RESERVED2";

    // numeric
    case OP_1ADD                   : return "1ADD";
    case OP_1SUB                   : return "1SUB";
    case OP_2MUL                   : return "2MUL";
    case OP_2DIV                   : return "2DIV";
    case OP_NEGATE                 : return "NEGATE";
    case OP_ABS                    : return "ABS";
    case OP_NOT                    : return "NOT";
    case OP_0NOTEQUAL              : return "0NOTEQUAL";
    case OP_ADD                    : return "ADD";
    case OP_SUB                    : return "SUB";
    case OP_MUL                    : return "MUL";
    case OP_DIV                    : return "DIV";
    case OP_MOD                    : return "MOD";
    case OP_LSHIFT                 : return "LSHIFT";
    case OP_RSHIFT                 : return "RSHIFT";
    case OP_BOOLAND                : return "BOOLAND";
    case OP_BOOLOR                 : return "BOOLOR";
    case OP_NUMEQUAL               : return "NUMEQUAL";
    case OP_NUMEQUALVERIFY         : return "NUMEQUALVERIFY";
    case OP_NUMNOTEQUAL            : return "NUMNOTEQUAL";
    case OP_LESSTHAN               : return "LESSTHAN";
    case OP_GREATERTHAN            : return "GREATERTHAN";
    case OP_LESSTHANOREQUAL        : return "LESSTHANOREQUAL";
    case OP_GREATERTHANOREQUAL     : return "GREATERTHANOREQUAL";
    case OP_MIN                    : return "MIN";
    case OP_MAX                    : return "MAX";
    case OP_WITHIN                 : return "WITHIN";

    // crypto
    case OP_RIPEMD160              : return "RIPEMD160";
    case OP_SHA1                   : return "SHA1";
    case OP_SHA256                 : return "SHA256";
    case OP_HASH160                : return "HASH160";
    case OP_HASH256                : return "HASH256";
    case OP_CODESEPARATOR          : return "CODESEPARATOR";
    case OP_CHECKSIG               : return "CHECKSIG";
    case OP_CHECKSIGVERIFY         : return "CHECKSIGVERIFY";
    case OP_CHECKMULTISIG          : return "CHECKMULTISIG";
    case OP_CHECKMULTISIGVERIFY    : return "CHECKMULTISIGVERIFY";

    // expanson
    case OP_NOP1                   : return "NOP1";
    case OP_NOP2                   : return "NOP2";
    case OP_NOP3                   : return "NOP3";
    case OP_NOP4                   : return "NOP4";
    case OP_NOP5                   : return "NOP5";
    case OP_NOP6                   : return "NOP6";
    case OP_NOP7                   : return "NOP7";
    case OP_NOP8                   : return "NOP8";
    case OP_NOP9                   : return "NOP9";
    case OP_NOP10                  : return "NOP10";

    case OP_INVALIDOPCODE          : return "INVALIDOPCODE";

    // Note:
    //  The template matching params OP_SMALLDATA/etc are defined in opcodetype enum
    //  as kind of implementation hack, they are *NOT* real opcodes.  If found in real
    //  Script, just let the default: case deal with them.

    default:
        return "UNKNOWN";
    }
}

unsigned int CScript::GetSigOpCount(bool fAccurate) const
{
    unsigned int n = 0;
    const_iterator pc = begin();
    opcodetype lastOpcode = OP_INVALIDOPCODE;
    while (pc < end())
    {
        opcodetype opcode;
        if (!GetOp(pc, opcode))
            break;
        if (opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY)
            n++;
        else if (opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY)
        {
            if (fAccurate && lastOpcode >= OP_1 && lastOpcode <= OP_16)
                n += DecodeOP_N(lastOpcode);
            else
                n += 20;
        }
        lastOpcode = opcode;
    }
    return n;
}

unsigned int CScript::GetSigOpCount(const CScript& scriptSig) const
{
    if (!IsPayToScriptHash())
        return GetSigOpCount(true);

    // This is a pay-to-script-hash scriptPubKey;
    // get the last item that the scriptSig
    // pushes onto the stack:
    const_iterator pc = scriptSig.begin();
    vector<unsigned char> data;
    while (pc < scriptSig.end())
    {
        opcodetype opcode;
        if (!scriptSig.GetOp(pc, opcode, data))
            return 0;
        if (opcode > OP_16)
            return 0;
    }

    /// ... and return its opcount:
    CScript subscript(data.begin(), data.end());
    return subscript.GetSigOpCount(true);
}

bool CScript::IsPayToScriptHash() const
{
    // Extra-fast test for pay-to-script-hash CScripts:
    return (this->size() == 23 &&
            this->at(0) == OP_HASH160 &&
            this->at(1) == 0x14 &&
            this->at(22) == OP_EQUAL);
}

bool CScript::IsPushOnly() const
{
    const_iterator pc = begin();
    while (pc < end())
    {
        opcodetype opcode;
        if (!GetOp(pc, opcode))
            return false;
        // Note that IsPushOnly() *does* consider OP_RESERVED to be a
        // push-type opcode, however execution of OP_RESERVED fails, so
        // it's not relevant to P2SH/BIP62 as the scriptSig would fail prior to
        // the P2SH special validation code being executed.
        if (opcode > OP_16)
            return false;
    }
    return true;
}

std::string CScript::ToString() const
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    const_iterator pc = begin();
    while (pc < end())
    {
        if (!str.empty())
            str += " ";
        if (!GetOp(pc, opcode, vch))
        {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4)
            str += ValueString(vch);
        else
            str += GetOpName(opcode);
    }
    return str;
}
