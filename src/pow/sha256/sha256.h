// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_SHA256_H
#define BITCOIN_POW_SHA256_H

#include <algorithm>

#include "crypto/sha256.h"
#include "arith_uint256.h"
#include "uint256.h"
#include "pow/pow.h"

namespace powa {

class sha256;
typedef std::shared_ptr<sha256> sha256_ref;

class sha256challenge : public challenge {
public:
    using challenge::challenge;
    uint32_t compact_target;
    uint8_t nonce_size;
    uint32_t nonce_offset;
    sha256challenge(const challenge& other) : challenge(other) {
        assert(config.params.size() == 9);
        uint8_t* configb = &config.params[0];
        compact_target = *(uint32_t*)configb;
        configb += 4;
        nonce_size = configb[0];
        configb++;
        nonce_offset = *(uint32_t*)configb;
    }
    sha256challenge(const uint32_t compact_target_in, const uint8_t nonce_size_in, const uint32_t nonce_offset_in, const uint32_t size) {
        compact_target = compact_target_in;
        nonce_size = nonce_size_in;
        nonce_offset = nonce_offset_in;
        uint32_t realsize = std::max(nonce_size_in + nonce_offset_in, size);
        params.resize(realsize);
        config.params.resize(9);
        uint8_t* configb = &config.params[0];
        *(uint32_t*)configb = compact_target;
        configb += 4;
        configb[0] = nonce_size;
        configb++;
        *(uint32_t*)configb = nonce_offset;
    }
    static challenge_ref random_challenge(const uint32_t compact_target_in, const uint8_t nonce_size_in, const uint32_t nonce_offset_in, const uint32_t size) {
        sha256challenge* c = new sha256challenge(compact_target_in, nonce_size_in, nonce_offset_in, size);
        c->randomize(c->params.size());
        return challenge_ref(c);
    }
};

class sha256 : public pow {
public:
    uint64_t next_nonce;
    sha256challenge* sc;
    sha256(challenge_ref c_in, callback_ref cb_in = nullptr) : pow(POWID_SHA256, c_in, cb_in), next_nonce(0), sc((sha256challenge*)c_in.get()) {}

    uint256 get_hash(solution& s) const {
        // two modes: if nonce_size is 0 we simply append, otherwise we expect
        // the solution to be the size of the nonce only
        uint32_t size;
        uint8_t* data;
        if (sc->nonce_size == 0) {
            // append
            size = sc->params.size() + s.params.size();
            data = new uint8_t[size];
            memcpy(data, &sc->params[0], sc->params.size());
            memcpy(&data[sc->params.size()], &s.params[0], s.params.size());
        } else {
            // insert nonce only, retaining remaining challenge params
            if (s.params.size() != sc->nonce_size) return uint256();
            size = std::max(uint32_t(sc->nonce_offset + sc->nonce_size), uint32_t(sc->params.size()));
            data = new uint8_t[size];
            memcpy(data, &sc->params[0], sc->params.size());
            memcpy(&data[sc->nonce_offset], &s.params[0], s.params.size());
        }
        uint256 hash;
        CSHA256().Write(data, size).Finalize(hash.begin());
        delete [] data;
        return hash;
    }

    bool is_valid(solution& s) const override {
        uint256 hash = get_hash(s);
        arith_uint256 target;
        target.SetCompact(sc->compact_target);
        printf("sha256: is_valid():\n\t%s\n>\t%s\n", target.ToString().c_str(), hash.ToString().c_str());
        return target > UintToArith256(hash);
    }

    void set_output(solution& s, std::vector<uint8_t>& output) override {
        // the output for a sha256 POW is the full hash
        output.resize(32);
        memcpy(&output[0], get_hash(s).begin(), 32);
    }

    template<typename T>
    void solve_t(T startnonce) {
        CSHA256 sha;
        uint256 hash;
        arith_uint256 target;

        uint32_t size = std::max(uint32_t(sc->nonce_offset + sc->nonce_size), uint32_t(sc->params.size()));
        uint8_t* data = new uint8_t[size];
        memcpy(data, &sc->params[0], sc->params.size());
        T& refnonce = *(T*)&data[sc->nonce_offset];
        target.SetCompact(sc->compact_target);
        bool satisfied = false;
        for (refnonce = startnonce; state == state_running && !satisfied && refnonce + 1 != startnonce; refnonce++) {
            sha.Write(data, size);
            sha.Finalize(hash.begin());
            sha.Reset();
            if (UintToArith256(hash) < target) {
                solution_ref s(new solution(refnonce));
                satisfied = cb->found_solution(*this, c, s);
            }
            ticks_left -= ticks_left > -1;
            if (ticks_left == 0) break; // ran out of ticks
        }
        next_nonce = refnonce;
        delete [] data;
        state = state == state_term ? state_aborted : state_stopped;
    }

    void solve(uint32_t threads = 0, bool background = false, int32_t ticks = -1) override {
        assert(state == state_ready || state == state_paused);
        if (background || threads > 1) printf("warning: sha256 solver does not spawn threads\n");
        if (state == state_ready && sc->nonce_size > 0 && !fZeroStartingNonce) {
            // randomize nonce
            GetRandBytes((unsigned char*)&next_nonce, sizeof(next_nonce));
        }
        state = state_running;
        ticks_left = ticks;
        solution_ref s(new solution());
        switch (sc->nonce_size) {
        case 0: if (is_valid(*s)) { cb->found_solution(*this, c, s); } return;
        case 4: solve_t((uint32_t)next_nonce); return;
        case 8: solve_t(next_nonce); return;
        default: return; // we don't support this nonce size
        }
    }

    int64_t expected_iteration_cycles() const override { return 11000; }

    int64_t expected_invprob() const override {
        arith_uint256 u;
        u.SetCompact(sc->compact_target);
        return 1.0 / u.GetProbabilityEstimate();
    }

    virtual std::string to_string() const override {
        assert(sc == c.get());
        return strprintf("SHA256<%p; target = %x; params# = %lu>", this, sc->compact_target, sc->params.size());
    }
};

}  // namespace powa

#endif  // BITCOIN_POW_SHA256_H
