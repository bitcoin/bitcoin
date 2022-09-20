// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/enriched_proof.h>

EnrichedProof EnrichedProof::Build(
  const Proof& proof,
  const size_t& inv_offset,
  const size_t& rounds,
  const size_t& log_m
) {
    CHashWriter transcript(0,0);

    // add Vs to transcript
    for (size_t i = 0; i < proof.Vs.Size(); ++i) {
        transcript << proof.Vs[i];
    }
    // A and S are not a part of ProofData and only added to transcript
    transcript << proof.A;
    transcript << proof.S;

    // y is created from transcript and transcript is updated with the y
    Scalar y = transcript.GetHash();
    transcript << y;

    // z is created from transcript and transcript is updated with the z
    Scalar z = transcript.GetHash();
    transcript << z;

    // T1 and T2 are not a part of ProofData and only added to transcript
    transcript << proof.T1;
    transcript << proof.T2;

    // x is created from transcript and transcript is updated with the x
    Scalar x = transcript.GetHash();
    transcript << x;

    // tau_x, mu and t are not a part of ProofData and only added to transcript
    transcript << proof.tau_x;
    transcript << proof.mu;
    transcript << proof.t;

    // x_ip is created from transcript
    Scalar x_ip = transcript.GetHash();

    // for each L and R, they are added to transcript and then w is created from transcript at that point
    // ws is the vectors of such w's
    Scalars ws;
    for (size_t i = 0; i < rounds; ++i) {
        transcript << proof.Ls[i];
        transcript << proof.Rs[i];
        Scalar w(transcript.GetHash());
        ws.Add(w);
    }

    return EnrichedProof(
        proof,
        x,
        y,
        z,
        x_ip,
        ws,
        log_m,
        inv_offset
    );
}
