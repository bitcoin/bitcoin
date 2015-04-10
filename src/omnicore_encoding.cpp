// This file serves to seperate encoding of data outputs from the main mastercore.cpp/h files.

#include "mastercore.h"
#include "omnicore_encoding.h"
#include "omnicore_utils.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "pubkey.h"
#include "script/script.h"
#include "script/standard.h"
#include "utilstrencodings.h"
#include "random.h"
#include "wallet.h"
#include "openssl/sha.h"

#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>

bool OmniCore_Encode_ClassB(const std::string& senderAddress, const CPubKey& redeemingPubKey, const std::vector<unsigned char>& vecPayload, std::vector<std::pair <CScript,int64_t> >& vecOutputs)
{
    int nRemainingBytes = vecPayload.size();
    int nNextByte = 0;
    unsigned char seqNum = 1;
    std::string strObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
    PrepareObfuscatedHashes(senderAddress, strObfuscatedHashes);
    while (nRemainingBytes > 0) {
        int nKeys = 1; // assume one key of data since we have data remaining
        if (nRemainingBytes > (PACKET_SIZE - 1)) { nKeys += 1; } // we have enough data for 2 keys in this output
        std::vector<CPubKey> keys;
        keys.push_back(redeemingPubKey); // always include the redeeming pubkey
        for (int i = 0; i < nKeys; i++) {
            std::vector<unsigned char> fakeKey;
            fakeKey.push_back(seqNum); // add sequence number
            int numBytes = nRemainingBytes < (PACKET_SIZE - 1) ? nRemainingBytes: (PACKET_SIZE - 1); // add up to 30 bytes of data
            fakeKey.insert(fakeKey.end(), vecPayload.begin() + nNextByte, vecPayload.begin() + nNextByte + numBytes);
            nNextByte += numBytes;
            nRemainingBytes -= numBytes;
            while (fakeKey.size() < PACKET_SIZE) { fakeKey.push_back(0); } // pad to 31 total bytes with zeros
            std::vector<unsigned char>hash = ParseHex(strObfuscatedHashes[seqNum]);
            for (int j = 0; j < PACKET_SIZE; j++) { // xor in the obfuscation
                fakeKey[j] = fakeKey[j] ^ hash[j];
            }
            fakeKey.insert(fakeKey.begin(), 2); // prepend the 2
            CPubKey pubKey;
            fakeKey.resize(33);
            unsigned char random_byte = (unsigned char)(GetRand(256)); // fix up the ecdsa code point
            for (int j = 0; i < 256 ; j++) {
                fakeKey[32] = random_byte;
                pubKey = CPubKey(fakeKey);
                if (pubKey.IsFullyValid()) break;
                ++random_byte; // cycle 256 times, if we must to find a valid ECDSA point
            }
            keys.push_back(pubKey);
            seqNum++;
        }
        CScript multisig_output = GetScriptForMultisig(1, keys);
        vecOutputs.push_back(make_pair(multisig_output, GetDustLimit(multisig_output)));
    }
    CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(exodus_address).Get());
    vecOutputs.push_back(make_pair(scriptPubKey, GetDustLimit(scriptPubKey))); // add the Exodus marker
    return true;
}

bool OmniCore_Encode_ClassC(const std::vector<unsigned char>& vecPayload, std::vector<std::pair <CScript,int64_t> >& vecOutputs)
{
    const unsigned char bytes[] = {0x6f,0x6d}; // define Omni marker bytes
    if (vecPayload.size() > nMaxDatacarrierBytes-sizeof(bytes)/sizeof(bytes[0])) { return false; } // we shouldn't see this since classAgnostic_send handles size vs class, but include check here for safety
    std::vector<unsigned char> omniBytesPlusData(bytes, bytes+sizeof(bytes)/sizeof(bytes[0]));
    omniBytesPlusData.insert(omniBytesPlusData.end(), vecPayload.begin(), vecPayload.end());
    CScript script;
    script << OP_RETURN << omniBytesPlusData;
    vecOutputs.push_back(make_pair(script, 0));
    return true;
}
