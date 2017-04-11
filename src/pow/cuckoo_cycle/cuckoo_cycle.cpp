// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cuckoo_cycle.h"

#include <algorithm>

#include "../random.h"

namespace powa {

namespace cuckoo_cycle {

int cuckoo_cycle::last_err = POW_OK;

cuckoo_cycle::~cuckoo_cycle() {
    if (thread) {
        if (state == state_running) {
            state = state_term;
            thread->join();
        }
        delete thread;
        thread = nullptr;
    }
}

void cuckoo_cycle::solve(uint32_t threads, bool background, int32_t ticks) {
    assert(state == state_ready || state == state_paused);
    if (state == state_ready && !fZeroStartingNonce) {
        // randomize nonce
        GetRandBytes((unsigned char*)&next_nonce, sizeof(next_nonce));
    }
    state = state_running;
    ticks_left = ticks;
    if (c->params.size() != HEADERLEN - 4) {
        c->params.resize(HEADERLEN - 4);
    }
    if (background) {
        thread = new std::thread(&cuckoo_cycle::solve_async, this, threads ?: 1);
    } else {
        solve_async(threads ?: 1);
    }
}

void cuckoo_cycle::abort() {
    if (state != state_running) return;
    state = state_term;
    thread->join();
}

void cuckoo_cycle::solve_async(uint32_t thread_count) { // asynchronous
    int ntrims = 1 + (PART_BITS+3)*(PART_BITS+4)/2;
    int nonce  = next_nonce;
    thread_ctx* threads = new thread_ctx[thread_count];
    assert(threads);
    size_t wx = std::min((size_t)80, c->params.size() + 4);
    char* ws = new char[wx];
    memcpy(ws, &c->params[0], c->params.size());
    ctx = new cuckoo_ctx(thread_count, ntrims, 8, c->proofsize_min, c->proofsize_max);
    while (state == state_running) {
        ctx->setheadernonce(ws, wx, nonce);
        for (uint32_t t = 0; t < thread_count; t++) {
            threads[t].id = t;
            threads[t].ctx = ctx;
            int err = pthread_create(&threads[t].thread, nullptr, worker, (void*)&threads[t]);
            assert(err == 0);
        }
        for (uint32_t t = 0; t < thread_count; t++) {
            int err = pthread_join(threads[t].thread, nullptr);
            assert(err == 0);
        }
        // if we are aborted due to deallocation we don't want to talk to callback
        // even if we have solutions, so we break on term
        if (state != state_running) break;
        for (unsigned s = 0; s < ctx->nsols; s++) {
            nonce_t proofsize = ctx->sols[s][ctx->proofsize_max];
            printf("- solution with proofsize = %u found for nonce = %d\n", proofsize, nonce);
            solution_ref sol(new solution());
            sol->params.resize(4 * (1 + proofsize));
            SetNonce(sol, nonce);
            SetKeys(sol, ctx->sols[s], proofsize);
            if (cb->found_solution(*this, c, sol)) {
                state = state_stopped;
                break;
            }
        }
        nonce++;
        ticks_left -= ticks_left > -1;
        if (ticks_left == 0) state = state_paused;
    }
    if (state == state_term) state = state_aborted;
    delete ctx;
    delete [] ws;
    delete [] threads;
    next_nonce = nonce;
}

}  // namespace cuckoo_cycle

}  // namespace powa
