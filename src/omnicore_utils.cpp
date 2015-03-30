// This file serves to seperate utility functions from the main mastercore.cpp/h files.

#include "mastercore.h"
#include "omnicore_encoding.h"
#include "omnicore_utils.h"
#include "openssl/sha.h"
#include "wallet.h"

#include <boost/algorithm/string.hpp>

#include <string>
#include <vector>

void prepareObfuscatedHashes(const string &address, string (&ObfsHashes)[1+MAX_SHA256_OBFUSCATION_TIMES])
{
    unsigned char sha_input[128];
    unsigned char sha_result[128];
    std::vector<unsigned char> vec_chars;
    strcpy((char *)sha_input, address.c_str());
    // do only as many re-hashes as there are mastercoin packets, 255 per spec
    for (unsigned int j = 1; j<=MAX_SHA256_OBFUSCATION_TIMES;j++)
    {
        SHA256(sha_input, strlen((const char *)sha_input), sha_result);
        vec_chars.resize(32);
        memcpy(&vec_chars[0], &sha_result[0], 32);
        ObfsHashes[j] = HexStr(vec_chars);
        boost::to_upper(ObfsHashes[j]); // uppercase per spec
        strcpy((char *)sha_input, ObfsHashes[j].c_str());
    }
}

int64_t GetDustLimit(const CScript& scriptPubKey)
{
    // The total size is based on a typical scriptSig size of 148 byte,
    // 8 byte accounted for the size of output value and the serialized
    // size of scriptPubKey.
    size_t nSize = ::GetSerializeSize(scriptPubKey, SER_DISK, 0) + 156;

    // The minimum relay fee dictates a threshold value under which a
    // transaction won't be relayed.
    int64_t nRelayTxFee = (::minRelayTxFee).GetFee(nSize);

    // A transaction is considered as "dust", if less than 1/3 of the
    // minimum fee required to relay a transaction is spent by one of
    // it's outputs. The minimum relay fee is defined per 1000 byte.
    int64_t nDustLimit = nRelayTxFee * 3;

    return nDustLimit;
}


