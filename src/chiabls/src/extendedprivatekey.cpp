// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <cstring>
#include "extendedprivatekey.hpp"
#include "util.hpp"
#include "bls.hpp"
namespace bls {

ExtendedPrivateKey ExtendedPrivateKey::FromSeed(const uint8_t* seed,
                                                size_t seedLen) {
    // "BLS HD seed" in ascii
    const uint8_t prefix[] = {66, 76, 83, 32, 72, 68, 32, 115, 101, 101, 100};

    uint8_t* hashInput = Util::SecAlloc<uint8_t>(seedLen + 1);
    std::memcpy(hashInput, seed, seedLen);

    // 32 bytes for secret key, and 32 bytes for chaincode
    uint8_t* ILeft = Util::SecAlloc<uint8_t>(
            PrivateKey::PRIVATE_KEY_SIZE);
    uint8_t IRight[ChainCode::CHAIN_CODE_SIZE];

    // Hash the seed into 64 bytes, half will be sk, half will be cc
    hashInput[seedLen] = 0;
    md_hmac(ILeft, hashInput, seedLen + 1, prefix, sizeof(prefix));

    hashInput[seedLen] = 1;
    md_hmac(IRight, hashInput, seedLen + 1, prefix, sizeof(prefix));

    // Make sure private key is less than the curve order
    bn_t* skBn = Util::SecAlloc<bn_t>(1);
    bn_t order;
    bn_new(order);
    g1_get_ord(order);

    bn_new(*skBn);
    bn_read_bin(*skBn, ILeft, PrivateKey::PRIVATE_KEY_SIZE);
    bn_mod_basic(*skBn, *skBn, order);
    bn_write_bin(ILeft, PrivateKey::PRIVATE_KEY_SIZE, *skBn);

    ExtendedPrivateKey esk(ExtendedPublicKey::VERSION, 0, 0, 0,
                           ChainCode::FromBytes(IRight),
                           PrivateKey::FromBytes(ILeft));

    Util::SecFree(skBn);
    Util::SecFree(ILeft);
    Util::SecFree(hashInput);
    return esk;
}

ExtendedPrivateKey ExtendedPrivateKey::FromBytes(const uint8_t* serialized) {
    uint32_t version = Util::FourBytesToInt(serialized);
    uint32_t depth = serialized[4];
    uint32_t parentFingerprint = Util::FourBytesToInt(serialized + 5);
    uint32_t childNumber = Util::FourBytesToInt(serialized + 9);
    const uint8_t* ccPointer = serialized + 13;
    const uint8_t* skPointer = ccPointer + ChainCode::CHAIN_CODE_SIZE;

    ExtendedPrivateKey esk(version, depth, parentFingerprint, childNumber,
                          ChainCode::FromBytes(ccPointer),
                          PrivateKey::FromBytes(skPointer));
    return esk;
}

ExtendedPrivateKey ExtendedPrivateKey::PrivateChild(uint32_t i) const {
    if (depth >= 255) {
        throw std::string("Cannot go further than 255 levels");
    }
    // Hardened keys have i >= 2^31. Non-hardened have i < 2^31
    uint32_t cmp = (1 << 31);
    bool hardened = i >= cmp;

    uint8_t* ILeft = Util::SecAlloc<uint8_t>(
            PrivateKey::PRIVATE_KEY_SIZE);
    uint8_t IRight[ChainCode::CHAIN_CODE_SIZE];

    // Chain code is used as hmac key
    uint8_t hmacKey[ChainCode::CHAIN_CODE_SIZE];
    chainCode.Serialize(hmacKey);

    size_t inputLen = hardened ? PrivateKey::PRIVATE_KEY_SIZE + 4 + 1
                                : PublicKey::PUBLIC_KEY_SIZE + 4 + 1;
    // Hmac input includes sk or pk, int i, and byte with 0 or 1
    uint8_t* hmacInput = Util::SecAlloc<uint8_t>(inputLen);

    // Fill the input with the required data
    if (hardened) {
        sk.Serialize(hmacInput);
        Util::IntToFourBytes(hmacInput + PrivateKey::PRIVATE_KEY_SIZE, i);
    } else {
        sk.GetPublicKey().Serialize(hmacInput);
        Util::IntToFourBytes(hmacInput + PublicKey::PUBLIC_KEY_SIZE, i);
    }
    hmacInput[inputLen - 1] = 0;

    md_hmac(ILeft, hmacInput, inputLen,
                    hmacKey, ChainCode::CHAIN_CODE_SIZE);

    // Change 1 byte to generate a different sequence for chaincode
    hmacInput[inputLen - 1] = 1;

    md_hmac(IRight, hmacInput, inputLen,
                    hmacKey, ChainCode::CHAIN_CODE_SIZE);

    PrivateKey newSk = PrivateKey::FromBytes(ILeft, true);
    newSk = PrivateKey::AggregateInsecure({sk, newSk});

    ExtendedPrivateKey esk(version, depth + 1,
                           sk.GetPublicKey().GetFingerprint(), i,
                           ChainCode::FromBytes(IRight),
                           newSk);

    Util::SecFree(ILeft);
    Util::SecFree(hmacInput);

    return esk;
}

uint32_t ExtendedPrivateKey::GetVersion() const {
    return version;
}

uint8_t ExtendedPrivateKey::GetDepth() const {
    return depth;
}

uint32_t ExtendedPrivateKey::GetParentFingerprint() const {
    return parentFingerprint;
}

uint32_t ExtendedPrivateKey::GetChildNumber() const {
    return childNumber;
}

ExtendedPublicKey ExtendedPrivateKey::PublicChild(uint32_t i) const {
    return PrivateChild(i).GetExtendedPublicKey();
}

PrivateKey ExtendedPrivateKey::GetPrivateKey() const {
    return sk;
}

PublicKey ExtendedPrivateKey::GetPublicKey() const {
    return sk.GetPublicKey();
}

ChainCode ExtendedPrivateKey::GetChainCode() const {
    return chainCode;
}

ExtendedPublicKey ExtendedPrivateKey::GetExtendedPublicKey() const {
    uint8_t buffer[ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE];
    Util::IntToFourBytes(buffer, version);
    buffer[4] = depth;
    Util::IntToFourBytes(buffer + 5, parentFingerprint);
    Util::IntToFourBytes(buffer + 9, childNumber);

    chainCode.Serialize(buffer + 13);
    sk.GetPublicKey().Serialize(buffer + 13 + ChainCode::CHAIN_CODE_SIZE);

    return ExtendedPublicKey::FromBytes(buffer);
}

// Comparator implementation.
bool operator==(ExtendedPrivateKey const &a,  ExtendedPrivateKey const &b) {
    return (a.GetPrivateKey() == b.GetPrivateKey() &&
            a.GetChainCode() == b.GetChainCode());
}

bool operator!=(ExtendedPrivateKey const&a,  ExtendedPrivateKey const&b) {
    return !(a == b);
}

void ExtendedPrivateKey::Serialize(uint8_t *buffer) const {
    Util::IntToFourBytes(buffer, version);
    buffer[4] = depth;
    Util::IntToFourBytes(buffer + 5, parentFingerprint);
    Util::IntToFourBytes(buffer + 9, childNumber);
    chainCode.Serialize(buffer + 13);
    sk.Serialize(buffer + 13 + ChainCode::CHAIN_CODE_SIZE);
}

std::vector<uint8_t> ExtendedPrivateKey::Serialize() const {
    std::vector<uint8_t> data(EXTENDED_PRIVATE_KEY_SIZE);
    Serialize(data.data());
    return data;
}

// Destructors in PrivateKey and ChainCode handle cleaning of memory
ExtendedPrivateKey::~ExtendedPrivateKey() {}
} // end namespace bls
