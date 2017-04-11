// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_POW_H
#define BITCOIN_POW_POW_H

#include <vector>
#include <stdint.h>

#include "random.h"
#include "serialize.h"
#include "tinyformat.h"

namespace powa {

extern bool fZeroStartingNonce; ///< used for debugging to get deterministic challenges

class challenge;
class solution;
class callback;
class pow;

typedef std::shared_ptr<challenge> challenge_ref;
typedef std::shared_ptr<solution> solution_ref;
typedef std::shared_ptr<callback> callback_ref;
typedef std::shared_ptr<pow> pow_ref;

enum solver_state {
    state_ready    = 1,  //!< ready to run
    state_running  = 2,  //!< currently running
    state_paused   = 3,  //!< ran out of ticks, and waiting to resume
    state_aborted  = 4,  //!< run was aborted
    state_stopped  = 5,  //!< no longer running
    state_term     = 6,  //!< asked to terminate
};

static const uint32_t POWID_SHA256       = 1;
static const uint32_t POWID_CUCKOO_CYCLE = 2;

class container {
public:
    std::vector<uint8_t> params;
    container() {}
    container(const std::vector<uint8_t>& params_in) : params(params_in) {}
    container(const container& other) { params = other.params; }
    void set_params(const uint8_t* arr, const uint32_t size) { params = std::vector<uint8_t>(arr, arr + size); }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(params);
    }

    virtual bool operator==(const container& other) const {
        return params == other.params;
    }
};

class challenge : public container {
public:
    container config;
    using container::container;
    challenge() : container() {}
    challenge(const challenge& other) : container(other) { config = other.config; }
    void randomize(const uint32_t bytes, const uint32_t offset = 0) {
        if (params.size() < bytes) params.resize(bytes);
        GetRandBytes(&params.begin()[offset], bytes);
    }
    /**
     * Generate a random challenge of this POW type.
     * @param  size Number of random bytes to generate.
     * @return      A challenge whose params consists of size random bytes.
     */
    static challenge* random_challenge(const uint32_t size) {
        challenge* c = new challenge();
        c->randomize(size);
        return c;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(config);
        READWRITE(params);
    }
};

class solution : public container {
public:
    using container::container;
    solution() : container() {}
    template<typename T> solution(T t) {
        params.resize(sizeof(T));
        memcpy(&params[0], &t, sizeof(T));
    }
};

class pow;

class callback {
public:
    virtual ~callback() {}
    /**
     * Called whenever a solution is found to a given challenge. This method
     * should return true if the solution satisfies external requirements or
     * false if the solver should continue finding solutions.
     * @param  p The proof of work
     * @param  c The challenge
     * @param  s The solution
     * @return   Whether the solver should continue looking.
     */
    virtual bool found_solution(const pow& p, challenge_ref c, solution_ref s) {
        assert(!"not implemented found_solution callback");
    }
};

class callback_proxy : public callback {
public:
    callback* actual;
    callback_proxy(callback* actual_in) : actual(actual_in) {}
    bool found_solution(const pow& p, challenge_ref c, solution_ref s) override {
        return actual->found_solution(p, c, s);
    }
};

class pow {
public:
    uint32_t powid;
    challenge_ref c;
    callback_ref cb;
    solver_state state;
    int32_t ticks_left;

    static pow_ref create(const uint32_t powid, challenge_ref c_in, callback_ref cb_in = nullptr);

    pow(uint32_t powid_in, challenge_ref c_in, callback_ref cb_in = nullptr) : powid(powid_in), c(c_in), cb(cb_in), state(state_ready) {}

    /**
     * Determine if s is a valid solution to c.
     * @param  s A solution to this POW
     * @return   Whether the solution solves the challenge.
     */
    virtual bool is_valid(solution& s) const = 0;

    /**
     * Set the given vector to the output given the solution s.
     * @param s      The solution on which output should be based.
     * @param output The output produced by this POW.
     */
    virtual void set_output(solution& s, std::vector<uint8_t>& output) = 0;

    /**
     * Attempt to solve c in the given number of threads over the given number
     * of ticks. Whenever a solution is found, cb.found_solution is called with
     * the given solution.
     * @param threads    The number of threads to spin up to solve.
     * @param background If true, the solver will create a new thread and work from there, returning control to the caller immediately.
     * @param ticks      Number of ticks (cycles) to attempt before temporarily pausing. If -1, unlimited.
     */
    virtual void solve(uint32_t threads = 0, bool background = false, int32_t ticks = -1) = 0;

    /**
     * Stop solving.
     */
    virtual void abort() {
        if (state == state_running) state = state_term;
    }

    virtual int64_t expected_iteration_cycles() const = 0;
    virtual int64_t expected_invprob() const = 0;

    bool operator==(const pow& other) const {
        return powid == other.powid &&
            *c == *other.c;
    }

    virtual std::string to_string() const {
        return strprintf("POW<id=%u; ptr=%p>", powid, this);
    }
};

class powchain : public pow, public callback {
public:
    std::vector<pow_ref> pows;

    powchain(challenge_ref c_in = challenge_ref(new challenge()), callback_ref cb_in = nullptr) : pow(0, c_in, cb_in) {}

    void add_pow(pow_ref p) {
        pows.push_back(p);
    }

    bool found_solution(const pow& p, challenge_ref c, solution_ref s) override {
        if (p == *pows.back()) {
            if (!is_valid(*s)) return false;
            return cb->found_solution(*this, c, s);
        }
        assert(p == *pows.front());
        return cb->found_solution(p, c, s);
    }

    bool is_valid(solution& s) const override {
        solution z = s;
        std::vector<uint8_t> output;
        for (auto it = pows.rbegin(); it != pows.rend(); ++it) {
            pow* p = it->get();
            if (!p->is_valid(z)) return false;
            p->set_output(z, output);
            z = solution(output);
        }
        return true;
    }

    void set_output(solution& s, std::vector<uint8_t>& output) override {
        pows.back()->set_output(s, output);
    }

    void solve(uint32_t threads = 0, bool background = false, int32_t ticks = -1) override {
        pows.back()->cb = callback_ref(new callback_proxy(this));
        pows.back()->solve(threads, false, ticks);
        state = pows.back()->state; // TODO: deal with background case (right now using false for background always)
    }

    int64_t expected_iteration_cycles() const override {
        int64_t cycles = 0;
        for (pow_ref p : pows) {
            cycles += p->expected_iteration_cycles();
        }
        return cycles;
    }

    int64_t expected_invprob() const override {
        int64_t invprob = 1;
        for (pow_ref p : pows) {
            invprob *= p->expected_invprob();
        }
        return invprob;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (ser_action.ForRead()) {
            uint8_t pows_size;
            READWRITE(pows_size);
            pows.resize(pows_size);
            for (uint32_t i = 0; i < pows_size; i++) {
                uint32_t powid;
                challenge_ref c(new challenge());
                READWRITE(powid);
                READWRITE(*c);
                pow_ref p = pow::create(powid, c);
                pows[i] = p;
            }
            return;
        }

        // Writing to stream
        uint8_t pows_size = pows.size();
        READWRITE(pows_size);
        for (pow_ref p : pows) {
            READWRITE(p->powid);
            READWRITE(*p->c);
        }
    }

    virtual std::string to_string() const override {
        std::string s = strprintf("POW-chain<%p>[", this);
        for (pow_ref p : pows) {
            s += "\n\t" + p->to_string();
        }
        return s + "\n]";
    }
};

}  // namespace powa

#endif  // BITCOIN_POW_POW_H
