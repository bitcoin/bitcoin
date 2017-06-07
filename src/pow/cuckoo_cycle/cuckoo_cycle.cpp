// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cuckoo_cycle.h"

#include <algorithm>

// #include "../random.h"

namespace powa {

namespace cuckoo_cycle {

int cuckoo_cycle::last_err = POW_OK;

cuckoo_cycle::~cuckoo_cycle() {
    if (thread) {
        if (state == state_running) state = state_term;
        thread->join();
        delete thread;
        thread = nullptr;
    }
}

bool cuckoo_cycle::solve(uint32_t threads, bool background, int32_t ticks) {
    assert(state == state_ready || state == state_paused);
    if (external_nonce && state == state_ready && !fZeroStartingNonce) {
        // randomize nonce
        // GetRandBytes((unsigned char*)&next_nonce, sizeof(next_nonce));
        next_nonce = (int)reinterpret_cast<uint64_t>(this);
    }
    state = state_running;
    ticks_left = ticks;
    if (c->params.size() != HEADERLEN - (external_nonce ? 4 : 0)) {
        c->params.resize(HEADERLEN - (external_nonce ? 4 : 0));
    }
    if (background) {
        thread = new std::thread(&cuckoo_cycle::solve_async, this, threads ?: 1);
        return false;
    }
    return solve_async(threads ?: 1);
}

void cuckoo_cycle::abort() {
    if (pAbort) {
        printf("*** setting abort flag\n");
        *pAbort = true;
        pAbort = NULL;
    }
    if (state != state_running) return;
    state = state_term;
    thread->join();
}

bool cuckoo_cycle::solve_async(uint32_t thread_count) { // asynchronous
    int ntrims = 1 + (PART_BITS+3)*(PART_BITS+4)/2;
    int nonce  = next_nonce;
    thread_ctx* threads = new thread_ctx[thread_count];
    assert(threads);
    size_t wx = std::min((size_t)80, c->params.size() + 4);
    char* ws = new char[wx];
    memcpy(ws, &c->params[0], c->params.size());
    ctx = new cuckoo_ctx(thread_count, ntrims, 8, c->proofsize_min, c->proofsize_max);
    pAbort = &ctx->abort;
    while (state == state_running) {
        printf("CC: solve_async() entering loop\n");
        if (external_nonce) {
            ctx->setheadernonce(ws, wx, nonce);
        } else {
            ctx->prepare(ws);
        }
        printf("CC: making threads\n");
        for (uint32_t t = 0; t < thread_count; t++) {
            threads[t].id = t;
            threads[t].ctx = ctx;
            int err = pthread_create(&threads[t].thread, nullptr, worker, (void*)&threads[t]);
            assert(err == 0);
        }
        printf("CC: joining threads\n");
        for (uint32_t t = 0; t < thread_count; t++) {
            int err = pthread_join(threads[t].thread, nullptr);
            assert(err == 0);
        }
        printf("CC: joined threads; state is %s; got %u solutions\n", state == state_running ? "running" : "not running", ctx->nsols);
        // if we are aborted due to deallocation we don't want to talk to callback
        // even if we have solutions, so we break on term
        if (state != state_running) break;
        for (unsigned s = 0; s < ctx->nsols; s++) {
            nonce_t proofsize = ctx->sols[s][ctx->proofsize_max];
            printf("- solution with proofsize = %u found for nonce = %d\n", proofsize, nonce);
            solution_ref sol(new solution());
            sol->params.resize(4 * (proofsize + (external_nonce ? 1 : 0)));
            if (external_nonce) {
                SetNonce(sol, nonce);
                SetKeys(sol, ctx->sols[s], proofsize);
            } else {
                memcpy(&sol->params[0], ctx->sols[s], proofsize * 4);
                printf("solution with nonces %u, %u, %u, ..., %u\n", ctx->sols[s][0], ctx->sols[s][1], ctx->sols[s][2], ctx->sols[s][proofsize-1]);
            }
            if (cb->found_solution(*this, c, sol)) {
                state = state_stopped;
                break;
            }
        }
        printf("CC: nonce increment\n");
        nonce++;
        ticks_left -= ticks_left > -1;
        if (state == state_running && (ticks_left == 0 || !external_nonce)) {
            state = state_paused;
        }
    }
    printf("CC: leaving loop; state = %s\n", state == state_term ? "term" : state == state_paused ? "paused" : "not term or paused");
    if (state == state_term) state = state_aborted;
    delete ctx;
    delete [] ws;
    delete [] threads;
    next_nonce = nonce;
    pAbort = NULL;
    return state == state_stopped;
}

}  // namespace cuckoo_cycle

}  // namespace powa
