// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// signining/verification part of implementation is based on:
// https://www.ietf.org/archive/id/draft-irtf-cfrg-bls-signature-05.html

#ifndef NAVCOIN_BLSCT_SIGNATURE_H
#define NAVCOIN_BLSCT_SIGNATURE_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <node/protocol_version.h>
#include <serialize.h>
#include <vector>

namespace blsct {

class Signature
{
public:
    Signature();

    static Signature Aggregate(const std::vector<blsct::Signature>& sigs);

    bool operator==(const Signature& b) const;
    std::vector<uint8_t> GetVch() const;
    void SetVch(const std::vector<uint8_t>& b);

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        auto vec = GetVch();
        s.write(MakeByteSpan(vec));
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        std::vector<unsigned char> vec(SERIALIZATION_SIZE);
        s.read(MakeWritableByteSpan(vec));
        SetVch(vec);
    }

    blsSignature m_data;

    static constexpr int SERIALIZATION_SIZE = 96;
};

} // namespace blsct

#endif // NAVCOIN_BLSCT_SIGNATURE_H
