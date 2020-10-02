// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <set>
#include <string>
#include <cstring>
#include <algorithm>

#include "bls.hpp"
namespace bls {

const char BLS::GROUP_ORDER[] =
        "73EDA753299D7D483339D80809A1D80553BDA402FFFE5BFEFFFFFFFF00000001";

bool BLSInitResult = BLS::Init();

Util::SecureAllocCallback Util::secureAllocCallback;
Util::SecureFreeCallback Util::secureFreeCallback;

static void relic_core_initializer(void* ptr) {
    core_init();
    if (err_get_code() != STS_OK) {
        std::cout << "core_init() failed";
        // this will most likely crash the application...but there isn't much we can do
        throw std::string("core_init() failed");
    }

    const int r = ep_param_set_any_pairf();
    if (r != STS_OK) {
        std::cout << "ep_param_set_any_pairf() failed";
        // this will most likely crash the application...but there isn't much we can do
        throw std::string("ep_param_set_any_pairf() failed");
    }
}

bool BLS::Init() {
    if (ALLOC != AUTO) {
        std::cout << "Must have ALLOC == AUTO";
        throw std::string("Must have ALLOC == AUTO");
    }
#if BLSALLOC_SODIUM
    if (sodium_init() < 0) {
        std::cout << "libsodium init failed";
        throw std::string("libsodium init failed");
    }
    SetSecureAllocator(libsodium::sodium_malloc, libsodium::sodium_free);
#else
    SetSecureAllocator(malloc, free);
#endif

    core_set_thread_initializer(relic_core_initializer, nullptr);

    return true;
}

void BLS::SetSecureAllocator(Util::SecureAllocCallback allocCb, Util::SecureFreeCallback freeCb) {
    Util::secureAllocCallback = allocCb;
    Util::secureFreeCallback = freeCb;
}

void BLS::HashPubKeys(bn_t* output, size_t numOutputs,
                      std::vector<uint8_t*> const &serPubKeys,
                      std::vector<size_t> const& sortedIndices) {
    bn_t order;

    bn_new(order);
    g2_get_ord(order);

    uint8_t *pkBuffer = new uint8_t[serPubKeys.size() * PublicKey::PUBLIC_KEY_SIZE];

    for (size_t i = 0; i < serPubKeys.size(); i++) {
        memcpy(pkBuffer + i * PublicKey::PUBLIC_KEY_SIZE, serPubKeys[sortedIndices[i]], PublicKey::PUBLIC_KEY_SIZE);
    }

    uint8_t pkHash[32];
    Util::Hash256(pkHash, pkBuffer, serPubKeys.size() * PublicKey::PUBLIC_KEY_SIZE);
    for (size_t i = 0; i < numOutputs; i++) {
        uint8_t hash[32];
        uint8_t buffer[4 + 32];
        memset(buffer, 0, 4);
        // Set first 4 bytes to index, to generate different ts
        Util::IntToFourBytes(buffer, i);
        // Set next 32 bytes as the hash of all the public keys
        std::memcpy(buffer + 4, pkHash, 32);
        Util::Hash256(hash, buffer, 4 + 32);

        bn_read_bin(output[i], hash, 32);
        bn_mod_basic(output[i], output[i], order);
    }

    delete[] pkBuffer;

    CheckRelicErrors();
}

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
struct PolyOps<PrivateKey> : PolyOpsBase
{
    PrivateKey New() {
        PrivateKey r;
        return r;
    }
    PrivateKey Add(const PrivateKey& a, const PrivateKey& b) {
        return PrivateKey::AggregateInsecure({a, b});
    }
    PrivateKey Mul(const PrivateKey& a, const bn_t& b) {
        return a.Mul(b);
    }
};

template<>
struct PolyOps<PublicKey> : PolyOpsBase
{
    PublicKey New() {
        return PublicKey();
    }
    PublicKey Add(const PublicKey& a, const PublicKey& b) {
        return PublicKey::AggregateInsecure({a, b});
    }
    PublicKey Mul(const PublicKey& a, const bn_t& b) {
        return a.Exp(b);
    }
};

template<>
struct PolyOps<InsecureSignature> : PolyOpsBase
{
    InsecureSignature New() {
        return InsecureSignature();
    }
    InsecureSignature Add(const InsecureSignature& a, const InsecureSignature& b) {
        return InsecureSignature::Aggregate({a, b});
    }
    InsecureSignature Mul(const InsecureSignature& a, const bn_t& b) {
        return a.Exp(b);
    }
};

template <typename BLSType>
static inline BLSType PolyEvaluate(const std::vector<BLSType>& vec, const uint8_t* id) {
    typedef PolyOps<BLSType> Ops;
    Ops ops;

    if (vec.size() < 2) {
        throw std::string("At least 2 coefficients required");
    }

    bn_t x;
    bn_new(x);
    bn_read_bin(x, id, BLS::ID_SIZE);
    ops.ModOrder(x);

    BLSType y = vec[vec.size() - 1];

    for (int i = (int)vec.size() - 2; i >= 0; i--) {
        y = ops.Mul(y, x);
        y = ops.Add(y, vec[i]);
    }

    bn_free(x);

    return y;
}

template <typename BLSType>
static inline BLSType LagrangeInterpolate(const std::vector<BLSType>& vec, const std::vector<const uint8_t*>& ids) {
    typedef PolyOps<BLSType> Ops;
    Ops ops;

    if (vec.size() < 2) {
        throw std::string("At least 2 shares required");
    }
    if (vec.size() != ids.size()) {
        throw std::string("Numbers of shares and ids must be equal");
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
        bn_read_bin(ids2[i], ids[i], BLS::ID_SIZE);
        ops.ModOrder(ids2[i]);
    }

    bn_t a, b, v;
    bn_new(a);
    bn_new(b);
    bn_new(v);
    bn_copy(a, ids2[0]);
    for (size_t i = 1; i < k; i++) {
        ops.MulFP(a, a, ids2[i]);
    }
    if (bn_is_zero(a)) {
        delete[] delta;
        delete[] ids2;
        throw std::string("Zero id");
    }
    for (size_t i = 0; i < k; i++) {
        bn_copy(b, ids2[i]);
        for (size_t j = 0; j < k; j++) {
            if (j != i) {
                ops.SubFP(v, ids2[j], ids2[i]);
                if (bn_is_zero(v)) {
                    delete[] delta;
                    delete[] ids2;
                    throw std::string("Duplicate id");
                }
                ops.MulFP(b, b, v);
            }
        }
        ops.DivFP(delta[i], a, b);
    }

    /*
        f(0) = sum_i f(S[i]) delta_{i,S}(0)
    */
    BLSType r = ops.New();
    for (size_t i = 0; i < k; i++) {
        r = ops.Add(r, ops.Mul(vec[i], delta[i]));
        bn_free(delta[x]);
    }
    delete[] delta;
    delete[] ids2;

    return r;
}

PrivateKey BLS::PrivateKeyShare(const std::vector<PrivateKey>& sks, const uint8_t *id) {
    return PolyEvaluate(sks, id);
}

PrivateKey BLS::RecoverPrivateKey(const std::vector<PrivateKey>& sks, const std::vector<const uint8_t*>& ids) {
    return LagrangeInterpolate(sks, ids);
}

PublicKey BLS::PublicKeyShare(const std::vector<PublicKey>& pks, const uint8_t *id) {
    return PolyEvaluate(pks, id);
}

PublicKey BLS::RecoverPublicKey(const std::vector<PublicKey>& sks, const std::vector<const uint8_t*>& ids) {
    return LagrangeInterpolate(sks, ids);
}

InsecureSignature BLS::SignatureShare(const std::vector<InsecureSignature>& sks, const uint8_t* id) {
    return PolyEvaluate(sks, id);
}

InsecureSignature BLS::RecoverSig(const std::vector<InsecureSignature>& sigs, const std::vector<const uint8_t*>& ids) {
    return LagrangeInterpolate(sigs, ids);
}

PublicKey BLS::DHKeyExchange(const PrivateKey& privKey, const PublicKey& pubKey) {
    if (!privKey.keydata) {
        throw std::string("keydata not initialized");
    }
    PublicKey ret = pubKey.Exp(*privKey.keydata);
    CheckRelicErrors();
    return ret;
}

void BLS::CheckRelicErrors() {
    if (!core_get()) {
        throw std::string("Library not initialized properly. Call BLS::Init()");
    }
    if (core_get()->code != STS_OK) {
        core_get()->code = STS_OK;
        throw std::string("Relic library error");
    }
}
} // end namespace bls
