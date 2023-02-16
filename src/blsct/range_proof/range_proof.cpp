// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/range_proof.h>

template <typename T>
std::vector<uint8_t> SerializeRangeProof(const RangeProof<T>& proof)
{
    CDataStream s(0, 0);

    s << proof.Vs;
    s << proof.Ls;
    s << proof.Rs;
    s << proof.A;
    s << proof.S;
    s << proof.T1;
    s << proof.T2;
    s << proof.tau_x;
    s << proof.mu;
    s << proof.a;
    s << proof.b;
    s << proof.t_hat;

    Span spanStream(s);
    std::vector<uint8_t> vRet(spanStream.size());
    memcpy(&vRet[0], spanStream.begin(), spanStream.size());

    return vRet;
}
template std::vector<uint8_t> SerializeRangeProof(const RangeProof<Mcl>& proof);

template <typename T>
RangeProof<T> UnserializeRangeProof(const std::vector<unsigned char>& vecIn)
{
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;

    RangeProof<T> retProof;
    CDataStream s(vecIn, 0, 0);

    s >> retProof.Vs;
    s >> retProof.Ls;
    s >> retProof.Rs;
    s >> retProof.A;
    s >> retProof.S;
    s >> retProof.T1;
    s >> retProof.T2;
    s >> retProof.tau_x;
    s >> retProof.mu;
    s >> retProof.a;
    s >> retProof.b;
    s >> retProof.t_hat;

    return retProof;
}
template RangeProof<Mcl> UnserializeRangeProof(const std::vector<unsigned char>& vecIn);
