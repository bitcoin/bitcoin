// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <util/ioprio.h>

#if HAVE_IOPRIO_SYSCALL

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>

#ifndef IOPRIO_WHO_PROCESS
#define IOPRIO_WHO_PROCESS   1
#endif
#ifndef IOPRIO_CLASS_IDLE
#define IOPRIO_CLASS_IDLE    3
#endif
#ifndef IOPRIO_CLASS_SHIFT
#define IOPRIO_CLASS_SHIFT  13
#endif

int ioprio_get() {
    return syscall(SYS_ioprio_get, IOPRIO_WHO_PROCESS, 0);
}

int ioprio_set(const int ioprio) {
    return syscall(SYS_ioprio_set, IOPRIO_WHO_PROCESS, 0, ioprio);
}

int ioprio_set_idle() {
    return ioprio_set(7 | (IOPRIO_CLASS_IDLE << IOPRIO_CLASS_SHIFT));
}

#elif HAVE_IOPOLICY

#include <sys/resource.h>

int ioprio_get() {
    return getiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD);
}

int ioprio_set(const int ioprio) {
    return setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_THREAD, ioprio);
}

int ioprio_set_idle() {
    return ioprio_set(IOPOL_UTILITY);
}

#endif
