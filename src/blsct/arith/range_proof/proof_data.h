// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_DATA_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_DATA_H

#include <blsct/arith/elements.h>
#include <blsct/arith/scalar.h>
#include <blsct/arith/range_proof/proof.h>

class ProofData
{
public:
    ProofData(
      const Proof& proof,
      const size_t& inv_offset,
      const size_t& rounds   // TODO move to Config?
    );

    Scalar& x();
    Scalar& y();
    Scalar& z();
    Scalar& x_ip();
    Scalars& ws();
    G1Points& Vs();
    size_t& log_m();
    size_t& inv_offset();

private:
    Scalar m_x;
    Scalar m_y;
    Scalar m_z;
    Scalar m_x_ip;
    Scalars m_ws;
    G1Points m_Vs;
    size_t m_log_m;
    size_t m_inv_offset;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_PROOF_DATA_H