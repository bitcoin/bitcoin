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

#include "bls.hpp"

namespace bls {

ExtendedPublicKey ExtendedPublicKey::FromBytes(const Bytes& bytes, const bool fLegacy) {
    uint32_t version = Util::FourBytesToInt(bytes.begin());
    uint32_t depth = bytes[4];
    uint32_t parentFingerprint = Util::FourBytesToInt(bytes.begin() + 5);
    uint32_t childNumber = Util::FourBytesToInt(bytes.begin() + 9);
    const uint8_t* ccPointer = bytes.begin() + 13;
    const uint8_t* pkPointer = ccPointer + ChainCode::SIZE;

    ExtendedPublicKey epk(version, depth, parentFingerprint, childNumber,
                          ChainCode::FromBytes(Bytes(ccPointer, ChainCode::SIZE)),
                          G1Element::FromBytes(Bytes(pkPointer, G1Element::SIZE), fLegacy));
    return epk;
}

ExtendedPublicKey ExtendedPublicKey::PublicChild(uint32_t i, const bool fLegacy) const {
    // Hardened children have i >= 2^31. Non-hardened have i < 2^31
    uint32_t cmp = (1 << 31);
    if (i >= cmp) {
        throw std::invalid_argument("Cannot derive hardened children from public key");
    }
    if (depth >= 255) {
        throw std::logic_error("Cannot go further than 255 levels");
    }
    uint8_t ILeft[PrivateKey::PRIVATE_KEY_SIZE];
    uint8_t IRight[ChainCode::SIZE];

    // Chain code is used as hmac key
    uint8_t hmacKey[ChainCode::SIZE];
    chainCode.Serialize(hmacKey);

    // Public key serialization, i serialization, and one 0 or 1 byte
    size_t inputLen = G1Element::SIZE + 4 + 1;

    // Hmac input includes sk or pk, int i, and byte with 0 or 1
    uint8_t hmacInput[G1Element::SIZE + 4 + 1];

    // Fill the input with the required data
    auto vecG1 = pk.Serialize(fLegacy);
    memcpy(hmacInput, vecG1.data(), vecG1.size());

    hmacInput[inputLen - 1] = 0;
    Util::IntToFourBytes(hmacInput + G1Element::SIZE, i);

    md_hmac(ILeft, hmacInput, inputLen,
                    hmacKey, ChainCode::SIZE);

    // Change 1 byte to generate a different sequence for chaincode
    hmacInput[inputLen - 1] = 1;

    md_hmac(IRight, hmacInput, inputLen,
                    hmacKey, ChainCode::SIZE);

    PrivateKey leftSk = PrivateKey::FromBytes(Bytes(ILeft, PrivateKey::PRIVATE_KEY_SIZE), true);
    G1Element newPk = pk + leftSk.GetG1Element();

    ExtendedPublicKey epk(version, depth + 1,
                          GetPublicKey().GetFingerprint(), i,
                          ChainCode::FromBytes(Bytes(IRight, ChainCode::SIZE)),
                          newPk);

    return epk;
}

uint32_t ExtendedPublicKey::GetVersion() const {
    return version;
}

uint8_t ExtendedPublicKey::GetDepth() const {
    return depth;
}

uint32_t ExtendedPublicKey::GetParentFingerprint() const {
    return parentFingerprint;
}

uint32_t ExtendedPublicKey::GetChildNumber() const {
    return childNumber;
}

ChainCode ExtendedPublicKey::GetChainCode() const {
    return chainCode;
}

G1Element ExtendedPublicKey::GetPublicKey() const {
    return pk;
}

// Comparator implementation.
bool operator==(ExtendedPublicKey const &a,  ExtendedPublicKey const &b) {
    return (a.GetPublicKey() == b.GetPublicKey() &&
            a.GetChainCode() == b.GetChainCode());
}

bool operator!=(ExtendedPublicKey const&a,  ExtendedPublicKey const&b) {
    return !(a == b);
}

std::ostream &operator<<(std::ostream &os, ExtendedPublicKey const &a) {
    return os << a.GetPublicKey() << a.GetChainCode();
}

void ExtendedPublicKey::Serialize(uint8_t *buffer, const bool fLegacy) const {
    Util::IntToFourBytes(buffer, version);
    buffer[4] = depth;
    Util::IntToFourBytes(buffer + 5, parentFingerprint);
    Util::IntToFourBytes(buffer + 9, childNumber);
    chainCode.Serialize(buffer + 13);
    auto vecG1 = pk.Serialize(fLegacy);
    memcpy(buffer + 13 + ChainCode::SIZE, vecG1.data(), vecG1.size());
}

std::vector<uint8_t> ExtendedPublicKey::Serialize(const bool fLegacy) const {
    std::vector<uint8_t> data(SIZE);
    Serialize(data.data(), fLegacy);
    return data;
}

} // end namespace bls
