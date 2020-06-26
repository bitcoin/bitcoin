/**
 * @file parsing.cpp
 *
 * This file contains parsing and transaction decoding related functions.
 */

#include <omnicore/parsing.h>

#include <omnicore/log.h>
#include <omnicore/script.h>

#include <base58.h>
#include <key_io.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <boost/algorithm/string.hpp>

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

/**
 * Checks whether the system uses big or little endian.
 */
static bool isBigEndian()
{
  union
  {
    uint32_t i;
    char c[4];
  } bint = {0x01020304};

  return 1 == bint.c[0];
}

/**
 * Swaps byte order of 16 bit wide numbers on little-endian systems.
 */
void SwapByteOrder16(uint16_t& us)
{
  if (isBigEndian()) return;

    us = (us >> 8) |
         (us << 8);
}

/**
 * Swaps byte order of 32 bit wide numbers on little-endian systems.
 */
void SwapByteOrder32(uint32_t& ui)
{
  if (isBigEndian()) return;

    ui = (ui >> 24) |
         ((ui << 8) & 0x00FF0000) |
         ((ui >> 8) & 0x0000FF00) |
         (ui << 24);
}

/**
 * Swaps byte order of 64 bit wide numbers on little-endian systems.
 */
void SwapByteOrder64(uint64_t& ull)
{
  if (isBigEndian()) return;

    ull = (ull >> 56) |
          ((ull << 40) & 0x00FF000000000000) |
          ((ull << 24) & 0x0000FF0000000000) |
          ((ull << 8)  & 0x000000FF00000000) |
          ((ull >> 8)  & 0x00000000FF000000) |
          ((ull >> 24) & 0x0000000000FF0000) |
          ((ull >> 40) & 0x000000000000FF00) |
          (ull << 56);
}

/**
 * Determines the Bitcoin address associated with a given hash and version.
 */
std::string HashToAddress(unsigned char version, const uint160& hash)
{
    if (version == Params().Base58Prefix(CChainParams::PUBKEY_ADDRESS)[0]) {
        CKeyID keyId(hash);
        return EncodeDestination(keyId);
    } else if (version == Params().Base58Prefix(CChainParams::SCRIPT_ADDRESS)[0]) {
        CScriptID scriptId(hash);
        return EncodeDestination(scriptId);
    }

    return "";
}

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
        CSHA256().Write(sha_input, strlen((const char *)sha_input)).Finalize(sha_result);
        vec_chars.resize(32);
        memcpy(&vec_chars[0], &sha_result[0], 32);
        vstrHashes[j] = HexStr(vec_chars);
        boost::to_upper(vstrHashes[j]); // Convert to upper case characters

        assert(vstrHashes[j].size() < sizeof(sha_input));
        strcpy((char *)sha_input, vstrHashes[j].c_str());
    }
}


// Move ParseTransaction into this file
