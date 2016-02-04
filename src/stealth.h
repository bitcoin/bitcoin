// Copyright (c) 2014 The ShadowCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef GAMEUNITS_STEALTH_H
#define GAMEUNITS_STEALTH_H

#include <stdlib.h> 
#include <stdio.h> 
#include <vector>
#include <inttypes.h>

#include "util.h"
#include "serialize.h"
#include "key.h"


typedef std::vector<uint8_t> data_chunk;

const uint32_t MAX_STEALTH_NARRATION_SIZE = 48;

const size_t ec_secret_size = 32;
const size_t ec_compressed_size = 33;
const size_t ec_uncompressed_size = 65;

const uint8_t stealth_version_byte = 0x28;

typedef struct ec_secret { uint8_t e[ec_secret_size]; } ec_secret;
typedef data_chunk ec_point;

typedef uint32_t stealth_bitfield;

struct stealth_prefix
{
    uint8_t number_bits;
    stealth_bitfield bitfield;
};

template <typename T, typename Iterator>
T from_big_endian(Iterator in)
{
    //VERIFY_UNSIGNED(T);
    T out = 0;
    size_t i = sizeof(T);
    while (0 < i)
        out |= static_cast<T>(*in++) << (8 * --i);
    return out;
}

template <typename T, typename Iterator>
T from_little_endian(Iterator in)
{
    //VERIFY_UNSIGNED(T);
    T out = 0;
    size_t i = 0;
    while (i < sizeof(T))
        out |= static_cast<T>(*in++) << (8 * i++);
    return out;
}

class CStealthAddress
{
public:
    CStealthAddress()
    {
        options = 0;
    }
    
    uint8_t options;
    ec_point scan_pubkey;
    ec_point spend_pubkey;
    //std::vector<ec_point> spend_pubkeys;
    size_t number_signatures;
    stealth_prefix prefix;
    
    mutable std::string label;
    data_chunk scan_secret;
    data_chunk spend_secret;
    
    bool SetEncoded(const std::string& encodedAddress);
    std::string Encoded() const;
    
    int SetScanPubKey(CPubKey pk);
    
    
    bool operator <(const CStealthAddress& y) const
    {
        return memcmp(&scan_pubkey[0], &y.scan_pubkey[0], ec_compressed_size) < 0;
    }
    
    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->options);
        READWRITE(this->scan_pubkey);
        READWRITE(this->spend_pubkey);
        READWRITE(this->label);
        
        READWRITE(this->scan_secret);
        READWRITE(this->spend_secret);
    );
    
};

void AppendChecksum(data_chunk& data);

bool VerifyChecksum(const data_chunk& data);

int GenerateRandomSecret(ec_secret& out);

int SecretToPublicKey(const ec_secret& secret, ec_point& out);

int StealthSecret(ec_secret& secret, ec_point& pubkey, const ec_point& pkSpend, ec_secret& sharedSOut, ec_point& pkOut);
int StealthSecretSpend(ec_secret& scanSecret, ec_point& ephemPubkey, ec_secret& spendSecret, ec_secret& secretOut);
int StealthSharedToSecretSpend(ec_secret& sharedS, ec_secret& spendSecret, ec_secret& secretOut);

bool IsStealthAddress(const std::string& encodedAddress);


#endif  // GAMEUNITS_STEALTH_H

