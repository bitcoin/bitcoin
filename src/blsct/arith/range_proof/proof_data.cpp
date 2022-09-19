// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/proof_data.h>

ProofData::ProofData(
  const Proof& proof,
  const size_t& inv_offset,
  const size_t& rounds
): m_Vs(proof.Vs), m_inv_offset(inv_offset) {

    CHashWriter transcript(0,0);

    // Vs and inv_offset have been stored to corresponding member variables

    // add Vs to transcript
    for (size_t i = 0; i < m_Vs.Size(); ++i) {
        transcript << m_Vs[i];
    }
    // A and S are not a part of ProofData and only added to transcript
    transcript << proof.A;
    transcript << proof.S;

    // y is created from transcript and transcript is updated with the y
    m_y = transcript.GetHash();
    transcript << m_y;

    // z is created from transcript and transcript is updated with the z
    m_z = transcript.GetHash();
    transcript << m_z;

    // T1 and T2 are not a part of ProofData and only added to transcript
    transcript << proof.T1;
    transcript << proof.T2;

    // x is created from transcript and transcript is updated with the x
    m_x = transcript.GetHash();
    transcript << m_x;

    // tau_x, mu and t are not a part of ProofData and only added to transcript
    transcript << proof.tau_x;
    transcript << proof.mu;
    transcript << proof.t;

    // x_ip is created from transcript
    m_x_ip = transcript.GetHash();

    // for each L and R, they are added to transcript and then w is created from transcript at that point
    // ws is the vectors of such w's
    for (size_t i = 0; i < rounds; ++i) {
        transcript << proof.Ls[i];
        transcript << proof.Rs[i];
        Scalar w(transcript.GetHash());
        m_ws.Add(w);
    }
}

Scalar& ProofData::x()
{
    return m_x;
}

Scalar& ProofData::y()
{
    return m_y;
}

Scalar& ProofData::z()
{
    return m_z;
}

Scalar& ProofData::x_ip()
{
    return m_x_ip;
}

Scalars& ProofData::ws()
{
    return m_ws;
}

G1Points& ProofData::Vs()
{
    return m_Vs;
}

size_t& ProofData::log_m()
{
    return m_log_m;
}

size_t& ProofData::inv_offset()
{
    return m_inv_offset;
}