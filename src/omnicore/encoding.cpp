#include "omnicore/encoding.h"

#include "omnicore/omnicore.h"
#include "omnicore/script.h"
#include "omnicore/utils.h"

#include "base58.h"
#include "pubkey.h"
#include "random.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

/**
 * Embedds a payload in obfuscated multisig outputs, and adds an Exodus marker output.
 *
 * @see The class B transaction encoding specification:
 * https://github.com/mastercoin-MSC/spec#class-b-transactions-also-known-as-the-multisig-method
 */
bool OmniCore_Encode_ClassB(const std::string& senderAddress, const CPubKey& redeemingPubKey,
        const std::vector<unsigned char>& vchPayload, std::vector<std::pair<CScript, int64_t> >& vecOutputs)
{
    unsigned int nRemainingBytes = vchPayload.size();
    unsigned int nNextByte = 0;
    unsigned char chSeqNum = 1;
    std::string strObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
    PrepareObfuscatedHashes(senderAddress, MAX_SHA256_OBFUSCATION_TIMES, strObfuscatedHashes);
    while (nRemainingBytes > 0) {
        int nKeys = 1; // Assume one key of data, because we have data remaining
        if (nRemainingBytes > (PACKET_SIZE - 1)) { nKeys += 1; } // ... or enough data to embed in 2 keys
        std::vector<CPubKey> vKeys;
        vKeys.push_back(redeemingPubKey); // Always include the redeeming pubkey
        for (int i = 0; i < nKeys; i++) {
            // Add up to 30 bytes of data
            unsigned int nCurrentBytes = nRemainingBytes < (PACKET_SIZE - 1) ? nRemainingBytes: (PACKET_SIZE - 1);
            std::vector<unsigned char> vchFakeKey;
            vchFakeKey.insert(vchFakeKey.end(), chSeqNum); // Add sequence number
            vchFakeKey.insert(vchFakeKey.end(), vchPayload.begin() + nNextByte,
                                                vchPayload.begin() + nNextByte + nCurrentBytes);
            vchFakeKey.resize(PACKET_SIZE); // Pad to 31 total bytes with zeros
            nNextByte += nCurrentBytes;
            nRemainingBytes -= nCurrentBytes;
            std::vector<unsigned char> vchHash = ParseHex(strObfuscatedHashes[chSeqNum]);
            for (size_t j = 0; j < PACKET_SIZE; j++) { // Xor in the obfuscation
                vchFakeKey[j] = vchFakeKey[j] ^ vchHash[j];
            }
            vchFakeKey.insert(vchFakeKey.begin(), 0x02); // Prepend a public key prefix
            vchFakeKey.resize(33);
            CPubKey pubKey;
            unsigned char chRandom = static_cast<unsigned char>(GetRand(256));
            for (int j = 0; i < 256 ; j++) { // Fix ECDSA coodinate
                vchFakeKey[32] = chRandom;
                pubKey = CPubKey(vchFakeKey);
                if (pubKey.IsFullyValid()) break;
                ++chRandom; // ... but cycle no more than 256 times to find a valid point
            }
            vKeys.push_back(pubKey);
            chSeqNum++;
        }

        // Push back a bare multisig output with obfuscated data
        CScript scriptMultisigOut = GetScriptForMultisig(1, vKeys);
        vecOutputs.push_back(std::make_pair(scriptMultisigOut, GetDustThreshold(scriptMultisigOut)));
    }

    // Add the Exodus marker output
    CScript scriptExodusOutput = GetScriptForDestination(ExodusAddress().Get());
    vecOutputs.push_back(std::make_pair(scriptExodusOutput, GetDustThreshold(scriptExodusOutput)));
    return true;
}

/**
 * Embedds a payload in an OP_RETURN output, prefixed with a transaction marker.
 *
 * The request is rejected, if the size of the payload with marker is larger than
 * the allowed data carrier size ("-datacarriersize=n").
 */
bool OmniCore_Encode_ClassC(const std::vector<unsigned char>& vchPayload,
        std::vector<std::pair <CScript, int64_t> >& vecOutputs)
{
    std::vector<unsigned char> vchData;
    std::vector<unsigned char> vchOmBytes = GetOmMarker();
    vchData.insert(vchData.end(), vchOmBytes.begin(), vchOmBytes.end());
    vchData.insert(vchData.end(), vchPayload.begin(), vchPayload.end());
    if (vchData.size() > nMaxDatacarrierBytes) { return false; }

    CScript script;
    script << OP_RETURN << vchData;
    vecOutputs.push_back(std::make_pair(script, 0));
    return true;
}
