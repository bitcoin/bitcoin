// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BLS_ETH 1

#include <blsct/common.h> // causes mutual dependency issue if included in the public_key header
#include <blsct/public_key.h>
#include <tinyformat.h>
#include <util/strencodings.h>

namespace blsct {
using Point = MclG1Point;

uint256 PublicKey::GetHash() const
{
    HashWriter ss{};
    ss << GetVch();
    return ss.GetHash();
}

CKeyID PublicKey::GetID() const
{
    return CKeyID(Hash160(GetVch()));
}

Point PublicKey::GetG1Point() const
{
    return point;
}

std::string PublicKey::ToString() const
{
    return HexStr(GetVch());
};

bool PublicKey::operator==(const PublicKey& rhs) const
{
    return GetVch() == rhs.GetVch();
}

bool PublicKey::operator!=(const PublicKey& rhs) const
{
    return GetVch() != rhs.GetVch();
}

bool PublicKey::IsValid() const
{
    if (fValid == -1) {
        if (point.IsValid())
            const_cast<PublicKey*>(this)->fValid = 1;
    }
    return fValid == 1;
}

std::vector<unsigned char> PublicKey::GetVch() const
{
    return point.GetVch();
}

blsPublicKey PublicKey::ToBlsPublicKey() const
{
    blsPublicKey bls_pk{point.GetUnderlying()};
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
    return CoreVerify(Common::BLSCTBALANCE, sig);
}

bool PublicKey::Verify(const Message& msg, const Signature& sig) const
{
    auto aug_msg = AugmentMessage(msg);
    return CoreVerify(aug_msg, sig);
}

} // namespace blsct
