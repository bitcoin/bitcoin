// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/range_proof/proof.h>
#include <blsct/arith/range_proof/range_proof.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include <tinyformat.h>

Scalar* RangeProof::m_one = nullptr;
Scalar* RangeProof::m_two = nullptr;
Scalars* RangeProof::m_two_pows_64 = nullptr;
Scalar* RangeProof::m_inner_prod_1x2_pows_64 = nullptr;
GeneratorsFactory* RangeProof::m_gf = nullptr;

static void PrintG1(const char* name, G1Point& p)
{
    printf("%s: %s\n", name, HexStr(p.GetVch()).c_str());
}

static void PrintScalar(const char* name, Scalar& s)
{
    printf("%s: %s\n", name, HexStr(s.GetVch()).c_str());
}

RangeProof::RangeProof()
{
    if (m_is_initialized) return;
    boost::lock_guard<boost::mutex> lock(RangeProof::m_init_mutex);

    MclInitializer::Init();
    G1Point::Init();

    RangeProof::m_one = new Scalar(1);
    RangeProof::m_two = new Scalar(2);
    auto two_pows_64 = Scalars::FirstNPow(*m_two, Config::m_input_value_bits);
    RangeProof::m_two_pows_64 = new Scalars(two_pows_64);
    auto ones_64 = Scalars::RepeatN(*RangeProof::m_one, Config::m_input_value_bits);
    RangeProof::m_inner_prod_1x2_pows_64 = new Scalar((ones_64 * *RangeProof::m_two_pows_64).Sum());

    RangeProof::m_gf = new GeneratorsFactory();

    m_is_initialized = true;
}

bool RangeProof::InnerProductArgument(
    const size_t concat_input_values_in_bits,
    G1Points& Gi,
    G1Points& Hi,
    const G1Point& u,
    const Scalar& cx_factor,  // factor to multiply to cL and cR
    Scalars& a,
    Scalars& b,
    const Scalar& y,
    Proof& proof,
    CHashWriter& transcript_gen
) {
    const Scalars y_inv_pows = Scalars::FirstNPow(y.Invert(), concat_input_values_in_bits);
    size_t n = concat_input_values_in_bits;
    size_t rounds = 0;

    while (n > 1) {
        n /= 2;

        Scalar cL = (a.To(n) * b.From(n)).Sum();
        Scalar cR = (a.From(n) * b.To(n)).Sum();

        G1Point L =
            (Gi.From(n) * a.To(n)).Sum() +
            (Hi.To(n) * (rounds == 0 ? b.From(n) * y_inv_pows.To(n) : b.From(n))).Sum() +
            (u * cL * cx_factor);
        G1Point R =
            (Gi.To(n) * a.From(n)).Sum() +
            (Hi.From(n) * (rounds == 0 ? b.To(n) * y_inv_pows.From(n) : b.To(n))).Sum() +
            (u * cR * cx_factor);
        proof.Ls.Add(L);
        proof.Rs.Add(R);

        transcript_gen << L;
        transcript_gen << R;

        Scalar x = transcript_gen.GetHash();
        if (x == 0)
            return false;
        Scalar x_inv = x.Invert();

        // update Gi, Hi, a, b and y_inv_pows
        if (n > 1) {  // if the last loop, there is no need to update Gi and Hi
            Gi = (Gi.To(n) * x_inv) + (Gi.From(n) * x);
            if (rounds == 0) {
                Hi = (Hi.To(n) * y_inv_pows.To(n) * x) + (Hi.From(n) * y_inv_pows.From(n) * x_inv);
            } else {
                Hi = (Hi.To(n) * x) + (Hi.From(n) * x_inv);
            }
        }
        a = (a.To(n) * x) + (a.From(n) * x_inv);
        b = (b.To(n) * x_inv) + (b.From(n) * x);

        ++rounds;
    }

    proof.a = a[0];
    proof.b = b[0];
    proof.num_rounds = rounds;

    return true;
}

Proof RangeProof::Prove(
    Scalars& vs,
    G1Point& nonce,
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

    const size_t num_input_values_power_of_2 =
        Config::GetFirstPowerOf2GreaterOrEqTo(vs.Size());

    // this is power of 2 as well since m_input_value_bits is power of 2
    const size_t concat_input_values_in_bits =
        num_input_values_power_of_2 * Config::m_input_value_bits;

    ////////////// Proving steps
    Proof proof;

    // Initialize gammas
    Scalars gammas;
    for (size_t i = 0; i < vs.Size(); ++i) {
        auto hash = nonce.GetHashWithSalt(100 + i);
        gammas.Add(hash);
    }

    // Get Generators for the token_id
    Generators2 gens = m_gf->GetInstance(token_id);
    auto Gi = gens.GetGiSubset(concat_input_values_in_bits);
    auto Hi = gens.GetHiSubset(concat_input_values_in_bits);

    // swap H and G for testing purpose
    auto H = gens.G.get();
    auto G = gens.H;

    // This hash is updated for Fiat-Shamir throughout the proof
    CHashWriter transcript_gen(0, 0);

    // Calculate value commitments and add them to transcript
    for (size_t i = 0; i < vs.Size(); ++i) {
        auto V = (G * vs[i]) + (H * gammas[i]);
        proof.Vs.Add(V);
        transcript_gen << V;
    }
    auto gamma0 = gammas[0];
    // PrintScalar("gamma[0]", gamma0);
    auto v0 = vs[0];
    // PrintScalar("v[0]", v0);
    auto V0 = proof.Vs[0];
    // PrintG1("V[0]", V0);

    // (41)-(42)
    // Values to be obfuscated are encoded in binary and flattened to a single vector aL
    Scalars aL;   // ** size of aL can be shorter than concat_input_values_in_bits
    for (Scalar& v: vs.m_vec) {  // for each input value
        auto bits = v.GetBits();  // gets the value in binary
        for(bool bit: bits) {
            aL.Add(bit);
        }
        // pad the remaining bits with false if exist
        for(size_t i = 0; i < Config::m_input_value_bits - bits.size(); ++i) {
            aL.Add(false);
        }
    }
    // pad 0 bits at the end if aL.size < concat_input_values_in_bits
    while (aL.Size() < concat_input_values_in_bits) {
        aL.Add(false);
    }

    auto one_value_concat_bits = Scalars::RepeatN(*m_one, concat_input_values_in_bits);

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
    // first part of message + 64-bit vs[0]
    Scalar msg1_v0 = (msg1_scalar << Config::m_input_value_bits) | vs[0];

    Scalar alpha = nonce.GetHashWithSalt(1);
    alpha = alpha + msg1_v0;

    // Using generator H for alpha following the paper
    proof.A = (H * alpha) + (Gi * aL).Sum() + (Hi * aR).Sum();

    // (45)-(47)
    // Commitment to blinding vectors sL and sR (obfuscated with rho)
    auto sL = Scalars::RandVec(concat_input_values_in_bits, false);  // TODO bring false back to true
    auto sR = Scalars::RandVec(concat_input_values_in_bits, false);  // TODO bring false back to true

    auto rho = nonce.GetHashWithSalt(2);
    // Using generator H for alpha following the paper
    proof.S = (H * rho) + (Gi * sL).Sum() + (Hi * sR).Sum();

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
    Scalars zs = Scalars::RepeatN(z, concat_input_values_in_bits);
    Scalars l0 = aL - zs;

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    Scalars l1 = sL;

    // Calculation of r(0) and r(1) on page 19
    Scalars z_pow_twos;
    Scalars z_pows_from_2 = Scalars::FirstNPow(z, num_input_values_power_of_2, 2);  // z_pows excludes z^0 and z^1

    // The last term of r(X) on page 19
    for (size_t i = 0; i < num_input_values_power_of_2; ++i) {
        auto base_z_pow = z_pows_from_2[i];  // use different Scalar for each input value

        for (size_t bit_idx = 0; bit_idx < Config::m_input_value_bits; ++bit_idx) {
            z_pow_twos.Add(base_z_pow * (*m_two_pows_64)[bit_idx]);
        }
    }

    Scalars y_pows = Scalars::FirstNPow(y, concat_input_values_in_bits);
    Scalars r0 = (y_pows * (aR + zs)) + z_pow_twos;
    Scalars r1 = y_pows * sR;

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

    proof.T1 = (G * t1) + (H * tau1);
    proof.T2 = (G * t2) + (H * tau2);

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

    // resize z_pows so that the length matches with gammas
    proof.tau_x = (tau2 * x.Square()) + (tau1 * x) + (z_pows_from_2 * gammas).Sum();  // (61)
    proof.mu = alpha + (rho * x);  // (62)

    // (63)
    transcript_gen << x;
    transcript_gen << proof.tau_x;
    transcript_gen << proof.mu;
    transcript_gen << proof.t_hat;

    Scalar cx_factor = transcript_gen.GetHash();
    if (cx_factor == 0) goto retry;

///// debug

auto lhs_65_h = H * proof.tau_x;
auto rhs_65_h =
    (proof.Vs * z_pows_from_2).Sum()
    + proof.T1 * x
    + proof.T2 * x.Square()
    - (G * t1 * x)
    - (G * t2 * x.Square())
    - G * (vs * z_pows_from_2).Sum();
if (lhs_65_h != rhs_65_h)
    throw std::runtime_error(strprintf("%s: (65) H failed", __func__));
else
    printf("==============> (65) H worked\n");

Scalars z_pows = Scalars::FirstNPow(z, num_input_values_power_of_2 + 1, 2);
Scalar y_pows_sum = Scalars::FirstNPow(y, concat_input_values_in_bits).Sum();

Scalar delta_yz =
    z * y_pows_sum  // (1)
    - (z_pows[0] * y_pows_sum); // (2)
for (size_t i = 1; i <= num_input_values_power_of_2; ++i) {
    delta_yz = delta_yz - z_pows[i] * *RangeProof::m_inner_prod_1x2_pows_64;  // (3)
}

auto lhs_65_g = G * proof.t_hat;
auto rhs_65_g =
    G * delta_yz
    + (proof.Vs * z_pows_from_2).Sum()
    + (proof.T2 * x.Square())
    + (proof.T1 * x);
rhs_65_g = rhs_65_g
    - (H * tau1 * x)
    - (H * tau2 * x.Square());
for (size_t i=0; i<vs.Size(); ++i) {
    rhs_65_g = rhs_65_g - (H * gammas[i] * z.Square());  // ?????
}
if (lhs_65_g != rhs_65_g)
    throw std::runtime_error(strprintf("%s: (65) G failed", __func__));
else
    printf("==============> (65) G worked\n");

/// (66), (67)
auto y_inv_pows = Scalars::FirstNPow(y.Invert(), concat_input_values_in_bits);
auto HiPrime = Hi * y_inv_pows;

/// (66)
auto hp_exp = (y_pows * z) + z_pow_twos;
auto P66 = proof.A + (proof.S * x) + (Gi * zs.Negate()).Sum() + (HiPrime * hp_exp).Sum();

/// (67)
auto P67 = (H * proof.mu) + (Gi * l).Sum() + (HiPrime * r).Sum();

if (P66 != P67)
    throw std::runtime_error(strprintf("%s: (66) != (67)", __func__));
else
    printf("==============> (66) == (67)!!\n");

/////

    if (!InnerProductArgument(  // fails if x == 0 is generated from transcript_gen
        concat_input_values_in_bits,
        Gi,
        Hi,
        G,    // u
        cx_factor,
        l,         // a
        r,         // b
        y,
        proof,
        transcript_gen
    )) {
        goto retry;
    }
    return proof;
}

void RangeProof::ValidateProofsBySizes(
    const std::vector<Proof>& proofs
) const {
    for (const Proof& proof: proofs) {
        // proof must contain input values
        if (proof.Vs.Size() == 0)
            throw std::runtime_error(strprintf("%s: no input value", __func__));

        // invalid if # of input values are lager than maximum
        if (proof.Vs.Size() > Config::m_max_input_values)
            throw std::runtime_error(strprintf("%s: number of input values exceeds the maximum %ld",
                __func__, Config::m_max_input_values));

        // L,R keep track of aggregation history and the size should equal to # of rounds
        if (proof.Ls.Size() != proof.num_rounds)
            throw std::runtime_error(strprintf("%s: size of Ls (%ld) differs from number of rounds (%ld)",
                __func__, proof.Ls.Size(), proof.num_rounds));

        // if Ls and Rs should have the same size
        if (proof.Ls.Size() != proof.Rs.Size())
            throw std::runtime_error(strprintf("%s: size of Ls (%ld) differs from size of Rs (%ld)",
                __func__, proof.Ls.Size(), proof.Rs.Size()));
    }
}

G1Point RangeProof::VerifyLoop2(
    const std::vector<ProofWithTranscript>& proof_transcripts,
    const Generators2& gens,
    const size_t& max_mn
) const {
    G1Points points;
    Scalar h_pos_exp = 0;
    Scalar g_neg_exp = 0;
    Scalar h_neg_exp = 0;
    Scalar g_pos_exp = 0;
    Scalars gi_exps(max_mn, 0);
    Scalars hi_exps(max_mn, 0);

    // swap G and H for testing purpose
    G1Point G = gens.H;
    G1Point H = gens.G.get();

    for (const ProofWithTranscript& p: proof_transcripts) {
        Scalar weight_y = Scalar::Rand();
        Scalar weight_z = Scalar::Rand();

        Scalars z_pows_from_2 = Scalars::FirstNPow(p.z, p.num_input_values_power_2 + 1, 2); // z^2, z^3, ... // VectorPowers(pd.z, M+3);
        Scalar y_pows_sum = Scalars::FirstNPow(p.y, p.concat_input_values_in_bits).Sum(); // VectorPowerSum(p.y, MN);

        //////// (65)
        // g^t_hat * h^tau_x = V^(z^2) * g^delta_yz * T1^x * T2^(x^2)
        // g^(t_hat - delta_yz) = h^(-tau_x) * V^(z^2) * T1^x * T2^(x^2)

        // LHS (65)
        //h_pos_exp = h_pos_exp - (p.proof.tau_x * weight_y);
        h_neg_exp = h_neg_exp + p.proof.tau_x * weight_y;  // LHS (65)

        // delta(y,z) in (39)
        // = (z - z^2)*<1^n, y^n> - z^3<1^n,2^n>
        // = z*<1^n, y^n> (1) - z^2*<1^n, y^n> (2) - z^3<1^n,2^n> (3)
        Scalar delta_yz =
            p.z * y_pows_sum  // (1)
            - (z_pows_from_2[0] * y_pows_sum);  // (2)
        for (size_t i = 1; i <= p.num_input_values_power_2; ++i) {
            // multiply z^3, z^4, ..., z^(mn+3)
            delta_yz = delta_yz - z_pows_from_2[i] * *RangeProof::m_inner_prod_1x2_pows_64;  // (3)
        }
        PrintScalar("delta_yz", delta_yz);
        PrintScalar("ip12", *RangeProof::m_inner_prod_1x2_pows_64);

        // g part of LHS in (65) where delta_yz on RHS is moved to LHS
        // g^t_hat ... = ... g^delta_yz
        // g^(t_hat - delta_yz) = ...
        g_neg_exp = g_neg_exp + (p.proof.t_hat - delta_yz) * weight_y;
        auto t_hat_delta_yz = p.proof.t_hat - delta_yz;
        PrintScalar("t_hat - delta_yz", t_hat_delta_yz);

        // V^(z^2) in RHS (65)
        for (size_t i = 0; i < p.proof.Vs.Size(); ++i) {
            points.Add(p.proof.Vs[i] * (z_pows_from_2[i] * weight_y));  // multiply z^2, z^3, ...
        }

        // T1^x and T2^(x^2) in RHS (65)
        points.Add(p.proof.T1 * (p.x * weight_y));  // T1^x
        points.Add(p.proof.T2 * (p.x.Square() * weight_y));  // T2^(x^2)

///// debug
G1Points p65;
p65.Add(H * p.proof.tau_x.Negate() * weight_y);
p65.Add(G * (p.proof.t_hat - delta_yz).Negate() * weight_y);
p65.Add(p.proof.T1 * p.x * weight_y);
p65.Add(p.proof.T2 * p.x.Square() * weight_y);
for (size_t i = 0; i < p.proof.Vs.Size(); ++i) {
    p65.Add(p.proof.Vs[i] * z_pows_from_2[i] * weight_y);
}
if (!p65.Sum().IsUnity()) {
    throw std::runtime_error(strprintf("%s: (65) is not balancing", __func__));
} else {
    printf("=========================> (65) balances!!!\n");
}
/////
        //////// (66)
        // P = A * S^x * g^(-z) * (h')^(z * y^n + z^2 * 2^n)
        // exponents of g and (h') are created in a loop later

        // A and S^x in RHS (66)
        points.Add(p.proof.A * weight_z); // A
        points.Add(p.proof.S * (p.x * weight_z));  // S^x

        //////// (67), (68)

        // this loop generates exponents for gi and hi generators so that
        // when there are aggregated, they become g and h in (16)
        std::vector<Scalar> acc_xs(1 << p.proof.num_rounds, 1);  // initialize all elems to 1
        acc_xs[0] = p.inv_xs[0];
        acc_xs[1] = p.xs[0];
        for (size_t i = 1; i < p.proof.num_rounds; ++i) {
            const size_t sl = 1 << (i + 1);  // 4, 8, 16 ...
            for (long signed int s = sl - 1; s > 0; s -= 2) {
                acc_xs[s] = acc_xs[s / 2] * p.xs[i];
                acc_xs[s - 1] = acc_xs[s / 2] * p.inv_xs[i];
            }
        }

        // for all bits of concat input values, do:
        Scalar y_inv_pow(1);
        Scalar y_pow(1);
        for (size_t i = 0; i < p.concat_input_values_in_bits; ++i) {
            // g^a * h^b (16)
            Scalar gi_exp = p.proof.a * acc_xs[i];  // g^a in (16) is distributed to each generator
            Scalar hi_exp = p.proof.b *
                y_inv_pow *
                acc_xs[p.concat_input_values_in_bits - 1 - i];  // h^b in (16) is distributed to each generator. y_inv_pow to turn generator to (h')

            gi_exp = gi_exp + p.z;  // g^(-z) in RHS (66)

            // ** z^2 * 2^n in (h')^(z * y^n + z^2 * 2^n) in RHS (66)
            Scalar tmp =
                z_pows_from_2[i / Config::m_input_value_bits] *  // skipping the first 2 powers, different z_pow is assigned to each number
                (*m_two_pows_64)[i % Config::m_input_value_bits];   // power of 2 corresponding to i-th bit of the number being processed

            // ** z * y^n in (h')^(z * y^n + z^2 * 2^n) (66)
            hi_exp = hi_exp - (tmp + p.z * y_pow) * y_inv_pow;

            char buf[100];
            gi_exps.Set(i, gi_exps[i] - (gi_exp * weight_z));  // (16) g^a moved to LHS
            hi_exps.Set(i, hi_exps[i] - (hi_exp * weight_z));  // (16) h^b moved to LHS

            // update y_pow and y_inv_pow to the next power
            y_inv_pow = y_inv_pow * p.inv_y;
            y_pow = y_pow * p.y;
        }

        h_neg_exp = h_neg_exp + p.proof.mu * weight_z;  // ** h^mu (67) RHS

        // add L and R of all rounds to RHS (66) which equals P to generate the P of the final round on LHS (16)
        for (size_t i = 0; i < p.proof.num_rounds; ++i) {
            points.Add(p.proof.Ls[i] * (p.xs[i].Square() * weight_z));
            points.Add(p.proof.Rs[i] * (p.inv_xs[i].Square() * weight_z));
        }

        g_pos_exp = g_pos_exp + ((p.proof.t_hat - p.proof.a * p.proof.b) * p.cx_factor * weight_z);
    }
    // generate points from aggregated exponents from G, H, Gi and Hi generators
    auto g_exp = g_pos_exp - g_neg_exp;
    auto h_exp = h_pos_exp - h_neg_exp;

    // swap exponents for testing purpose
    auto tmp = g_exp;
    g_exp = h_exp;
    h_exp = tmp;

    PrintScalar("g_exp", g_exp);
    PrintScalar("h_exp", h_exp);
    points.Add(G * (g_pos_exp - g_neg_exp));
    points.Add(H * (h_pos_exp - h_neg_exp));

    auto Gi = gens.GetGiSubset(max_mn);
    auto Hi = gens.GetHiSubset(max_mn);

    char buf[100];
    for (size_t i = 0; i < max_mn; ++i) {
        points.Add(Gi[i] * gi_exps[i]);
        points.Add(Hi[i] * hi_exps[i]);

    }

    // should be aggregated to zero if proofs are all valid
    return points.Sum();
}

bool RangeProof::Verify(
    const std::vector<Proof>& proofs,
    const TokenId& token_id
) const {
    ValidateProofsBySizes(proofs);

    std::vector<ProofWithTranscript> proof_transcripts;
    size_t max_num_rounds = 0;
    size_t Vs_size_sum = 0;

    for (const Proof& proof: proofs) {
        // update max # of rounds and sum of all V bits
        max_num_rounds = std::max(max_num_rounds, proof.Ls.Size());
        Vs_size_sum += proof.Vs.Size();

        // derive transcript from the proof
        auto proof_transcript = ProofWithTranscript::Build(proof);
        proof_transcripts.push_back(proof_transcript);
    }

    const size_t max_mn = 1 << max_num_rounds;
    const Generators2 gens = m_gf->GetInstance(token_id);

    G1Point point_sum = VerifyLoop2(
        proof_transcripts,
        gens,
        max_mn
    );
    return point_sum.IsUnity();
}

std::vector<RecoveredAmount> RangeProof::RecoverAmounts(
    const std::vector<AmountRecoveryReq>& reqs,
    const TokenId& token_id
) const {
    std::vector<RecoveredAmount> ret;  // will contain result of successful requests only

    for (const AmountRecoveryReq& req: reqs) {
        const Generators2 gens = m_gf->GetInstance(token_id);

        // swap G and H for testing purpose
        G1Point G = gens.H;
        G1Point H = gens.G.get();

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
// create 0xFFFFFFFFFFFFFFFF mask
Scalar int64_max(INT64_MAX);
Scalar one(1);
Scalar mask = (int64_max << 1) + one;
        const Scalar input_value0 = message_v0 & mask; //Scalar(0xFFFFFFFFFFFFFFFF);

        // skip this tx_in if recovered input value 0 commitment doesn't match with Vs[0]
        G1Point input_value0_commitment = (G * input_value0_gamma) + (H * input_value0);
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
