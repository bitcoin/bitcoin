#include "script.h"

using namespace std;

// Same as scriptutils.cpp but without the CSignatureCache
bool CheckSig(vector<unsigned char> vchSig, const vector<unsigned char> &vchPubKey, const CScript &scriptCode,
              const CTransaction& txTo, unsigned int nIn, int nHashType, int flags)
{
    CPubKey pubkey(vchPubKey);
    if (!pubkey.IsValid())
        return false;

    // Hash type is one byte tacked on to the end of the signature
    if (vchSig.empty())
        return false;
    if (nHashType == 0)
        nHashType = vchSig.back();
    else if (nHashType != vchSig.back())
        return false;
    vchSig.pop_back();

    uint256 sighash = SignatureHash(scriptCode, txTo, nIn, nHashType);

    if (!pubkey.Verify(sighash, vchSig))
        return false;

    return true;
}

int LogPrintStr(const string&)
{
    return 0;
}

bool LogAcceptCategory(const char*)
{
    return false;
}

int64_t GetTime()
{
    return time(NULL);
}


