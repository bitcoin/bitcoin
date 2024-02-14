// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/signature.h>
#include <bls/bls384_256.h>
#include <iterator>
#include <streams.h>
#include <tinyformat.h>

#include <algorithm>

namespace blsct {

Signature::Signature()
{
    // Replacement of mclBnG2_clear to avoid segfault in static context
    std::memset(&m_data.v, 0, sizeof(mclBnG2));
}

Signature Signature::Aggregate(const std::vector<blsct::Signature>& sigs)
{
    std::vector<blsSignature> bls_sigs;
    std::transform(sigs.begin(), sigs.end(), std::back_inserter(bls_sigs), [](auto sig) {
        return sig.m_data;
    });
    blsct::Signature aggr_sig;
    blsAggregateSignature(&aggr_sig.m_data, &bls_sigs[0], bls_sigs.size());
    return aggr_sig;
}

std::vector<uint8_t> Signature::GetVch() const
{
    size_t ser_size = mclBn_getFpByteSize() * 2;
    std::vector<uint8_t> buf(ser_size);
    size_t n = mclBnG2_serialize(&buf[0], ser_size, &m_data.v);
    if (n != ser_size) {
        blsct::Signature ret;
        mclBnG2_clear(&ret.m_data.v);
        return ret.GetVch();
    }
    return buf;
}

void Signature::SetVch(const std::vector<uint8_t>& buf)
{
    size_t ser_size = mclBn_getFpByteSize() * 2;
    if (buf.size() != ser_size) {
        return;
    }
    if (mclBnG2_deserialize(&m_data.v, &buf[0], ser_size) == 0) {
        mclBnG2_clear(&m_data.v);
    }
}

bool Signature::operator==(const Signature& b) const
{
    return mclBnG2_isEqual(&m_data.v, &b.m_data.v);
}

} // namespace blsct
