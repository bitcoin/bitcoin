#include "script.h"
#include "version.h"
#include "core.h"

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

// From util.cpp
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



bool VerifyScript(const unsigned char *scriptPubKey, const unsigned int scriptPubKeyLen,
                             const unsigned char *txTo,         const unsigned int txToLen,
                             const unsigned int nIn, const unsigned int flags)
{
    try {
        if (!scriptPubKey || !txTo)
            return false;

        CTransaction tx;
        CDataStream stream(std::vector<unsigned char>(txTo, txTo + txToLen), SER_NETWORK, PROTOCOL_VERSION);
        stream >> tx;

        if (nIn >= tx.vin.size())
            return false;

        return VerifyScript(tx.vin[nIn].scriptSig,
                            CScript(scriptPubKey, scriptPubKey + scriptPubKeyLen),
                            tx, nIn, flags, 0);
    } catch (std::exception &e) {
        return false; // Error deserializing
    }
}
