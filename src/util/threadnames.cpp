// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <thread>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#include <util/threadnames.h>
#include <memory.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h> // For prctl, PR_SET_NAME, PR_GET_NAME
#endif

//! Set the thread's name at the process level. Does not affect the
//! internal name.
static void SetThreadName(const char* name)
{
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
    pthread_set_name_np(pthread_self(), name);
#elif defined(MAC_OSX)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

// If we have thread_local, just keep thread ID and name in a thread_local
// global.
#if defined(HAVE_THREAD_LOCAL)

#define THREADNAME_SIZE 64

static thread_local char g_thread_name[THREADNAME_SIZE] = {0};
const char *util::ThreadGetInternalName() { return g_thread_name; }
//! Set the in-memory internal name for this thread. Does not affect the process
//! name.
static void SetInternalName(const char *name) {
    memcpy(g_thread_name, name, sizeof(g_thread_name));
}

// Without thread_local available, don't handle internal name at all.
#else

static const std::string empty_string;
const std::string& util::ThreadGetInternalName() { return empty_string; }
static void SetInternalName(std::string name) { }
#endif

void util::ThreadRename(const char* name)
{
    util::ThreadRenameWithWorker(name, -1);
}

void util::ThreadRenameWithWorker(const char *name, int worker)
{
    char buf[THREADNAME_SIZE];
    snprintf(buf, sizeof(buf),
             worker == -1 ? "b-%s" : "b-%s.%d",
             name,
             worker == -1 ? 0 : worker);
    SetThreadName(buf);
    SetInternalName(buf);
}

void util::ThreadSetInternalName(const char *name)
{
    SetInternalName(name);
}
