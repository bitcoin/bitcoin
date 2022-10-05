// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/range_proof.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include <tinyformat.h>

Scalar RangeProof::m_one;
Scalar RangeProof::m_two;
Scalars RangeProof::m_two_pows;

RangeProof::RangeProof()
{
    if (m_is_initialized) return;
    boost::lock_guard<boost::mutex> lock(RangeProof::m_init_mutex);

    MclInitializer::Init();
    G1Point::Init();

    //RangeProof::m_gens = Generators();
    RangeProof::m_one = Scalar(1);
    RangeProof::m_two = Scalar(2);
    RangeProof::m_two_pows = Scalars::FirstNPow(m_two, Config::m_input_value_bits);
    auto ones = Scalars::RepeatN(RangeProof::m_one, Config::m_input_value_bits);
    RangeProof::m_inner_prod_ones_and_two_pows = (ones * RangeProof::m_two_pows).Sum();

    m_is_initialized = true;
}

bool RangeProof::InnerProductArgument(
    const size_t input_value_vec_len,
    const Generators& gens,
    const Scalar& x_ip,
    const Scalars& l,
    const Scalars& r,
    const Scalar& y,
    Proof& proof,
    CHashWriter& transcript_gen
) {
    // build initial state
    Scalars scale_factors = Scalars::FirstNPow(y.Invert(), input_value_vec_len);
    G1Points g_prime = gens.Gi;
    G1Points h_prime = gens.Hi;
    Scalars a_prime = l;
    Scalars b_prime = r;

    size_t n_prime = input_value_vec_len;  // # of rounds is log2 n_prime
    size_t round = 0;
    Scalars xs;

    while (n_prime > 1) {
        // (20)
        n_prime /= 2;

        // (21)-(22)
        Scalar cL = (a_prime.To(n_prime) * b_prime.From(n_prime)).Sum();
        Scalar cR = (a_prime.From(n_prime) * b_prime.To(n_prime)).Sum();

        // (23)-(24)
        Scalar extra_scalar_cL = cL * x_ip;
        proof.Ls.Add(
            (g_prime.From(n_prime) * a_prime.To(n_prime)).Sum() +
            (h_prime.To(n_prime) * (round == 0 ? b_prime * scale_factors.To(n_prime) : b_prime.From(n_prime))).Sum() +
            (gens.H * extra_scalar_cL)
        );
        Scalar extra_scalar_cR = cR * x_ip;
        proof.Rs.Add(
            (g_prime.To(n_prime) * a_prime.From(n_prime)).Sum() +
            (h_prime.From(n_prime) * (round == 0 ? b_prime * scale_factors.From(n_prime) : b_prime.To(n_prime))).Sum() +
            (gens.H * extra_scalar_cR)
        );

        // (25)-(27)
        transcript_gen << proof.Ls[round];
        transcript_gen << proof.Rs[round];

        Scalar x = transcript_gen.GetHash();
        if (x == 0)
            return false;
        Scalar x_inv = x.Invert();
        xs.Add(x);

        // (29)-(31)
        if (n_prime > 1) {
            g_prime = (g_prime.To(n_prime) * x_inv) + (g_prime.From(n_prime) * x);

            // apply scale_factors to x and x_inv
            Scalars sf_ws = scale_factors * x;
            Scalars sf_w_invs = scale_factors * x_inv;
            h_prime = (h_prime.To(n_prime) * sf_ws) + (h_prime.From(n_prime) * sf_w_invs);
        }

        // (33)-(34)
        a_prime = (a_prime.To(n_prime) * x) + (a_prime.From(n_prime) * x_inv);
        b_prime = (b_prime.To(n_prime) * x_inv) + (b_prime.From(n_prime) * x);

        ++round;
    }

    proof.a = a_prime[0];
    proof.b = b_prime[0];

    return true;
}

size_t RangeProof::GetInnerProdArgRounds(const size_t& num_input_values) const
{
    const size_t num_input_values_power_of_2 = GetFirstPowerOf2GreaterOrEqTo(num_input_values);
    const size_t rounds = std::log2(num_input_values_power_of_2) + std::log2(Config::m_input_value_bits);
    return rounds;
}

Proof RangeProof::Prove(
    Scalars vs,
    G1Point nonce,
    const std::vector<uint8_t>& message,
    const TokenId& token_id
) {
    if (message.size() > Config::m_max_message_size) {
        throw std::runtime_error(strprintf("%s: message size is too large", __func__));
    }
    if (vs.Empty()) {
        throw std::runtime_error(strprintf("%s: value vector is empty", __func__));
    }
    if (vs.Size() > Config::m_max_input_values) {
        throw std::runtime_error(strprintf("%s: number of input values exceeds the maximum", __func__));
    }

    const size_t num_input_values_power_2 = GetFirstPowerOf2GreaterOrEqTo(vs.Size());
    const size_t concat_input_values_in_bits = num_input_values_power_2 * Config::m_input_value_bits;

    ////////////// Proving steps
    Proof proof;

    // Initialize gammas
    Scalars gammas;
    for (size_t i = 0; i < vs.Size(); ++i) {
        auto hash = nonce.GetHashWithSalt(100 + i);
        gammas.Add(hash);
    }

    // Get Generators for the token_id
    Generators gens = m_gf.GetInstance(token_id);

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter transcript_gen(0, 0);

    // Calculate value commitments and add them to transcript
    proof.Vs = G1Points(gens.H * gammas.m_vec) + G1Points(gens.G.get() * vs.m_vec);
    for (size_t i = 0; i < vs.Size(); ++i) {
        transcript_gen << proof.Vs[i];
    }

    // (41)-(42)
    // Values to be obfuscated are encoded in binary and flattened to a single vector aL
    Scalars aL;   // ** size of aL can be shorter than concat_input_values_in_bits
    for(Scalar& v: vs.m_vec) {  // for each input value
        auto bits = v.GetBits();  // gets the value in binary
        for(bool bit: bits) {
            aL.Add(bit);
        }
        // fill the remaining bits if needed
        for(size_t i = 0; i < Config::m_input_value_bits - bits.size(); ++i) {
            aL.Add(false);
        }
    }
    // TODO fill bits if aL.size < concat_input_values_in_bits
    assert(false);

    auto one_value_concat_bits = Scalars::FirstNPow(m_one, concat_input_values_in_bits);

    // aR is aL - 1
    Scalars aR = aL - one_value_concat_bits;

    size_t num_tries = 0;
retry:  // hasher is not cleared so that different hash will be obtained upon retry

    if (++num_tries > Config::m_max_prove_tries) {
        throw std::runtime_error(strprintf("%s: exceeded maxinum number of tries", __func__));
    }

    // (43)-(44)
    // Commitment to aL and aR (obfuscated with alpha)

    // trim message to first 23 bytes if needed
    Scalar msg1_scalar(
        message.size() > 23 ?
            std::vector<uint8_t>(message.begin(), message.begin() + 23) :
            message
    );
    // first part of message + 64-byte vs[0]
    Scalar msg1_v0 = (msg1_scalar << Config::m_input_value_bits) | vs[0];

    Scalar alpha = nonce.GetHashWithSalt(1);
    alpha = alpha + msg1_v0;

    // Using generator H for alpha following the paper
    proof.A = (gens.H * alpha) + (gens.Gi.get() * aL).Sum() + (gens.Hi.get() * aR).Sum();

    // (45)-(47)
    // Commitment to blinding vectors sL and sR (obfuscated with rho)
    auto sL = Scalars::RandVec(Config::m_input_value_bits, true);
    auto sR = Scalars::RandVec(Config::m_input_value_bits, true);

    auto rho = nonce.GetHashWithSalt(2);
    // Using generator H for alpha following the paper
    proof.S = (gens.H * rho) + (gens.Gi.get() * sL).Sum() + (gens.Hi.get() * sR).Sum();

    // (48)-(50)
    transcript_gen << proof.A;
    transcript_gen << proof.S;

    Scalar y = transcript_gen.GetHash();
    if (y == 0) goto retry;
    transcript_gen << y;

    Scalar z = transcript_gen.GetHash();
    if (z == 0) goto retry;
    transcript_gen << z;

    // Polynomial construction by coefficients
    // AFTER (50)

    // l(x) = (aL - z 1^n) + sL X
    // aL is a concatination of all input value bits, so mn bits (= input_value_total_bits) are needed
    Scalars z_value_total_bits = Scalars::FirstNPow(z, concat_input_values_in_bits);
    Scalars l0 = aL - z_value_total_bits;

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    Scalars l1 = sL;

    // Calculation of r(0) and r(1) on page 19
    Scalars z_n_times_two_n;
    Scalars z_pows = Scalars::FirstNPow(z, concat_input_values_in_bits, 2);  // z_pows excludes 1 and z

    // The last term of r(X) on page 19
    for (size_t i = 0; i < concat_input_values_in_bits; ++i) {
        auto base_z = z_pows[i];  // change base Scalar for each input value

        for (size_t bit_idx = 0; bit_idx < Config::m_input_value_bits; ++bit_idx) {
            z_n_times_two_n.Add(base_z * m_two_pows[bit_idx]);
        }
    }

    Scalars y_value_total_bits = Scalars::FirstNPow(y, concat_input_values_in_bits);
    Scalars r0 = (y_value_total_bits * (aR + z_value_total_bits)) + z_n_times_two_n;
    Scalars r1 = y_value_total_bits * sR;

    // Polynomial construction before (51)
    Scalar t1 = (l0 * r1).Sum() + (l1 * r0).Sum();
    Scalar t2 = (l1 * r1).Sum();

    // (52)-(53)
    Scalar tau1 = nonce.GetHashWithSalt(3);
    Scalar tau2 = nonce.GetHashWithSalt(4);

    // if message size is 24-byte or bigger, treat that part as msg2
    Scalar msg2_scalar = Scalar({
        message.size() > 23 ?
            std::vector<uint8_t>(message.begin() + 23, message.end()) :
            std::vector<uint8_t>()
    });
    tau1 = tau1 + msg2_scalar;

    proof.T1 = (gens.G.get() * t1) + (gens.H * tau1);
    proof.T2 = (gens.G.get() * t2) + (gens.H * tau2);

    // (54)-(56)
    transcript_gen << proof.T1;
    transcript_gen << proof.T2;

    Scalar x = transcript_gen.GetHash();
    if (x == 0) goto retry;

    // x will be added to transcript later

    // (58)-(59)
    Scalars l = l0 + (l1 * x);  // l0 = aL - z_mn; l1 = sL
    Scalars r = r0 + (r1 * x);  // r0 = RHS of (58) - r1; r1 = y_mn o (sR * x)

    // LHS of (60)
    proof.t_hat = (l * r).Sum();

    // RHS of (60)
    Scalar t0 = (l0 * r0).Sum();
    Scalar t_of_x = t0 + t1 * x + t2 * x.Square();

    // (60)
    if (proof.t_hat != t_of_x)
        throw std::runtime_error(strprintf("%s: equality didn't hold in (60)", __func__));

    proof.tau_x = (tau2 * x.Square()) + (tau1 * x) + (z_pows * gammas).Sum();  // (61)
    proof.mu = alpha + (rho * x);  // (62)

    // (63)
    transcript_gen << x;
    transcript_gen << proof.tau_x;
    transcript_gen << proof.mu;
    transcript_gen << proof.t_hat;

    Scalar x_for_inner_product_argument = transcript_gen.GetHash();
    if (x_for_inner_product_argument == 0) goto retry;

    if (!InnerProductArgument(concat_input_values_in_bits, gens, x_for_inner_product_argument, l, r, y, proof, transcript_gen)) {
        goto retry;
    }
    return proof;
}

bool RangeProof::ValidateProofsBySizes(
    const std::vector<Proof>& proofs,
    const size_t& num_rounds
) const {
    for (const Proof& proof: proofs) {
        // proof must contain input values
        if (proof.Vs.Size() == 0) return false;

        // invalid if # of input values are lager than maximum
        if (proof.Vs.Size() > Config::m_max_input_values) return false;

        // L,R keep track of aggregation history and the size should equal to # of rounds
        if (proof.Ls.Size() != num_rounds)
            return false;

        // if Ls and Rs should have the same size
        if (proof.Ls.Size() != proof.Rs.Size()) return false;
    }
    return true;
}

G1Point RangeProof::VerifyLoop2(
    const std::vector<ProofWithTranscript>& proof_transcripts,
    const Generators& gens,
    const size_t& max_mn
) const {
    G1Points points;
    Scalar h_pos_exp = 0;
    Scalar g_neg_exp = 0;
    Scalar h_neg_exp = 0;
    Scalar g_pos_exp = 0;
    Scalars gi_exps(max_mn, 0);
    Scalars hi_exps(max_mn, 0);

    for (const ProofWithTranscript& p: proof_transcripts) {
        Scalar weight_y = Scalar::Rand();
        Scalar weight_z = Scalar::Rand();

        Scalars z_pows = Scalars::FirstNPow(p.z, Config::m_input_value_bits, 2); // z^2, z^3, ... // VectorPowers(pd.z, M+3);
        Scalar y_pows_sum = Scalars::FirstNPow(p.y, p.concat_input_values_in_bits - 1).Sum(); // VectorPowerSum(p.y, MN);

        //////// (65)
        // g^t_hat * h^tau_x = V^(z^2) * g^delta_yz * T1^x * T2^(x^2)
        // g^t_hat * h^(-tau_x) * V^(z^2) * g^delta_yz * T1^x * T2^(x^2)

        h_neg_exp = h_neg_exp + (p.proof.tau_x * weight_y);  // h^tau_x

        // delta(y,z) in (39)
        // = (z - z^2)*<1^n, y^n> - z^3<1^n,2^n>
        // = z*<1^n, y^n> (1) - z^2*<1^n, y^n> (2) - z^3<1^n,2^n> (3)
        Scalar delta_yz = p.z * y_pows_sum;  // (1)
        delta_yz = (z_pows[0] * y_pows_sum).Negate(); // (2)
        for (size_t i = 1; i <= Config::m_input_value_bits; ++i) {
            // multiply z^3, z^4, ..., z^(mn+3)
            delta_yz = delta_yz - (z_pows[i] * RangeProof::m_inner_prod_ones_and_two_pows);  // (3)
        }

        // g^t_hat ... = ... g^delta_yz
        // g^(t_hat - delta_yz) = ...
        // ... = - g^(t_hat - delta_yz)
        g_neg_exp = g_neg_exp + ((p.proof.t_hat - delta_yz) * weight_y);

        // V^(z^2)
        for (size_t i = 0; i < p.proof.Vs.Size(); ++i) {
            points.Add(p.proof.Vs[i] * (z_pows[i] * weight_y));  // multiply z^2, z^3, ...
        }

        points.Add(p.proof.T1 * (p.x * weight_y));  // T1^x
        points.Add(p.proof.T2 * (p.x.Square() * weight_y));  // T2^(x^2)

        //////// (66)
        // P = A * S^x * g^(-z) * (h')^(z * y^n + z^2 * 2^n)
        // ** exponent of g and (h') are created in loop below
        points.Add(p.proof.A * weight_z); // A
        points.Add(p.proof.S * (p.x * weight_z));  // S^x

        //////// (67), (68)

        // for all bits of input values
        std::vector<Scalar> w_cache(1 << p.num_rounds, 1);  // initialize all elems to 1
        w_cache[0] = p.inv_ws[0];
        w_cache[1] = p.ws[0];
        for (size_t j = 1; j < p.num_rounds; ++j) {
            const size_t sl = 1 << (j + 1);  // 4, 8, 16 ...

            for (size_t s = sl; s > 0; s -= 2) {
                w_cache[s] = w_cache[s / 2] * p.ws[j];
                w_cache[s - 1] = w_cache[s / 2] * p.inv_ws[j];
            }
        }

        // for each bit of concat input values, do:
        Scalar y_inv_pow(1);
        Scalar y_pow(1);
        for (size_t i = 0; i < p.concat_input_values_in_bits; ++i) {
            Scalar gi_exp = p.proof.a * w_cache[i];  // from the beg from end
            Scalar hi_exp = p.proof.b * y_inv_pow * w_cache[p.concat_input_values_in_bits - 1 - i];  // from the end to beg. y_inv_pow to turn generator to (h')

            gi_exp = gi_exp + p.z;  // g^(-z) (66)

            // ** z^2 * 2^n in (h')^(z * y^n + z^2 * 2^n) (66)
            Scalar tmp =
                z_pows[2 + i / Config::m_input_value_bits] *  // skipping the first 2 powers, different z_pow is assigned to each number
                m_two_pows[i % Config::m_input_value_bits];   // power of 2 corresponding to i-th bit of the number being processed
            // ** z * y^n in (h')^(z * y^n + z^2 * 2^n) (66)
            hi_exp = hi_exp - (tmp + p.z * y_pow) * y_inv_pow;

            gi_exps[i] = gi_exps[i] - (gi_exp * weight_z);
            hi_exps[i] = hi_exps[i] - (hi_exp * weight_z);

            // update y_pow and y_inv_pow to the next power
            y_inv_pow = y_inv_pow * p.inv_y;
            y_pow = y_pow * p.y;
        }

        h_neg_exp = h_neg_exp + (p.proof.mu * weight_z);  // ** h^mu (67) RHS

        // for each round, do:
        // add L^(x^2) * R^(x-1)^2 of all rounds
        for (size_t i = 0; i < p.num_rounds; ++i) {
            points.Add(p.proof.Ls[i] * (p.ws[i].Square() * weight_z));
            points.Add(p.proof.Rs[i] * (p.inv_ws[i].Square() * weight_z));
        }

        // t_hat = <L, R> (68)?
        // a, b are result of inner product argument
        // x_ip is a random number generated from transcript
        g_pos_exp = g_pos_exp + (((p.proof.t_hat - (p.proof.a * p.proof.b)) * p.x_ip) * weight_z);
    }

    points.Add(gens.G.get() * (g_pos_exp - g_neg_exp));
    points.Add(gens.H * (h_pos_exp - h_neg_exp));

    for (size_t i = 0; i < max_mn; ++i) {
        points.Add(gens.Gi.get()[i] * gi_exps[i]);
        points.Add(gens.Hi.get()[i] * hi_exps[i]);
    }

    // should be aggregated to zero if proofs are all valid
    return points.Sum();
}

bool RangeProof::Verify(
    const std::vector<Proof>& proofs,
    const TokenId& token_id
) const {
    const size_t num_rounds = GetInnerProdArgRounds(Config::m_input_value_bits);
    if (!ValidateProofsBySizes(proofs, num_rounds)) return false;

    std::vector<ProofWithTranscript> proof_derivs;
    size_t max_num_rounds = 0;
    size_t Vs_size_sum = 0;

    for (const Proof& proof: proofs) {
        // update max # of rounds and sum of all V bits
        max_num_rounds = std::max(max_num_rounds, proof.Ls.Size());
        Vs_size_sum += proof.Vs.Size();

        // derive required Scalars from proof
        auto proof_deriv = ProofWithTranscript::Build(proof, num_rounds);
        proof_derivs.push_back(proof_deriv);
    }


    const size_t max_mn = 1u << max_num_rounds;
    const Generators gens = m_gf.GetInstance(token_id);

    G1Point point_sum = VerifyLoop2(
        proof_derivs,
        gens,
        max_mn
    );
    return point_sum.IsUnity();
}

std::vector<RecoveredAmount> RangeProof::RecoverAmounts(
    const std::vector<AmountRecoveryReq>& reqs,
    const TokenId& token_id
) const {
    const Generators gens = m_gf.GetInstance(token_id);
    std::vector<RecoveredAmount> ret;  // will contain result of successful requests only

    for (const AmountRecoveryReq& req: reqs) {
        // skip this tx_in if sizes of Ls and Rs differ or Vs is empty
        auto Ls_Rs_valid = req.Ls.Size() > 0 && req.Ls.Size() == req.Rs.Size();
        if (req.Vs.Size() == 0 || !Ls_Rs_valid) {
            continue;
        }

        // derive random Scalar values from nonce
        const Scalar alpha = req.nonce.GetHashWithSalt(1);  // (A)
        const Scalar rho = req.nonce.GetHashWithSalt(2);
        const Scalar tau1 = req.nonce.GetHashWithSalt(3);  // (C)
        const Scalar tau2 = req.nonce.GetHashWithSalt(4);
        const Scalar input_value0_gamma = req.nonce.GetHashWithSalt(100);  // gamma for vs[0]

        // mu = alpha + rho * x ... (62)
        // alpha = mu - rho * x ... (B)
        //
        // alpha (B) equals to alpha (A) + (message || 64-byte v[0])
        // so by subtracting alpha (A) from alpha (B), you can extract (message || 64-byte v[0])
        // then applying 64-byte mask fuether extracts 64-byte v[0]
        const Scalar message_v0 = (req.mu - rho * req.x) - alpha;
        const Scalar input_value0 = message_v0 & Scalar(0xFFFFFFFFFFFFFFFF);

        // skip this tx_in if recovered input value 0 commitment doesn't match with Vs[0]
        G1Point input_value0_commitment = (gens.G.get() * input_value0_gamma) + (gens.H * input_value0);
        if (input_value0_commitment != req.Vs[0]) {
            continue;
        }

        // generate message and set to data
        // extract the message part from (up-to-23-byte message || 64-byte v[0])
        // by 64 bytes tot the right
        std::vector<uint8_t> msg1 = (message_v0 >> 64).GetVch(true);

        auto tau_x = req.tau_x;
        auto x = req.x;
        auto z = req.z;

        // tau_x = tau2 * x^2 + tau1 * x + z^2 * gamma ... (61)
        //
        // solving this equation for tau1, you get:
        //
        // tau_x - tau2 * x^2 - z^2 * gamma = tau1 * x
        // tau1 = (tau_x - tau2 * x^2 - z^2 * gamma) * x^-1 ... (D)
        //
        // since tau1 in (61) is tau1 (C) + msg2, by subtracting tau1 (C) from RHS of (D)
        // you can extract msg2
        Scalar msg2_scalar = ((tau_x - (tau2 * x.Square()) - (z.Square() * input_value0_gamma)) * x.Invert()) - tau1;
        std::vector<uint8_t> msg2 = msg2_scalar.GetVch(true);

        RecoveredAmount recovered_amount(
            req.index,
            input_value0.GetUint64(),
            input_value0_gamma,
            std::string(msg1.begin(), msg1.end()) + std::string(msg2.begin(), msg2.end())
        );
        ret.push_back(recovered_amount);
    }
    return ret;
}
