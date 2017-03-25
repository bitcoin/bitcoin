// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Anti-DoS mechanism using proof of work to grant temporary privileges to
 * peers who successfully solve a given challenge. It is simultaneously an
 * interface for solving posed challenges in order to obtain said
 * privileges.
 */

#ifndef BITCOIN_ADOS_H
#define BITCOIN_ADOS_H

#include "protocol.h"
#include "pow/pow.h"

class CNode;
class CAddress;

namespace ados {

static const uint32_t PURPOSE_CONNECT = 1;

struct offer {
    powa::pow_ref pow;
    powa::powchain* chain;
    powa::solution_ref sol;
    uint32_t purpose;
    int64_t expiration;
    std::vector<unsigned char> vchSig;
    bool is_challenge;

    offer(bool is_challenge_in = true) : is_challenge(is_challenge_in) {}

    bool operator==(const offer& other) const {
        return pow.get() == other.pow.get() &&
            purpose == other.purpose &&
            expiration == other.expiration;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (!pow.get()) pow.reset(chain = new powa::powchain());
        if (!sol.get()) sol.reset(new powa::solution());
        printf("offer serializing pow %p\n", pow.get());
        READWRITE(*chain);
        printf("offer serializing purpose\n");
        READWRITE(purpose);
        READWRITE(expiration);
        READWRITE(vchSig);
        if (!is_challenge || !ser_action.ForRead()) READWRITE(*sol);
        printf("offer serializing done\n");
    }
};
typedef std::shared_ptr<offer> offer_ref;

class callback : public powa::callback {
public:
    offer_ref o;

    callback() {}
    callback(offer_ref o_in) : o(o_in) {}

    /**
     * A challenge presented to us by a peer was solved.
     * @param  challenge The challenge that was solved.
     * @param  solution  The solution.
     * @return           Whether the given solution satisfies external requirements. If false, another solution is attempted.
     */
    virtual bool solved_challenge(powa::solution_ref solution) {
        return true; // stop finding solutions; return true here to continue searching
    }

    bool found_solution(const powa::pow& pow_, powa::challenge_ref challenge, powa::solution_ref solution) override {
        return solved_challenge(solution);
    }

    virtual ~callback() {
        if (o->pow.get()) o->pow->abort();
    }
};

class connection_challenge : public callback {
public:
    CAddress addr;
    connection_challenge(CNode& peer, offer_ref o_in);
    bool solved_challenge(powa::solution_ref sol) override;
};

/**
 * Begin to solve the challenge for the given offer. The investment
 * defines the amount of ticks to use per attempt, before moving on to
 * other challenges (if any).
 * @param o          Reference to offer object.
 * @param investment Investment (tick cap) per attempt.
 */
void begin_solving(offer_ref o, uint32_t investment = 10000);

/**
 * Derive a rough estimate on the estimated time to solve the challenge for the
 * given offer.
 * @param  o Reference to offer object.
 * @return   An estimate in seconds.
 */
double expected_solution_time(offer_ref o);

/**
 * Determine if the challenge for the given offer is solvable. It is considered
 * solvable if 1.5 times the expected solution time is less than the time until
 * the offer expires.
 * @param  o Reference to offer object.
 * @return   True if the offer is most likely solvable, false if it probably isn't.
 */
bool solvable(offer_ref o);

/**
 * Send a CHALLENGE message to the given peer for the given purpose under the
 * given amount of pressure. The pressure is a value between 0 and 1 which
 * indicates how difficult the challenge should be.
 * @param peer     The CNode peer object.
 * @param purpose  The purpose of the challenge, e.g. PURPOSE_CONNECT.
 * @param pressure The current pressure value, between 0 and 1.
 */
void challenge_peer(CNode& peer, uint32_t purpose, double pressure);

/**
 * Check if the given solution is valid. This includes making sure we signed
 * the challenge, that it wasn't solved previously by someone else, that it
 * isn't expired, and that its solution validly solves the challenge.
 * @param  o Reference to offer object, which must have a solution.
 * @return   True if the offer has been successfully solved, false otherwise.
 */
bool check_solution(offer_ref o);

}  // namespace ados

#endif  // BITCOIN_ADOS_H
