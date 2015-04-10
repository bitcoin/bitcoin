/**
 * @file omnicore_utils.cpp
 *
 * This file serves to seperate utility functions from the main mastercore.cpp
 * and mastercore.h files.
 */

#include "omnicore_utils.h"

#include "mastercore.h" // MAX_SHA256_OBFUSCATION_TIMES

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "utilstrencodings.h"

// TODO: use crypto/sha256 instead of openssl
#include "openssl/sha.h"

#include <boost/algorithm/string.hpp>

#include <string.h>
#include <string>
#include <vector>

/** The minimum transaction relay fee. */
extern CFeeRate minRelayTxFee;

/**
 * Generates hashes used for obfuscation via ToUpper(HexStr(SHA256(x))).
 *
 * @see The class B transaction encoding specification:
 * https://github.com/mastercoin-MSC/spec#class-b-transactions-also-known-as-the-multisig-method
 *
 * @param strSeed[in]      A seed used for the obfuscation
 * @param vstrHashes[out]  The generated hashes
 */
void PrepareObfuscatedHashes(const std::string& strSeed, std::string(&vstrHashes)[1+MAX_SHA256_OBFUSCATION_TIMES])
{
    unsigned char sha_input[128];
    unsigned char sha_result[128];
    std::vector<unsigned char> vec_chars;
    strcpy((char *)sha_input, strSeed.c_str());
    // Do only as many re-hashes as there are data packets, 255 per specification
    for (unsigned int j = 1; j <= MAX_SHA256_OBFUSCATION_TIMES; ++j)
    {
        SHA256(sha_input, strlen((const char *)sha_input), sha_result);
        vec_chars.resize(32);
        memcpy(&vec_chars[0], &sha_result[0], 32);
        vstrHashes[j] = HexStr(vec_chars);
        boost::to_upper(vstrHashes[j]); // Convert to upper case characters
        strcpy((char *)sha_input, vstrHashes[j].c_str());
    }
}

/**
 * Determines the minimum output amount to be spent by an output, based on the
 * scriptPubKey size in relation to the minimum relay fee.
 *
 * @param scriptPubKey[in]  The scriptPubKey
 * @return The dust threshold value
 */
int64_t GetDustLimit(const CScript& scriptPubKey)
{
    // The total size is based on a typical scriptSig size of 148 byte,
    // 8 byte accounted for the size of output value and the serialized
    // size of scriptPubKey.
    size_t nSize = ::GetSerializeSize(scriptPubKey, SER_DISK, 0) + 156u;

    // The minimum relay fee dictates a threshold value under which a
    // transaction won't be relayed.
    int64_t nRelayTxFee = minRelayTxFee.GetFee(nSize);

    // A transaction is considered as "dust", if less than 1/3 of the
    // minimum fee required to relay a transaction is spent by one of
    // it's outputs. The minimum relay fee is defined per 1000 byte.
    return nRelayTxFee * 3;
}


