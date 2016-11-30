// Copyright (c) 2016 Satoshi Nakamoto
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_IOPRIO_H
#define BITCOIN_UTIL_IOPRIO_H

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <logging.h>

#if HAVE_IOPRIO_SYSCALL
int ioprio_get();
int ioprio_set(int ioprio);
int ioprio_set_idle();

class ioprio_idler {
private:
    int orig;

public:
    ioprio_idler(const bool lowprio) {
        if (!lowprio) {
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
#define IOPRIO_IDLER(lowprio)  ioprio_idler ioprio_idler_(lowprio)

#else
#define ioprio_get() ((void)-1)
#define ioprio_set(ioprio) ((void)-1)
#define ioprio_set_idle() ((void)-1)
#define IOPRIO_IDLER(lowprio)  (void)lowprio;
#endif

#endif // BITCOIN_UTIL_IOPRIO_H
