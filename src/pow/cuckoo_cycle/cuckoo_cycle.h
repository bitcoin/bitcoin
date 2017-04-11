// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_CUCKOO_CYCLE_H
#define BITCOIN_POW_CUCKOO_CYCLE_H

#include <thread>

#include "pow/pow.h"
#include "cuckoo_miner.h"

namespace powa {

namespace cuckoo_cycle {

class cc_challenge;

typedef std::shared_ptr<cc_challenge> cc_challenge_ref;

class cc_challenge : public challenge {
public:
    using challenge::challenge;
    uint8_t sizeshift;
    uint16_t proofsize_min;
    uint16_t proofsize_max;

    cc_challenge() : challenge() {
        config.params.resize(5);
        uint8_t* configb = &config.params[0];
        *configb = sizeshift = SIZESHIFT;
        configb += 1;
        *(uint16_t*)configb = proofsize_min = 12;
        configb += 2;
        *(uint16_t*)configb = proofsize_max = 228;
    }

    cc_challenge(const challenge& other) : challenge(other) {
        assert(config.params.size() == 5);
        uint8_t* configb = &config.params[0];
        sizeshift = *configb;
        // TODO: deal with the fact we don't support certain size shifts; currently this is ignored
        configb += 1;
        proofsize_min = *(uint16_t*)configb;
        configb += 2;
        proofsize_max = *(uint16_t*)configb;
        // TODO: validate proofsizes; must be even, max must >= min, ...
    }

    static cc_challenge_ref random_challenge(const uint8_t len = HEADERLEN - 4) {
        cc_challenge* c = new cc_challenge();
        c->params.resize(len);
        c->randomize(c->params.size());
        return cc_challenge_ref(c);
    }

    cc_challenge& operator=(const cc_challenge& other) {
        params = other.params;
        return *this;
    }

    bool operator==(const cc_challenge& other) const {
        return !memcmp(&params[0], &other.params[0], params.size());
    }
};

inline uint32_t GetNonce(solution_ref& s) { return *(uint32_t*)&s->params[0]; }
inline uint32_t* GetKeys(solution_ref& s) { return &((uint32_t*)&s->params[0])[1]; }
inline void SetNonce(solution_ref& s, uint32_t nonce) { memcpy(&s->params[0], &nonce, 4); }
inline void SetKeys(solution_ref& s, uint32_t* keys, uint16_t count) { memcpy(&s->params[4], keys, count * 4); }

class cuckoo_cycle : public pow {
public:
    std::thread* thread;
    cc_challenge_ref c;
    cuckoo_ctx* ctx;
    int next_nonce;
    static int last_err;               ///< last error after call to is_valid()
    cuckoo_cycle(cc_challenge_ref c_in, callback_ref cb_in = nullptr) : pow(POWID_CUCKOO_CYCLE, c_in, cb_in), thread(nullptr), c(c_in), next_nonce(0) {}
    ~cuckoo_cycle();

    std::string last_error_string() {
        return errstr[last_err];
    }

    bool is_valid(solution& s) const override {
        if (s.params.size() < size_t(1 + c->proofsize_min) * 4 ||
            s.params.size() > size_t(1 + c->proofsize_max) * 4) {
            return false; // 1 nonce + edges, all 32 bit
        }
        u32* values = (u32*)&s.params[0];
        u32 nonce = values[0];
        u32* keys = &values[1];
        uint16_t proofsize = (s.params.size() >> 2) - 1;
        printf("CC is_valid(): nonce=%u, proofsize=%u\n", nonce, proofsize);

        char headernonce[HEADERLEN];
        u32 hdrlen = c.get()->params.size();
        memcpy(headernonce, &c.get()->params[0], hdrlen);
        if (hdrlen < sizeof(headernonce)) memset(headernonce + hdrlen, 0, sizeof(headernonce) - hdrlen);
        ((u32 *)headernonce)[HEADERLEN/sizeof(u32)-1] = htole32(nonce);

        last_err = verify(keys, headernonce, HEADERLEN, proofsize);
        return (last_err == POW_OK);
    }

    void set_output(solution& s, std::vector<uint8_t>& output) override {
        // the output for a CC POW is the solution in full
        output.resize(s.params.size());
        memcpy(&output[0], &s.params[0], s.params.size());
    }

    void solve(uint32_t threads = 0, bool background = false, int32_t ticks = -1) override;

    int64_t expected_iteration_cycles() const override { return 150000000000; }
    int64_t expected_invprob()          const override {
        // we require 12/228 at this point, or we expect a 1/999 probability of solving
        return c->proofsize_min == 12 && c->proofsize_max == 228 ? 1 : 999;
    }

    void abort() override;

    virtual std::string to_string() const override {
        return strprintf("Cuckoo-Cycle<%p; ps=[%u, %u]>", this, c->proofsize_min, c->proofsize_max);
    }
private:
    void solve_async(uint32_t thread_count);
};

}  // namespace cuckoo_cycle

}  // namespace powa

#endif  // BITCOIN_POW_CUCKOO_CYCLE_H
