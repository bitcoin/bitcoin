// Copyright (c) 2016 Satoshi Nakamoto
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_IOPRIO_H
#define BITCOIN_UTIL_IOPRIO_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <util/system.h>

#ifdef HAVE_IOPRIO_SYSCALL
int ioprio_get();
int ioprio_set(int ioprio);
int ioprio_set_idle();

class ioprio_idler {
private:
    int orig;

public:
    ioprio_idler(const bool actually_idle) {
        if (!actually_idle) {
            orig = -1;
            return;
        }

        orig = ioprio_get();
        if (orig == -1) {
            return;
        }
        if (ioprio_set_idle() == -1) {
            orig = -1;
        }
    }

    ~ioprio_idler() {
        if (orig == -1) {
            return;
        }
        if (ioprio_set(orig) == -1) {
            LogPrintf("failed to restore ioprio\n");
        }
    }
};
#define IOPRIO_IDLER(actually_idle)  ioprio_idler ioprio_idler_(actually_idle)

#else
#define ioprio_get() ((void)-1)
#define ioprio_set(ioprio) ((void)-1)
#define ioprio_set_idle() ((void)-1)
#define IOPRIO_IDLER(actually_idle)  (void)actually_idle;
#endif

#endif // BITCOIN_UTIL_IOPRIO_H
