// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/range_proof.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include "tinyformat.h"

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
    CHashWriter& transcript
) {
    // build initial state
    Scalars scale_factors = Scalars::FirstNPow(y.Invert(), input_value_vec_len);
    G1Points g_prime = gens.Gi;
    G1Points h_prime = gens.Hi;
    Scalars a_prime = l;
    Scalars b_prime = r;

    size_t n_prime = input_value_vec_len;
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
        transcript << proof.Ls[round];
        transcript << proof.Rs[round];

        Scalar x = transcript.GetHash();
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

size_t RangeProof::GetInputValueVecLen(const size_t& input_value_vec_len)
{
    size_t i = 1;
    while (i < input_value_vec_len) {
        i *= 2;
        if (i > Config::m_max_input_value_vec_len) {
            throw std::runtime_error(strprintf(
                "%s: input value vector length %d is too large", __func__, input_value_vec_len));
        }
    }
    return i;
}

Proof RangeProof::Prove(
    Scalars vs,
    G1Point nonce,
    const std::vector<uint8_t>& message,
    const TokenId& token_id,
    const size_t max_tries,
    const std::optional<Scalars> gammas_override)
{
    if (message.size() > Config::m_max_message_size) {
        throw std::runtime_error(strprintf("%s: message size is too large", __func__));
    }
    if (vs.Empty()) {
        throw std::runtime_error(strprintf("%s: value vector is empty", __func__));
    }
    if (gammas_override.has_value() && gammas_override.value().Size() != vs.Size()) {
        throw std::runtime_error(strprintf("%s: expected %d gammas in gammas_override, but got %d",
        __func__, vs.Size(), gammas_override.value().Size()));
    }

    const size_t input_value_vec_len = GetInputValueVecLen(vs.Size());
    const size_t input_value_total_bits = input_value_vec_len * Config::m_input_value_bits;

    ////////////// Proving steps
    Proof proof;

    // Initialize gammas
    Scalars gammas;
    if (gammas_override.has_value()) {
        if (gammas_override.value().Size() != vs.Size()) {
            throw std::runtime_error(strprintf(
                "%s: gammas_override size %d differs to the size of input values %d",
                __func__, gammas_override.value().Size(), vs.Size()));

        }
        gammas = gammas_override.value();
    } else {
        for (size_t i = 0; i < vs.Size(); ++i) {
            auto hash = nonce.GetHashWithSalt(100 + i);
            gammas.Add(hash);
        }
    }

    // Get Generators for the token_id
    Generators gens = m_gf.GetInstance(token_id);

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter transcript(0, 0);

    // Calculate value commitments and add them to transcript
    proof.Vs = G1Points(gens.H * gammas.m_vec) + G1Points(gens.G.get() * vs.m_vec);
    for (size_t i = 0; i < vs.Size(); ++i) {
        transcript << proof.Vs[i];
    }

    // (41)-(42)
    // Values to be obfuscated are encoded in binary and flattened to a single vector aL
    Scalars aL;
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
    auto one_value_total_bits = Scalars::FirstNPow(m_one, input_value_total_bits);

    // aR is aL - 1
    Scalars aR = aL - one_value_total_bits;

    size_t num_tries = 0;

try_again:  // hasher is not cleared so that different hash will be obtained upon retry

    if (++num_tries > max_tries) {
        throw std::runtime_error(strprintf("%s: exceeded maxinum number of tries", __func__));
    }

    // (43)-(44)
    // Commitment to aL and aR (obfuscated with alpha)
    std::vector<uint8_t> first_message {
        message.size() > 23 ?
            std::vector<uint8_t>(message.begin(), message.begin() + 23) :
            message
    };
    Scalar message_scalar(first_message);

    Scalar alpha = nonce.GetHashWithSalt(1);
    alpha = alpha + (vs[0] | (message_scalar << Config::m_input_value_bits));

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
    transcript << proof.A;
    transcript << proof.S;

    Scalar y = transcript.GetHash();
    if (y == 0)
        goto try_again;
    transcript << y;

    Scalar z = transcript.GetHash();
    if (z == 0)
        goto try_again;
    transcript << z;

    // Polynomial construction by coefficients
    // AFTER (50)

    // l(x) = (aL - z 1^n) + sL X
    // aL is a concatination of all input value bits, so mn bits (= input_value_total_bits) are needed
    Scalars z_value_total_bits = Scalars::FirstNPow(z, input_value_total_bits);
    Scalars l0 = aL - z_value_total_bits;

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    Scalars l1 = sL;

    // Calculation of r(0) and r(1) on page 19
    Scalars z_n_times_two_n;
    Scalars z_pows = Scalars::FirstNPow(z, input_value_total_bits, 2);  // z_pows excludes 1 and z

    // The last term of r(X) on page 19
    for (size_t i = 0; i < input_value_vec_len; ++i) {
        auto base_z = z_pows[i];  // change base Scalar for each input value

        for (size_t bit_idx = 0; bit_idx < Config::m_input_value_bits; ++bit_idx) {
            z_n_times_two_n.Add(base_z * m_two_pows[bit_idx]);
        }
    }

    Scalars y_value_total_bits = Scalars::FirstNPow(y, input_value_total_bits);
    Scalars r0 = (y_value_total_bits * (aR + z_value_total_bits)) + z_n_times_two_n;
    Scalars r1 = y_value_total_bits * sR;

    // Polynomial construction before (51)
    Scalar t1 = (l0 * r1).Sum() + (l1 * r0).Sum();
    Scalar t2 = (l1 * r1).Sum();

    // (52)-(53)
    Scalar tau1 = nonce.GetHashWithSalt(3);
    Scalar tau2 = nonce.GetHashWithSalt(4);
    Scalar second_message = Scalar({
        message.size() > 23 ?
            std::vector<uint8_t>(message.begin() + 23, message.end()) :
            std::vector<uint8_t>()
    });
    tau1 = tau1 + second_message;

    proof.T1 = (gens.G.get() * t1) + (gens.H * tau1);
    proof.T2 = (gens.G.get() * t2) + (gens.H * tau2);

    // (54)-(56)
    transcript << proof.T1;
    transcript << proof.T2;

    Scalar x = transcript.GetHash();
    if (x == 0)
        goto try_again;
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
    transcript << x;
    transcript << proof.tau_x;
    transcript << proof.mu;
    transcript << proof.t;

    Scalar x_ip = transcript.GetHash();
    if (x_ip == 0)
        goto try_again;

    if (!InnerProductArgument(input_value_vec_len, gens, x_ip, l, r, y, proof, transcript)) {
        goto try_again;
    }
    return proof;
}

std::vector<uint8_t> RangeProof::GetTrimmedVch(Scalar& s)
{
    auto vch = s.GetVch();
    std::vector<uint8_t> vch_trimmed;

    bool take_char = false;
    for (auto c: vch) {
        if (!take_char && c != '\0') take_char = true;
        if (take_char) vch_trimmed.push_back(c);
    }
    return vch_trimmed;
}

std::optional<VerifyLoop1Result> RangeProof::VerifyLoop1(
    const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
    const Generators& gens,
    const G1Points& nonces
) {
    VerifyLoop1Result res;

    for (const std::pair<size_t, Proof>& p: indexed_proofs) {
        const Proof proof = p.second;

        const size_t M = GetInputValueVecLen(proof.Vs.Size());
        const size_t rounds = std::log2(M) + std::log2(Config::m_input_value_bits);  // pd.logM + logN;

        // check validity of proof in terms of component sizes
        auto Ls_Rs_valid = proof.Ls.Size() > 0 && proof.Ls.Size() == proof.Rs.Size();
        if (proof.Vs.Size() == 0 || !Ls_Rs_valid) return std::nullopt;

        // keep track of max size of L/R and sum of Vs sizes
        res.max_LR_len = std::max(res.max_LR_len, proof.Ls.Size());
        res.Vs_size_sum += proof.Vs.Size();

        // create ProofData from proof
        auto enriched_proof = EnrichedProof::Build(proof, res.to_invert_idx_offset, rounds, std::log2(M));
        res.proofs.push_back(enriched_proof);

        // add w's and y to to_invert list
        for (size_t i = 0; i < rounds; ++i) {
            res.to_invert.Add(enriched_proof.ws[i]);
        }
        res.to_invert.Add(enriched_proof.y);

        // advance the to_invert index offset
        res.to_invert_idx_offset += (rounds + 1);
    }
    return res;
}

std::optional<VerifyLoop2Result> RangeProof::VerifyLoop2(
    const std::vector<EnrichedProof>& proofs
) {
    VerifyLoop2Result res;

    for (const EnrichedProof& p: proofs) {

        if (p.proof.Ls.Size() != std::log2(Config::m_input_value_bits) + p.log_m)
            return std::nullopt;

        const size_t M = 1 << p.log_m;
        const size_t MN = M * Config::m_input_value_bits;

        Scalar weight_y = Scalar::Rand();
        Scalar weight_z = Scalar::Rand();

        res.y0 = res.y0 - (p.proof.tau_x * weight_y);

        Scalars z_pow = Scalars::FirstNPow(p.z, M, 3); // VectorPowers(pd.z, M+3);

        Scalar ip1y = VectorPowerSum(p.y, MN);

        Scalar k = (z_pow[2] * ip1y).Negate();

        for (size_t j = 1; j <= M; ++j) {
            k = k - (z_pow[j + 2] * BulletproofsRangeproof::ip12);
        }

        res.y1 = res.y1 + ((p.proof.t - (k + (p.z * ip1y))) * weight_y);

        for (size_t j = 0; j < p.proof.Vs.Size(); ++j) {
            res.multi_exp.Add(p.proof.Vs[j] * (z_pow[j+2] * weight_y));
        }

        res.multi_exp.Add(p.proof.T1 * (p.x * weight_y));
        res.multi_exp.Add(p.proof.T2 * (p.x.Square() * weight_y));
        res.multi_exp.Add(p.proof.A * weight_z);
        res.multi_exp.Add(p.proof.S * (p.x * weight_z));

        const size_t rounds = p.log_m + std::log2(Config::m_input_value_bits);

        Scalar y_inv_pow = 1;
        Scalar y_pow = 1;

        const Scalars w_invs = &inverses[p.inv_offset]; // should return Scalars
        const Scalar y_inv = inverses[p.inv_offset + rounds];

        std::vector<Scalar> w_cache(1<<rounds, 1);
        w_cache[0] = w_invs[0];
        w_cache[1] = p.ws[0];

        for (size_t j = 1; j < rounds; ++j) {
            const size_t sl = 1<<(j+1);

            for (size_t s = sl; s-- > 0; --s) {
                w_cache[s] = w_cache[s/2] * p.ws[j];
                w_cache[s-1] = w_cache[s/2] * w_invs[j];
            }
        }

        for (size_t i = 0; i < MN; ++i) {
            Scalar g_scalar = p.proof.a;
            Scalar h_scalar;

            if (i == 0) {
                h_scalar = p.proof.b;
            } else {
                h_scalar = p.proof.b * y_inv_pow;
            }

            g_scalar = g_scalar * w_cache[i];
            h_scalar = h_scalar * w_cache[(~i) & (MN-1)];

            g_scalar = g_scalar + p.z;

            Scalar tmp =
                z_pow[2 + i / Config::m_input_value_bits] *
                m_two_pows[i % Config::m_input_value_bits];
            if (i == 0) {
                h_scalar = h_scalar - (tmp + p.z);
            } else {
                h_scalar = h_scalar - ((tmp + (p.z * y_pow)) * y_inv_pow);
            }

            res.z4[i] = res.z4[i] - (g_scalar * weight_z);
            res.z5[i] = res.z5[i] - (h_scalar * weight_z);

            if (i == 0) {
                y_inv_pow = y_inv;
                y_pow = p.y;
            } else if (i != MN - 1) {
                y_inv_pow = y_inv_pow * y_inv;
                y_pow = y_pow * p.y;
            }
        }

        res.z1 = res.z1 + (p.proof.mu * weight_z);

        for (size_t i = 0; i < rounds; ++i) {
            res.multi_exp.Add(p.proof.Ls[i] * (p.ws[i].Square() * weight_z));
            res.multi_exp.Add(p.proof.Rs[i] * (w_invs[i].Square() * weight_z));
        }

        res.z3 = res.z3 + (((p.proof.t - (p.proof.a * p.proof.b)) * p.x_ip) * weight_z);
    }
}

bool RangeProof::Verify(
    const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
    const G1Points& nonces,
    const TokenId& token_id
) {
    const Generators gens = m_gf.GetInstance(token_id);

    const std::optional<VerifyLoop1Result> loop1_res = VerifyLoop1(
        indexed_proofs,
        gens,
        nonces
    );
    if (loop1_res == std::nullopt) return false; // return false if loop1 fails

    Scalars inverses = loop1_res.value().to_invert.Invert();
    size_t maxMN = 1u << loop1_res.value().max_LR_len;

    // loop2_res.base_exps will be further enriched, so not making it const
    std::optional<VerifyLoop2Result> loop2_res = VerifyLoop2(
        loop1_res.value().proofs
    );

    const Scalar y0 = loop2_res.value().y0;
    const Scalar y1 = loop2_res.value().y1;
    const Scalar z1 = loop2_res.value().z1;
    const Scalar z3 = loop2_res.value().z3;
    const Scalars z4 = loop2_res.value().z4;
    const Scalars z5 = loop2_res.value().z5;

    loop2_res.value().multi_exp.Add(gens.G.get() * (y0 - z1));
    loop2_res.value().multi_exp.Add(gens.H * (z3 - y1));

    // place Gi and Hi side by side
    // multi_exp_data needs to be maxMN * 2 long. z4 and z5 needs to be maxMN long.
    for (size_t i = 0; i < maxMN; ++i) {
        loop2_res.value().multi_exp.Add(gens.Gi.get()[i] * z4[i]);
        loop2_res.value().multi_exp.Add(gens.Hi.get()[i] * z5[i]);
    }
    G1Point m_exp = loop2_res.value().multi_exp.Sum();

    return m_exp.IsUnity(); // m_exp == bls::G1Element::Infinity();
}

std::vector<RecoveredTxInput> RangeProof::RecoverTxIns(
    const std::vector<std::pair<size_t, Proof>>& indexed_proofs,
    const G1Points& nonces,
    const TokenId& token_id
) {
    const Generators gens = m_gf.GetInstance(token_id);

    if (nonces.Size() != indexed_proofs.size() {
        throw std::runtime_error(strprintf("%s: unable to recover tx inputs since # of nonces and proofs differ", __func__));
    }

    size_t nonce_idx = 0;
    std::vector<RecoveredTxInput> tx_ins;

    // do for each nonce + proof combination
    for (size_t i = 0; i < nonces.Size(); ++i) {
        const p& = indexed_proofs[i];
        const size_t& tx_in_index = p.first;
        const Proof& proof = p.second;
        const G1Point& nonce = nonces[i];

        // TODO move this common validation somewhere

        // cannot recover if sizes of Ls and Rs differ or Vs is empty
        auto Ls_Rs_valid = proof.Ls.Size() > 0 && proof.Ls.Size() == proof.Rs.Size();
        if (proof.Vs.Size() == 0 || !Ls_Rs_valid) {
            continue;
        }

        Scalar alpha = nonce.GetHashWithSalt(1);
        Scalar rho = nonce.GetHashWithSalt(2);
        Scalar excess = (proof.mu - rho * pd.x) - alpha;
        Scalar amount = excess & Scalar(0xFFFFFFFFFFFFFFFF);
        Scalar gamma = nonce.GetHashWithSalt(100);

        // recovered input is valid only when gamma and amount match with Pedersen commitment pd.Vs[0]
        // only valid inputs will be included in the result vector
        G1Point value_commitment = (gens.G.get() * gamma) + (gens.H * amount);
        if (value_commitment != pd.Vs[0]) {
            continue;
        }

        RecoveredTxInput tx_in;
        tx_in.index = tx_in_index;
        tx_in.amount = amount.GetUint64();
        tx_in.gamma = gamma;

        // generate message and set to data
        std::vector<uint8_t> msg = GetTrimmedVch(excess >> 64);

        Scalar tau1 = nonces[nonce_idx].GetHashWithSalt(3);
        Scalar tau2 = nonces[nonce_idx].GetHashWithSalt(4);
        Scalar excess_msg_scalar = ((proof.tau_x - (tau2 * pd.x * pd.x) - (pd.z * pd.z * gamma)) * pd.x.Invert()) - tau1;
        std::vector<uint8_t> excess_msg = RangeProof::GetTrimmedVch(excess_msg_scalar);

        tx_in.message =
            std::string(msg.begin(), msg.end()) +
            std::string(excess_msg.begin(), excess_msg.end());

        tx_ins.push_back(tx_in);
    }
    return tx_ins;
}
