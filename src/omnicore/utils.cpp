/**
 * @file utils.cpp
 *
 * This file serves to seperate utility functions from the main omnicore.cpp
 * and omnicore.h files.
 */

#include "omnicore/utils.h"

#include "utilstrencodings.h"

// TODO: use crypto/sha256 instead of openssl
#include "openssl/sha.h"

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
    for (unsigned int j = 1; j <= hashCount; ++j)
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
