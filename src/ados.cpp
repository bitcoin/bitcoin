// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "ados.h"
#include "net.h"
#include "netmessagemaker.h"
#include "hash.h"
#include "arith_uint256.h"
#include "key.h"
#include "pow/sha256/sha256.h"
#include "pow/cuckoo_cycle/cuckoo_cycle.h"

#include <vector>
#include <map>

namespace ados {

connection_challenge::connection_challenge(CNode& peer, offer_ref o_in) : callback(o_in), addr(peer.addr) {}

bool connection_challenge::solved_challenge(powa::solution_ref sol) {
    o->sol = sol;
    printf("reconnecting to peer %s\n", addr.ToString().c_str());
    g_connman.get()->OpenNetworkConnection(addr, false, NULL, NULL, false, false, true, o);
    return true;
}

struct task {
    uint32_t investment; ///< ticks per attempt
    offer_ref o;         ///< offer associated with task
    task(uint32_t investment_in, offer_ref o_in) : investment(investment_in), o(o_in) {}

    bool operator==(const task& other) const { return investment == other.investment && o.get() == other.o.get(); }
};

struct expiring_hash {
    int64_t expiration;
    uint256 hash;
    bool operator<(const expiring_hash& other) const { return expiration < other.expiration || (expiration == other.expiration && hash < other.hash); }
};

CKey* ados_key = NULL;
std::set<expiring_hash> known_challenges;
std::vector<task> tasks;
std::thread* thread;
std::atomic<bool> solving(false);

void make_keypair() {
    ados_key = new CKey();
    ados_key->MakeNewKey(true);
}

void solve_async() {
    while (tasks.size() > 0) {
        for (uint32_t i = 0; i < tasks.size(); ++i) {
            task t = tasks[i];
            t.o->pow->solve(0, t.investment);
            if (t.o->pow->state == powa::solver_state::state_stopped) {
                // solver finished
                assert(tasks[i] == t);
                tasks.erase(tasks.begin() + i);
                --i;
            }
        }
        solving = tasks.size() > 0;
    }
}

void solve() {
    if (solving) return;
    solving = true;
    thread = new std::thread(solve_async);
}

void begin_solving(offer_ref o, uint32_t investment) {
    printf("- begin solving with cb %p and solver %p\n", o->pow->cb.get(), o->pow.get());
    tasks.emplace_back(investment, o);
    solve();
}

void challenge_peer(CNode& peer, uint32_t purpose, double pressure) {
    printf("- challenging peer: pressure = %.4lf\n", pressure);
    const CNetMsgMaker msgMaker(INIT_PROTO_VERSION);
    int64_t creation = GetTime();
    int64_t expiration = creation + 600 * (1.0 + pressure);
    double probtgt = 1.0 / (1.0 + pressure * pressure * 15.0); // 0, 0.26, 0.37, 0.45, ..., 0.89, 0.93, 0.97, 1.0 (p = sqrt((v-1)/15))
    uint32_t target_cmpct = arith_uint256().SetProbabilityTarget(probtgt).GetCompact(false);

    powa::challenge_ref chlg(new powa::sha256challenge(target_cmpct, 0, 0, 0));
    powa::cuckoo_cycle::cc_challenge_ref chlg2 = powa::cuckoo_cycle::cc_challenge::random_challenge();
    powa::pow_ref alg(new powa::sha256(chlg));
    powa::pow_ref alg2(new powa::cuckoo_cycle::cuckoo_cycle(chlg2));
    powa::powchain chain;
    chain.add_pow(alg);
    chain.add_pow(alg2);

    if (!ados_key) make_keypair();
    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << chain << purpose << expiration;
    uint256 sighash = ss.GetHash();
    printf("outgoing challenge hash = %s\n", sighash.ToString().c_str());
    std::vector<unsigned char> vchSig;
    ados_key->Sign(sighash, vchSig);

    g_connman.get()->PushMessage(&peer, msgMaker.Make(
        NetMsgType::CHALLENGE,
        chain,
        purpose,
        expiration,
        vchSig
    ));
}

bool check_challenge_hash(const int64_t expiration, uint256& sighash) {
    int64_t now = GetTime();
    bool result = true;
    auto it = known_challenges.begin();
    printf("check challenge hash for expiration %lld, hash %s\n", expiration, sighash.ToString().c_str());
    while (it != known_challenges.end()) {
        auto curr = it++;
        expiring_hash eh = *curr;
        printf("got %lld with %s\n", eh.expiration, eh.hash.ToString().c_str());
        result &= eh.hash != sighash;
        if (eh.expiration < now) {
            known_challenges.erase(curr);
        }
    }
    printf("challenge hash check result = %s\n", result ? "true" : "false");
    if (result) {
        known_challenges.insert({ expiration, sighash });
    }
    return result;
}

bool check_solution(offer_ref o) {
    printf("checking solution\n");

    int64_t now = GetTime();
    if (o->expiration < now) {
        printf("\texpired\n");
        return false; // expired
    }

    // firstly, we determine if this is a challenge we issued that we haven't received a solution to yet

    if (!ados_key) {
        printf("\tno ados key (we have issued no challenges since startup)\n");
        return false; // we have issued no challenges
    }

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << *o->chain << o->purpose << o->expiration;
    uint256 sighash = ss.GetHash();
    printf("\tchallenge hash = %s\n", sighash.ToString().c_str());

    // check challenge hash against known hashes
    if (!check_challenge_hash(o->expiration, sighash)) {
        printf("\tchallenge hash check failure (known hash)\n");
        return false;
    }

    // check signature
    if (!ados_key->GetPubKey().Verify(sighash, o->vchSig)) {
        printf("\tsig verification failed (not our challenge)\n");
        return false;
    }

    // we now know this is our challenge; check the solution
    if (!o->pow->is_valid(*o->sol)) {
        printf("\tis_valid() failed (solution invalid or does not satisfy target)\n");
        return false;
    }

    return true;
}

double expected_solution_time(offer_ref o) {
    int64_t cycles = o->chain->expected_iteration_cycles();
    int64_t attempts = o->chain->expected_invprob();
    return (cycles * attempts) / 2.195e9;
}

bool solvable(offer_ref o) {
    double est = expected_solution_time(o);
    int64_t eta = GetTime() + (int64_t)(est * 1.5);
    return (eta < o->expiration);
}

}  // namespace ados
