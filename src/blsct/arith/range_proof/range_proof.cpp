#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <blsct/arith/range_proof/range_proof.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include "tinyformat.h"

Scalar RangeProof::m_one;
Scalar RangeProof::m_two;
Scalars RangeProof::m_two_pow_bit_size;

RangeProof::RangeProof()
{
    if (m_is_initialized) return;
    boost::lock_guard<boost::mutex> lock(RangeProof::m_init_mutex);

    MclInitializer::Init();
    G1Point::Init();

    //RangeProof::m_gens = Generators();
    RangeProof::m_one = Scalar(1);
    RangeProof::m_two = Scalar(2);
    RangeProof::m_two_pow_bit_size = Scalars::FirstNPow(m_two, Config::m_input_value_bits);

    m_is_initialized = true;
}

bool RangeProof::InnerProductArgument(
    const size_t input_value_vec_len,
    const Generators& gens,
    const Scalar& x_ip,
    const Scalars& l,
    const Scalars& r,
    const Scalar& y,
    RangeProofState& st,
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
        st.Ls.Add(
            (g_prime.From(n_prime) * a_prime.To(n_prime)).Sum() +
            (h_prime.To(n_prime) * (round == 0 ? b_prime * scale_factors.To(n_prime) : b_prime.From(n_prime))).Sum() +
            (gens.H * extra_scalar_cL)
        );
        Scalar extra_scalar_cR = cR * x_ip;
        st.Rs.Add(
            (g_prime.To(n_prime) * a_prime.From(n_prime)).Sum() +
            (h_prime.From(n_prime) * (round == 0 ? b_prime * scale_factors.From(n_prime) : b_prime.To(n_prime))).Sum() +
            (gens.H * extra_scalar_cR)
        );

        // (25)-(27)
        transcript << st.Ls[round];
        transcript << st.Rs[round];

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

    st.a = a_prime[0];
    st.b = b_prime[0];

    return true;
}

size_t RangeProof::GetInputValueVecLen(size_t input_value_vec_len)
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

RangeProofState RangeProof::Prove(
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

    // set up m, mn, log_mn
    const size_t input_value_vec_len = GetInputValueVecLen(vs.Size());

    ////////////// Proving steps
    RangeProofState st;

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
    st.Vs = G1Points(gens.H * gammas.m_vec) + G1Points(gens.G.get() * vs.m_vec);
    for (size_t i = 0; i < vs.Size(); ++i) {
        transcript << st.Vs[i];
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
    auto one_value_bits = Scalars::FirstNPow(m_one, Config::m_input_value_bits);

    // aR is aL - 1
    Scalars aR = aL - one_value_bits;

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
    st.A = (gens.H * alpha) + (gens.Gi.get() * aL).Sum() + (gens.Hi.get() * aR).Sum();

    // (45)-(47)
    // Commitment to blinding vectors sL and sR (obfuscated with rho)
    auto sL = Scalars::RandVec(Config::m_input_value_bits, true);
    auto sR = Scalars::RandVec(Config::m_input_value_bits, true);

    auto rho = nonce.GetHashWithSalt(2);
    // Using generator H for alpha following the paper
    st.S = (gens.H * rho) + (gens.Gi.get() * sL).Sum() + (gens.Hi.get() * sR).Sum();

    // (48)-(50)
    transcript << st.A;
    transcript << st.S;

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
    // aL is a concatination of all input value bits, so mn bits are needed
    Scalars z_value_bits = Scalars::FirstNPow(z, Config::m_input_value_bits);
    Scalars l0 = aL - z_value_bits;

    // l(1) is (aL - z 1^n) + sL, but this is reduced to sL
    Scalars l1 = sL;

    // Calculation of r(0) and r(1) on page 19
    Scalars z_n_times_two_n;
    Scalars z_pows = Scalars::FirstNPow(z, input_value_vec_len, 2);  // z_pows excludes 1 and z

    // The last term of r(X) on page 19
    for (size_t i = 0; i < input_value_vec_len; ++i) {
        auto base_z = z_pows[i];  // change base Scalar for each input value

        for (size_t bit_idx = 0; bit_idx < Config::m_input_value_bits; ++bit_idx) {
            z_n_times_two_n[i * Config::m_input_value_bits + bit_idx] = base_z * m_two_pow_bit_size[bit_idx];
        }
    }

    Scalars y_value_bits = Scalars::FirstNPow(y, Config::m_input_value_bits);
    Scalars r0 = (y_value_bits * (aR + z_value_bits)) + z_n_times_two_n;
    Scalars r1 = y_value_bits * sR;

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

    st.T1 = (gens.G.get() * t1) + (gens.H * tau1);
    st.T2 = (gens.G.get() * t2) + (gens.H * tau2);

    // (54)-(56)
    transcript << st.T1;
    transcript << st.T2;

    Scalar x = transcript.GetHash();
    if (x == 0)
        goto try_again;
    // x will be added to transcript later

    // (58)-(59)
    Scalars l = l0 + (l1 * x);  // l0 = aL - z_mn; l1 = sL
    Scalars r = r0 + (r1 * x);  // r0 = RHS of (58) - r1; r1 = y_mn o (sR * x)

    // LHS of (60)
    st.t_hat = (l * r).Sum();

    // RHS of (60)
    Scalar t0 = (l0 * r0).Sum();
    Scalar t_of_x = t0 + t1 * x + t2 * x.Square();

    // (60)
    if (st.t_hat != t_of_x)
        throw std::runtime_error(strprintf("%s: equality didn't hold in (60)", __func__));

    st.tau_x = (tau2 * x.Square()) + (tau1 * x) + (z_pows * gammas).Sum();  // (61)
    st.mu = alpha + (rho * x);  // (62)

    // (63)
    transcript << x;
    transcript << st.tau_x;
    transcript << st.mu;
    transcript << st.t;

    Scalar x_ip = transcript.GetHash();
    if (x_ip == 0)
        goto try_again;

    if (!InnerProductArgument(input_value_vec_len, gens, x_ip, l, r, y, st, transcript)) {
        goto try_again;
    }
    return st;
}