// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/proof_with_derived_values.h>

ProofWithDerivedValues ProofWithDerivedValues::Build(
  const Proof& proof,
  const size_t& num_rounds
) {
    // create x, y, z and x_ip through transcript from proof
    // in the same way they were created in Prove function
    CHashWriter transcript(0,0);

    for (size_t i = 0; i < proof.Vs.Size(); ++i) {
        transcript << proof.Vs[i];
    }
    transcript << proof.A;
    transcript << proof.S;

    Scalar y = transcript.GetHash();
    transcript << y;

    Scalar z = transcript.GetHash();
    transcript << z;

    transcript << proof.T1;
    transcript << proof.T2;

    Scalar x = transcript.GetHash();
    transcript << x;

    transcript << proof.tau_x;
    transcript << proof.mu;
    transcript << proof.t;

    Scalar x_ip = transcript.GetHash();

    // for each proof, generate w from Ls and Rs and store the inverse
    Scalars ws;
    Scalars inv_ws;
    for (size_t i = 0; i < num_rounds; ++i) {
        transcript << proof.Ls[i];
        transcript << proof.Rs[i];
        Scalar w(transcript.GetHash());
        ws.Add(w);
        inv_ws.Add(w.Invert());
    }

    size_t num_input_values_power_2 = Config::GetFirstPowerOf2GreaterOrEqTo(proof.Vs.Size());
    size_t concat_input_values_in_bits = num_input_values_power_2 * Config::m_input_value_bits;

    return ProofWithDerivedValues(
        // Scalars derived from proof through transcript
        proof,
        x,
        y,
        z,
        x_ip,
        ws,
        inv_ws,
        num_input_values_power_2,
        concat_input_values_in_bits,
        num_rounds
    );
}
