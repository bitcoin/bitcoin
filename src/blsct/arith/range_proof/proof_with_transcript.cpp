// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/proof_with_transcript.h>

ProofWithTranscript ProofWithTranscript::Build(const Proof& proof) {
    // build transcript from proof in the same way it was built in Prove function
    CHashWriter transcript_gen(0,0);

    for (size_t i = 0; i < proof.Vs.Size(); ++i) {
        transcript_gen << proof.Vs[i];
    }
    transcript_gen << proof.A;
    transcript_gen << proof.S;

    Scalar y = transcript_gen.GetHash();
    transcript_gen << y;

    Scalar z = transcript_gen.GetHash();
    transcript_gen << z;

    transcript_gen << proof.T1;
    transcript_gen << proof.T2;

    Scalar x = transcript_gen.GetHash();
    transcript_gen << x;

    transcript_gen << proof.tau_x;
    transcript_gen << proof.mu;
    transcript_gen << proof.t_hat;

    Scalar x_ip = transcript_gen.GetHash();

    // for each proof, generate w from Ls and Rs and store the inverse
    Scalars xs;
    Scalars inv_xs;
    for (size_t i = 0; i < proof.num_rounds; ++i) {
        transcript_gen << proof.Ls[i];
        transcript_gen << proof.Rs[i];
        Scalar x(transcript_gen.GetHash());
        xs.Add(x);
        inv_xs.Add(x.Invert());
    }

    size_t num_input_values_power_2 = Config::GetFirstPowerOf2GreaterOrEqTo(proof.Vs.Size());
    size_t concat_input_values_in_bits = num_input_values_power_2 * Config::m_input_value_bits;

    return ProofWithTranscript(
        proof,
        x,
        y,
        z,
        x_ip,
        xs,
        inv_xs,
        num_input_values_power_2,
        concat_input_values_in_bits
    );
}
