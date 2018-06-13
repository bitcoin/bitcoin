// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <atomic>
#include <thread>

#include <threadutil.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h> // For prctl, PR_SET_NAME, PR_GET_NAME
#endif

/**
 * Set the thread's name at the process level. Does not affect the
 * internal name.
 */
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

/*
 * If we have thread_local, just keep thread ID and name in a thread_local
 * global.
 */
#if defined(HAVE_THREAD_LOCAL)

static thread_local std::string g_thread_name;

std::string thread_util::GetInternalName() {
    return g_thread_name;
}

/**
 * Set the in-memory internal name for this thread. Does not affect the process
 * name.
 */
static void SetInternalName(const std::string& name)
{
    g_thread_name = name;
}

/**
 * Without thread_local available, don't handle internal name at all.
 */
#else

std::string thread_util::GetInternalName()
{
    return "";
}

static void SetInternalName(const std::string& name) { }

#endif

void thread_util::Rename(const std::string& name)
{
    SetThreadName(("bitcoin-" + name).c_str());
    SetInternalName(name);
}
