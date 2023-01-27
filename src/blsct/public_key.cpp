// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BLS_ETH 1

#include <blsct/public_key.h>
#include <blsct/bls12_381_common.h>  // causes mutual dependency issue if included in the public_key header
#include <tinyformat.h>
#include <util/strencodings.h>

namespace blsct {

uint256 PublicKey::GetHash() const
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << data;
    return ss.GetHash();
}

CKeyID PublicKey::GetID() const
{
    return CKeyID(Hash160(data));
}

bool PublicKey::GetG1Point(Point& ret) const
{
    try {
        ret = Point(data);
    } catch (...) {
        return false;
    }
    return true;
}

std::string PublicKey::ToString() const
{
    return HexStr(GetVch());
};

bool PublicKey::operator==(const PublicKey& rhs) const
{
    return GetVch() == rhs.GetVch();
}

bool PublicKey::IsValid() const
{
    if (data.size() == 0) return false;

    Point g1;

    if (!GetG1Point(g1)) return false;

    return g1.IsValid();
}

std::vector<unsigned char> PublicKey::GetVch() const
{
    return data;
}

blsPublicKey PublicKey::ToBlsPublicKey() const
{
    MclG1Point p;
    if (!GetG1Point(p)) {
      throw std::runtime_error("Failed to convert PublicKey to MclG1Point");
    }
    blsPublicKey bls_pk { p.Underlying() };
    return bls_pk;
}

std::vector<uint8_t> PublicKey::AugmentMessage(const Message& msg) const
{
    auto pk_data = GetVch();
    std::vector<uint8_t> aug_msg(pk_data);
    aug_msg.insert(aug_msg.end(), msg.begin(), msg.end());
    return aug_msg;
}

bool PublicKey::CoreVerify(const Message& msg, const Signature& sig) const
{
    auto bls_pk = ToBlsPublicKey();
    auto res = blsVerify(&sig.m_data, &bls_pk, &msg[0], msg.size());
    return res == 1;
}

bool PublicKey::VerifyBalance(const Signature& sig) const
{
    return CoreVerify(BLS12_381_Common::BLSCTBALANCE, sig);
}

bool PublicKey::Verify(const Message& msg, const Signature& sig) const
{
    auto aug_msg = AugmentMessage(msg);
    return CoreVerify(aug_msg, sig);
}

}  // namespace blsct
