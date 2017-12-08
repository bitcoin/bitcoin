/**
 * @file utils.cpp
 *
 * This file serves to seperate utility functions from the main omnicore.cpp
 * and omnicore.h files.
 */

#include "omnicore/utils.h"

#include "base58.h"
#include "utilstrencodings.h"

// TODO: use crypto/sha256 instead of openssl
#include "openssl/sha.h"

#include "omnicore/log.h"
#include "omnicore/script.h"

#include <boost/algorithm/string.hpp>

#include <assert.h>
#include <string.h>
#include <string>
#include <vector>

/**
 * Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))).
 *
 * It is expected that the seed has a length of less than 128 characters.
 *
 * @see The class B transaction encoding specification:
 * https://github.com/mastercoin-MSC/spec#class-b-transactions-also-known-as-the-multisig-method
 *
 * @param strSeed[in]      A seed used for the obfuscation
 * @param hashCount[in]    How many hashes to generate (number of packets to debofuscate)
 * @param vstrHashes[out]  The generated hashes
 */
void PrepareObfuscatedHashes(const std::string& strSeed, int hashCount, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES])
{
    unsigned char sha_input[128];
    unsigned char sha_result[128];
    std::vector<unsigned char> vec_chars;

    assert(strSeed.size() < sizeof(sha_input));
    strcpy((char *)sha_input, strSeed.c_str());

    if (hashCount > MAX_SHA256_OBFUSCATION_TIMES) hashCount = MAX_SHA256_OBFUSCATION_TIMES;

    // Do only as many re-hashes as there are data packets, 255 per specification
    for (int j = 1; j <= hashCount; ++j)
    {
        SHA256(sha_input, strlen((const char *)sha_input), sha_result);
        vec_chars.resize(32);
        memcpy(&vec_chars[0], &sha_result[0], 32);
        vstrHashes[j] = HexStr(vec_chars);
        boost::to_upper(vstrHashes[j]); // Convert to upper case characters

        assert(vstrHashes[j].size() < sizeof(sha_input));
        strcpy((char *)sha_input, vstrHashes[j].c_str());
    }
}

std::string HashToAddress(unsigned char version, const uint160& hash)
{
    CBitcoinAddress address;
    if (version == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS)[0]) {
        CKeyID keyId = hash;
        address.Set(keyId);
        return address.ToString();
    } else if (version == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS)[0]) {
        CScriptID scriptId = hash;
        address.Set(scriptId);
        return address.ToString();
    }

    return "";
}

std::vector<unsigned char> AddressToBytes(const std::string& address)
{
    std::vector<unsigned char> addressBytes;
    bool success = DecodeBase58(address, addressBytes);
    if (!success) {
        PrintToLog("ERROR: failed to decode address %s.\n", address);
    }
    if (addressBytes.size() == 25) {
        addressBytes.resize(21); // truncate checksum
    } else {
        PrintToLog("ERROR: unexpected size from DecodeBase58 when decoding address %s.\n", address);
    }

    return addressBytes;
}
