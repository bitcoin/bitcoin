// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memory>

#include "threshold.hpp"

#include "schemes.hpp"

#include <memory>

static std::unique_ptr<bls::CoreMPL> pThresholdScheme(new bls::LegacySchemeMPL);

/**
 * Inverts a prime field element using the Euclidean Extended Algorithm,
 * using bns and a custom prime modulus.
 *
 * @param[out] c			- the result.
 * @param[in] a				- the prime field element to invert.
 * @param[in] p				- the custom prime modulus.
 */
static void fp_inv_exgcd_bn(bn_t c, const bn_t u_in, const bn_t p) {
    bn_t u, v, g1, g2, q, r;

    bn_null(u);
    bn_null(v);
    bn_null(g1);
    bn_null(g2);
    bn_null(q);
    bn_null(r);

    RLC_TRY {
        bn_new(u);
        bn_new(v);
        bn_new(g1);
        bn_new(g2);
        bn_new(q);
        bn_new(r);

        /* u = a, v = p, g1 = 1, g2 = 0. */
        bn_copy(u, u_in);
        bn_copy(v, p);
        bn_set_dig(g1, 1);
        bn_zero(g2);

        /* While (u != 1. */
        while (bn_cmp_dig(u, 1) != RLC_EQ) {
            /* q = [v/u], r = v mod u. */
            bn_div_rem(q, r, v, u);
            /* v = u, u = r. */
            bn_copy(v, u);
            bn_copy(u, r);
            /* r = g2 - q * g1. */
            bn_mul(r, q, g1);
            bn_sub(r, g2, r);
            /* g2 = g1, g1 = r. */
            bn_copy(g2, g1);
            bn_copy(g1, r);
        }

        if (bn_sign(g1) == RLC_NEG) {
            bn_add(g1, g1, p);
        }
        bn_copy(c, g1);
    }
    RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    }
    RLC_FINALLY {
        bn_free(u);
        bn_free(v);
        bn_free(g1);
        bn_free(g2);
        bn_free(q);
        bn_free(r);
    };
}

namespace bls {

    namespace Poly {

        static const int nIdSize{32};

        template <typename BLSType>
        BLSType Evaluate(const std::vector<BLSType>& vecIn, const Bytes& id);

        template <typename BLSType>
        BLSType LagrangeInterpolate(const std::vector<BLSType>& vec, const std::vector<Bytes>& ids);
    } // end namespace Poly

    struct PolyOpsBase {
        bn_t order;

        PolyOpsBase() {
            bn_new(order);
            gt_get_ord(order);
        }

        void MulFP(bn_t& r, const bn_t& a, const bn_t& b) {
            bn_mul(r, a, b);
            ModOrder(r);
        }

        void AddFP(bn_t& r, const bn_t& a, const bn_t& b) {
            bn_add(r, a, b);
            ModOrder(r);
        }

        void SubFP(bn_t& r, const bn_t& a, const bn_t& b) {
            bn_sub(r, a, b);
            ModOrder(r);
        }

        void DivFP(bn_t& r, const bn_t& a, const bn_t& b) {
            bn_t iv;
            bn_new(iv);
            fp_inv_exgcd_bn(iv, b, order);
            bn_mul(r, a, iv);
            ModOrder(r);
        }

        void ModOrder(bn_t& r) {
            bn_mod(r, r, order);
        }
    };

    template<typename G>
    struct PolyOps;

    template<>
    struct PolyOps<PrivateKey> : PolyOpsBase {
        PrivateKey Add(const PrivateKey& a, const PrivateKey& b) {
            return PrivateKey::Aggregate({a, b});
        }

        PrivateKey Mul(const PrivateKey& a, const bn_t& b) {
            return a * b;
        }
    };

    template<>
    struct PolyOps<G1Element> : PolyOpsBase {
        G1Element Add(const G1Element& a, const G1Element& b) {
            return pThresholdScheme->Aggregate({a, b});
        }

        G1Element Mul(const G1Element& a, const bn_t& b) {
            return a * b;
        }
    };

    template<>
    struct PolyOps<G2Element> : PolyOpsBase {
        G2Element Add(const G2Element& a, const G2Element& b) {
            return pThresholdScheme->Aggregate({a, b});
        }

        G2Element Mul(const G2Element& a, bn_t& b) {
            return a * b;
        }
    };

    template<typename BLSType>
    BLSType Poly::Evaluate(const std::vector<BLSType>& vecIn, const Bytes& id) {
        typedef PolyOps<BLSType> Ops;
        Ops ops;
        if (vecIn.size() < 2) {
            throw std::length_error("At least 2 coefficients required");
        }

        bn_t x;
        bn_new(x);
        bn_read_bin(x, id.begin(), Poly::nIdSize);
        ops.ModOrder(x);

        BLSType y = vecIn.back();
        for (int i = (int) vecIn.size() - 2; i >= 0; i--) {
            y = ops.Mul(y, x);
            y = ops.Add(y, vecIn[i]);
        }

        bn_free(x);

        return y;
    }

    template<typename BLSType>
    BLSType Poly::LagrangeInterpolate(const std::vector<BLSType>& vec, const std::vector<Bytes>& ids) {
        typedef PolyOps<BLSType> Ops;
        Ops ops;

        if (vec.size() < 2) {
            throw std::length_error("At least 2 shares required");
        }
        if (vec.size() != ids.size()) {
            throw std::length_error("Numbers of shares and ids must be equal");
        }

        /*
            delta_{i,S}(0) = prod_{j != i} S[j] / (S[j] - S[i]) = a / b
            where a = prod S[j], b = S[i] * prod_{j != i} (S[j] - S[i])
        */
        const size_t k = vec.size();

        bn_t *delta = new bn_t[k];
        bn_t *ids2 = new bn_t[k];

        for (size_t i = 0; i < k; i++) {
            bn_new(delta[i]);
            bn_new(ids2[i]);
            bn_read_bin(ids2[i], ids[i].begin(), Poly::nIdSize);
            ops.ModOrder(ids2[i]);
        }

        bn_t a, b, v;
        bn_new(a);
        bn_new(b);
        bn_new(v);

        auto cleanup = [&](){
            bn_free(a);
            bn_free(b);
            bn_free(v);
            for (size_t i = 0; i < k; i++) {
                bn_free(delta[i]);
                bn_free(ids2[i]);
            }
            delete[] delta;
            delete[] ids2;
        };

        bn_copy(a, ids2[0]);
        for (size_t i = 1; i < k; i++) {
            ops.MulFP(a, a, ids2[i]);
        }
        if (bn_is_zero(a)) {
            cleanup();
            throw std::invalid_argument("Zero id");
        }
        for (size_t i = 0; i < k; i++) {
            bn_copy(b, ids2[i]);
            for (size_t j = 0; j < k; j++) {
                if (j != i) {
                    ops.SubFP(v, ids2[j], ids2[i]);
                    if (bn_is_zero(v)) {
                        cleanup();
                        throw std::invalid_argument("Duplicate id");
                    }
                    ops.MulFP(b, b, v);
                }
            }
            ops.DivFP(delta[i], a, b);
        }

        /*
            f(0) = sum_i f(S[i]) delta_{i,S}(0)
        */
        BLSType r;
        for (size_t i = 0; i < k; i++) {
            r = ops.Add(r, ops.Mul(vec[i], delta[i]));
        }

        cleanup();

        return r;
    }

    PrivateKey Threshold::PrivateKeyShare(const std::vector<PrivateKey>& sks, const Bytes& id) {
        return Poly::Evaluate(sks, id);
    }

    PrivateKey Threshold::PrivateKeyRecover(const std::vector<PrivateKey>& sks, const std::vector<Bytes>& ids) {
        return Poly::LagrangeInterpolate(sks, ids);
    }

    G1Element Threshold::PublicKeyShare(const std::vector<G1Element>& pks, const Bytes& id) {
        return Poly::Evaluate(pks, id);
    }

    G1Element Threshold::PublicKeyRecover(const std::vector<G1Element>& sks, const std::vector<Bytes>& ids) {
        return Poly::LagrangeInterpolate(sks, ids);
    }

    G2Element Threshold::SignatureShare(const std::vector<G2Element>& sigs, const Bytes& id) {
        return Poly::Evaluate(sigs, id);
    }

    G2Element Threshold::SignatureRecover(const std::vector<G2Element>& sigs, const std::vector<Bytes>& ids) {
        return Poly::LagrangeInterpolate(sigs, ids);
    }
    
    G2Element Threshold::Sign(const PrivateKey& privateKey, const Bytes& vecMessage) {
        return pThresholdScheme->Sign(privateKey, vecMessage);
    }

    bool Threshold::Verify(const G1Element& pubKey, const Bytes& vecMessage, const G2Element& signature) {
        return pThresholdScheme->Verify(pubKey, vecMessage, signature);
    }
}
