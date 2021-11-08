// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <util/ioprio.h>

#ifdef HAVE_IOPRIO_SYSCALL

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


#ifdef HAVE_WINDOWS_IOPRIO

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <io.h>

bool ioprio_set_file_idle(FILE * const F) {
    static const FILE_IO_PRIORITY_HINT_INFO priorityHint = {
        .PriorityHint = IoPriorityHintLow,
    };
    HANDLE hFile = _get_osfhandle(_fileno(F));

    return SetFileInformationByHandle(hFile, FileIoPriorityHintInfo, &priorityHint, sizeof(priorityHint));
}

#endif
