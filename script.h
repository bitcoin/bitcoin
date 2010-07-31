// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

class CTransaction;

enum
{
    SIGHASH_ALL = 1,
    SIGHASH_NONE = 2,
    SIGHASH_SINGLE = 3,
    SIGHASH_ANYONECANPAY = 0x80,
};



enum opcodetype
{
    // push value
    OP_0=0,
    OP_FALSE=OP_0,
    OP_PUSHDATA1=76,
    OP_PUSHDATA2,
    OP_PUSHDATA4,
    OP_1NEGATE,
    OP_RESERVED,
    OP_1,
    OP_TRUE=OP_1,
    OP_2,
    OP_3,
    OP_4,
    OP_5,
    OP_6,
    OP_7,
    OP_8,
    OP_9,
    OP_10,
    OP_11,
    OP_12,
    OP_13,
    OP_14,
    OP_15,
    OP_16,

    // control
    OP_NOP,
    OP_VER,
    OP_IF,
    OP_NOTIF,
    OP_VERIF,
    OP_VERNOTIF,
    OP_ELSE,
    OP_ENDIF,
    OP_VERIFY,
    OP_RETURN,

    // stack ops
    OP_TOALTSTACK,
    OP_FROMALTSTACK,
    OP_2DROP,
    OP_2DUP,
    OP_3DUP,
    OP_2OVER,
    OP_2ROT,
    OP_2SWAP,
    OP_IFDUP,
    OP_DEPTH,
    OP_DROP,
    OP_DUP,
    OP_NIP,
    OP_OVER,
    OP_PICK,
    OP_ROLL,
    OP_ROT,
    OP_SWAP,
    OP_TUCK,

    // splice ops
    OP_CAT,
    OP_SUBSTR,
    OP_LEFT,
    OP_RIGHT,
    OP_SIZE,

    // bit logic
    OP_INVERT,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_RESERVED1,
    OP_RESERVED2,

    // numeric
    OP_1ADD,
    OP_1SUB,
    OP_2MUL,
    OP_2DIV,
    OP_NEGATE,
    OP_ABS,
    OP_NOT,
    OP_0NOTEQUAL,

    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_LSHIFT,
    OP_RSHIFT,

    OP_BOOLAND,
    OP_BOOLOR,
    OP_NUMEQUAL,
    OP_NUMEQUALVERIFY,
    OP_NUMNOTEQUAL,
    OP_LESSTHAN,
    OP_GREATERTHAN,
    OP_LESSTHANOREQUAL,
    OP_GREATERTHANOREQUAL,
    OP_MIN,
    OP_MAX,

    OP_WITHIN,

    // crypto
    OP_RIPEMD160,
    OP_SHA1,
    OP_SHA256,
    OP_HASH160,
    OP_HASH256,
    OP_CODESEPARATOR,
    OP_CHECKSIG,
    OP_CHECKSIGVERIFY,
    OP_CHECKMULTISIG,
    OP_CHECKMULTISIGVERIFY,

    // expansion
    OP_NOP1,
    OP_NOP2,
    OP_NOP3,
    OP_NOP4,
    OP_NOP5,
    OP_NOP6,
    OP_NOP7,
    OP_NOP8,
    OP_NOP9,
    OP_NOP10,




    // multi-byte opcodes
    OP_SINGLEBYTE_END = 0xF0,
    OP_DOUBLEBYTE_BEGIN = 0xF000,

    // template matching params
    OP_PUBKEY,
    OP_PUBKEYHASH,



    OP_INVALIDOPCODE = 0xFFFF,
};








inline const char* GetOpName(opcodetype opcode)
{
    switch (opcode)
    {
    // push value
    case OP_0                      : return "0";
    case OP_PUSHDATA1              : return "OP_PUSHDATA1";
    case OP_PUSHDATA2              : return "OP_PUSHDATA2";
    case OP_PUSHDATA4              : return "OP_PUSHDATA4";
    case OP_1NEGATE                : return "-1";
    case OP_RESERVED               : return "OP_RESERVED";
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
    case OP_NOP                    : return "OP_NOP";
    case OP_VER                    : return "OP_VER";
    case OP_IF                     : return "OP_IF";
    case OP_NOTIF                  : return "OP_NOTIF";
    case OP_VERIF                  : return "OP_VERIF";
    case OP_VERNOTIF               : return "OP_VERNOTIF";
    case OP_ELSE                   : return "OP_ELSE";
    case OP_ENDIF                  : return "OP_ENDIF";
    case OP_VERIFY                 : return "OP_VERIFY";
    case OP_RETURN                 : return "OP_RETURN";

    // stack ops
    case OP_TOALTSTACK             : return "OP_TOALTSTACK";
    case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
    case OP_2DROP                  : return "OP_2DROP";
    case OP_2DUP                   : return "OP_2DUP";
    case OP_3DUP                   : return "OP_3DUP";
    case OP_2OVER                  : return "OP_2OVER";
    case OP_2ROT                   : return "OP_2ROT";
    case OP_2SWAP                  : return "OP_2SWAP";
    case OP_IFDUP                  : return "OP_IFDUP";
    case OP_DEPTH                  : return "OP_DEPTH";
    case OP_DROP                   : return "OP_DROP";
    case OP_DUP                    : return "OP_DUP";
    case OP_NIP                    : return "OP_NIP";
    case OP_OVER                   : return "OP_OVER";
    case OP_PICK                   : return "OP_PICK";
    case OP_ROLL                   : return "OP_ROLL";
    case OP_ROT                    : return "OP_ROT";
    case OP_SWAP                   : return "OP_SWAP";
    case OP_TUCK                   : return "OP_TUCK";

    // splice ops
    case OP_CAT                    : return "OP_CAT";
    case OP_SUBSTR                 : return "OP_SUBSTR";
    case OP_LEFT                   : return "OP_LEFT";
    case OP_RIGHT                  : return "OP_RIGHT";
    case OP_SIZE                   : return "OP_SIZE";

    // bit logic
    case OP_INVERT                 : return "OP_INVERT";
    case OP_AND                    : return "OP_AND";
    case OP_OR                     : return "OP_OR";
    case OP_XOR                    : return "OP_XOR";
    case OP_EQUAL                  : return "OP_EQUAL";
    case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
    case OP_RESERVED1              : return "OP_RESERVED1";
    case OP_RESERVED2              : return "OP_RESERVED2";

    // numeric
    case OP_1ADD                   : return "OP_1ADD";
    case OP_1SUB                   : return "OP_1SUB";
    case OP_2MUL                   : return "OP_2MUL";
    case OP_2DIV                   : return "OP_2DIV";
    case OP_NEGATE                 : return "OP_NEGATE";
    case OP_ABS                    : return "OP_ABS";
    case OP_NOT                    : return "OP_NOT";
    case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
    case OP_ADD                    : return "OP_ADD";
    case OP_SUB                    : return "OP_SUB";
    case OP_MUL                    : return "OP_MUL";
    case OP_DIV                    : return "OP_DIV";
    case OP_MOD                    : return "OP_MOD";
    case OP_LSHIFT                 : return "OP_LSHIFT";
    case OP_RSHIFT                 : return "OP_RSHIFT";
    case OP_BOOLAND                : return "OP_BOOLAND";
    case OP_BOOLOR                 : return "OP_BOOLOR";
    case OP_NUMEQUAL               : return "OP_NUMEQUAL";
    case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
    case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
    case OP_LESSTHAN               : return "OP_LESSTHAN";
    case OP_GREATERTHAN            : return "OP_GREATERTHAN";
    case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
    case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
    case OP_MIN                    : return "OP_MIN";
    case OP_MAX                    : return "OP_MAX";
    case OP_WITHIN                 : return "OP_WITHIN";

    // crypto
    case OP_RIPEMD160              : return "OP_RIPEMD160";
    case OP_SHA1                   : return "OP_SHA1";
    case OP_SHA256                 : return "OP_SHA256";
    case OP_HASH160                : return "OP_HASH160";
    case OP_HASH256                : return "OP_HASH256";
    case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
    case OP_CHECKSIG               : return "OP_CHECKSIG";
    case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
    case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
    case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";

    // expanson
    case OP_NOP1                   : return "OP_NOP1";
    case OP_NOP2                   : return "OP_NOP2";
    case OP_NOP3                   : return "OP_NOP3";
    case OP_NOP4                   : return "OP_NOP4";
    case OP_NOP5                   : return "OP_NOP5";
    case OP_NOP6                   : return "OP_NOP6";
    case OP_NOP7                   : return "OP_NOP7";
    case OP_NOP8                   : return "OP_NOP8";
    case OP_NOP9                   : return "OP_NOP9";
    case OP_NOP10                  : return "OP_NOP10";



    // multi-byte opcodes
    case OP_SINGLEBYTE_END         : return "OP_SINGLEBYTE_END";
    case OP_DOUBLEBYTE_BEGIN       : return "OP_DOUBLEBYTE_BEGIN";
    case OP_PUBKEY                 : return "OP_PUBKEY";
    case OP_PUBKEYHASH             : return "OP_PUBKEYHASH";


    case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";
    default:
        return "UNKNOWN_OPCODE";
    }
};




inline string ValueString(const vector<unsigned char>& vch)
{
    if (vch.size() <= 4)
        return strprintf("%d", CBigNum(vch).getint());
    else
        return HexNumStr(vch.begin(), vch.end());
        //return string("(") + HexStr(vch.begin(), vch.end()) + string(")");
}

inline string StackString(const vector<vector<unsigned char> >& vStack)
{
    string str;
    foreach(const vector<unsigned char>& vch, vStack)
    {
        if (!str.empty())
            str += " ";
        str += ValueString(vch);
    }
    return str;
}









class CScript : public vector<unsigned char>
{
protected:
    CScript& push_int64(int64 n)
    {
        if (n == -1 || (n >= 1 && n <= 16))
        {
            push_back(n + (OP_1 - 1));
        }
        else
        {
            CBigNum bn(n);
            *this << bn.getvch();
        }
        return (*this);
    }

    CScript& push_uint64(uint64 n)
    {
        if (n == -1 || (n >= 1 && n <= 16))
        {
            push_back(n + (OP_1 - 1));
        }
        else
        {
            CBigNum bn(n);
            *this << bn.getvch();
        }
        return (*this);
    }

public:
    CScript() { }
    CScript(const CScript& b) : vector<unsigned char>(b.begin(), b.end()) { }
    CScript(const_iterator pbegin, const_iterator pend) : vector<unsigned char>(pbegin, pend) { }
#ifndef _MSC_VER
    CScript(const unsigned char* pbegin, const unsigned char* pend) : vector<unsigned char>(pbegin, pend) { }
#endif

    CScript& operator+=(const CScript& b)
    {
        insert(end(), b.begin(), b.end());
        return *this;
    }

    friend CScript operator+(const CScript& a, const CScript& b)
    {
        CScript ret = a;
        ret += b;
        return (ret);
    }


    explicit CScript(char b)           { operator<<(b); }
    explicit CScript(short b)          { operator<<(b); }
    explicit CScript(int b)            { operator<<(b); }
    explicit CScript(long b)           { operator<<(b); }
    explicit CScript(int64 b)          { operator<<(b); }
    explicit CScript(unsigned char b)  { operator<<(b); }
    explicit CScript(unsigned int b)   { operator<<(b); }
    explicit CScript(unsigned short b) { operator<<(b); }
    explicit CScript(unsigned long b)  { operator<<(b); }
    explicit CScript(uint64 b)         { operator<<(b); }

    explicit CScript(opcodetype b)     { operator<<(b); }
    explicit CScript(const uint256& b) { operator<<(b); }
    explicit CScript(const CBigNum& b) { operator<<(b); }
    explicit CScript(const vector<unsigned char>& b) { operator<<(b); }


    CScript& operator<<(char b)           { return (push_int64(b)); }
    CScript& operator<<(short b)          { return (push_int64(b)); }
    CScript& operator<<(int b)            { return (push_int64(b)); }
    CScript& operator<<(long b)           { return (push_int64(b)); }
    CScript& operator<<(int64 b)          { return (push_int64(b)); }
    CScript& operator<<(unsigned char b)  { return (push_uint64(b)); }
    CScript& operator<<(unsigned int b)   { return (push_uint64(b)); }
    CScript& operator<<(unsigned short b) { return (push_uint64(b)); }
    CScript& operator<<(unsigned long b)  { return (push_uint64(b)); }
    CScript& operator<<(uint64 b)         { return (push_uint64(b)); }

    CScript& operator<<(opcodetype opcode)
    {
        if (opcode <= OP_SINGLEBYTE_END)
        {
            insert(end(), (unsigned char)opcode);
        }
        else
        {
            assert(opcode >= OP_DOUBLEBYTE_BEGIN);
            insert(end(), (unsigned char)(opcode >> 8));
            insert(end(), (unsigned char)(opcode & 0xFF));
        }
        return (*this);
    }

    CScript& operator<<(const uint160& b)
    {
        insert(end(), sizeof(b));
        insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
        return (*this);
    }

    CScript& operator<<(const uint256& b)
    {
        insert(end(), sizeof(b));
        insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
        return (*this);
    }

    CScript& operator<<(const CBigNum& b)
    {
        *this << b.getvch();
        return (*this);
    }

    CScript& operator<<(const vector<unsigned char>& b)
    {
        if (b.size() < OP_PUSHDATA1)
        {
            insert(end(), (unsigned char)b.size());
        }
        else if (b.size() <= 0xff)
        {
            insert(end(), OP_PUSHDATA1);
            insert(end(), (unsigned char)b.size());
        }
        else
        {
            insert(end(), OP_PUSHDATA2);
            unsigned short nSize = b.size();
            insert(end(), (unsigned char*)&nSize, (unsigned char*)&nSize + sizeof(nSize));
        }
        insert(end(), b.begin(), b.end());
        return (*this);
    }

    CScript& operator<<(const CScript& b)
    {
        // I'm not sure if this should push the script or concatenate scripts.
        // If there's ever a use for pushing a script onto a script, delete this member fn
        assert(("warning: pushing a CScript onto a CScript with << is probably not intended, use + to concatenate", false));
        return (*this);
    }


    bool GetOp(iterator& pc, opcodetype& opcodeRet, vector<unsigned char>& vchRet)
    {
         // Wrapper so it can be called with either iterator or const_iterator
         const_iterator pc2 = pc;
         bool fRet = GetOp(pc2, opcodeRet, vchRet);
         pc = begin() + (pc2 - begin());
         return fRet;
    }

    bool GetOp(const_iterator& pc, opcodetype& opcodeRet, vector<unsigned char>& vchRet) const
    {
        opcodeRet = OP_INVALIDOPCODE;
        vchRet.clear();
        if (pc >= end())
            return false;

        // Read instruction
        unsigned int opcode = *pc++;
        if (opcode >= OP_SINGLEBYTE_END)
        {
            if (pc + 1 > end())
                return false;
            opcode <<= 8;
            opcode |= *pc++;
        }

        // Immediate operand
        if (opcode <= OP_PUSHDATA4)
        {
            unsigned int nSize = opcode;
            if (opcode == OP_PUSHDATA1)
            {
                if (pc + 1 > end())
                    return false;
                nSize = *pc++;
            }
            else if (opcode == OP_PUSHDATA2)
            {
                if (pc + 2 > end())
                    return false;
                nSize = 0;
                memcpy(&nSize, &pc[0], 2);
                pc += 2;
            }
            else if (opcode == OP_PUSHDATA4)
            {
                if (pc + 4 > end())
                    return false;
                memcpy(&nSize, &pc[0], 4);
                pc += 4;
            }
            if (pc + nSize > end())
                return false;
            vchRet.assign(pc, pc + nSize);
            pc += nSize;
        }

        opcodeRet = (opcodetype)opcode;
        return true;
    }


    void FindAndDelete(const CScript& b)
    {
        iterator pc = begin();
        opcodetype opcode;
        vector<unsigned char> vchPushValue;
        int count = 0;
        do
        {
            while (end() - pc >= b.size() && memcmp(&pc[0], &b[0], b.size()) == 0)
            {
                erase(pc, pc + b.size());
                count++;
            }
        }
        while (GetOp(pc, opcode, vchPushValue));
        //printf("FindAndDeleted deleted %d items\n", count); /// debug
    }


    uint160 GetBitcoinAddressHash160() const
    {
        opcodetype opcode;
        vector<unsigned char> vch;
        CScript::const_iterator pc = begin();
        if (!GetOp(pc, opcode, vch) || opcode != OP_DUP) return 0;
        if (!GetOp(pc, opcode, vch) || opcode != OP_HASH160) return 0;
        if (!GetOp(pc, opcode, vch) || vch.size() != sizeof(uint160)) return 0;
        uint160 hash160 = uint160(vch);
        if (!GetOp(pc, opcode, vch) || opcode != OP_EQUALVERIFY) return 0;
        if (!GetOp(pc, opcode, vch) || opcode != OP_CHECKSIG) return 0;
        if (pc != end()) return 0;
        return hash160;
    }

    string GetBitcoinAddress() const
    {
        uint160 hash160 = GetBitcoinAddressHash160();
        if (hash160 == 0)
            return "";
        return Hash160ToAddress(hash160);
    }

    void SetBitcoinAddress(const uint160& hash160)
    {
        this->clear();
        *this << OP_DUP << OP_HASH160 << hash160 << OP_EQUALVERIFY << OP_CHECKSIG;
    }

    void SetBitcoinAddress(const vector<unsigned char>& vchPubKey)
    {
        SetBitcoinAddress(Hash160(vchPubKey));
    }

    bool SetBitcoinAddress(const string& strAddress)
    {
        this->clear();
        uint160 hash160;
        if (!AddressToHash160(strAddress, hash160))
            return false;
        SetBitcoinAddress(hash160);
        return true;
    }


    void PrintHex() const
    {
        printf("CScript(%s)\n", HexStr(begin(), end()).c_str());
    }

    string ToString() const
    {
        string str;
        opcodetype opcode;
        vector<unsigned char> vch;
        const_iterator it = begin();
        while (GetOp(it, opcode, vch))
        {
            if (!str.empty())
                str += " ";
            if (opcode <= OP_PUSHDATA4)
                str += ValueString(vch);
            else
                str += GetOpName(opcode);
        }
        return str;
    }

    void print() const
    {
        printf("%s\n", ToString().c_str());
    }
};








uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
bool IsMine(const CScript& scriptPubKey);
bool ExtractPubKey(const CScript& scriptPubKey, bool fMineOnly, vector<unsigned char>& vchPubKeyRet);
bool ExtractHash160(const CScript& scriptPubKey, uint160& hash160Ret);
bool SignSignature(const CTransaction& txFrom, CTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL, CScript scriptPrereq=CScript());
bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, int nHashType=0);
