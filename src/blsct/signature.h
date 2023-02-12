// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// signining/verification part of implementation is based on:
// https://www.ietf.org/archive/id/draft-irtf-cfrg-bls-signature-05.html

#ifndef NAVCOIN_BLSCT_SIGNATURE_H
#define NAVCOIN_BLSCT_SIGNATURE_H

#define BLS_ETH 1
#include <bls/bls384_256.h>
#include <serialize.h>
#include <vector>
#include <version.h>

namespace blsct {

class Signature
{
public:
    static Signature Aggregate(const std::vector<blsct::Signature>& sigs);

    std::vector<uint8_t> GetVch() const;
    void SetVch(const std::vector<uint8_t>& b);
    size_t GetSerializeSize(int nVersion = PROTOCOL_VERSION) const;

    template <typename Stream>
    void Serialize(Stream& s) const;

    template <typename Stream>
    void Unserialize(Stream& s);

    blsSignature m_data;
};

}  // namespace blsct

#endif  // NAVCOIN_BLSCT_SIGNATURE_H
