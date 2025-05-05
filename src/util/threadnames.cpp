// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstring>
#include <string>
#include <thread>
#include <utility>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#include <util/threadnames.h>

#if __has_include(<sys/prctl.h>)
#include <sys/prctl.h>
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
#elif defined(__APPLE__)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

/**
 * The name of the thread. We use char array instead of std::string to avoid
 * complications with running a destructor when the thread exits. Avoid adding
 * other thread_local variables.
 * @see https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=278701
 */
static thread_local char g_thread_name[128]{'\0'};
std::string util::ThreadGetInternalName() { return g_thread_name; }
//! Set the in-memory internal name for this thread. Does not affect the process
//! name.
static void SetInternalName(const std::string& name)
{
    const size_t copy_bytes{std::min(sizeof(g_thread_name) - 1, name.length())};
    std::memcpy(g_thread_name, name.data(), copy_bytes);
    g_thread_name[copy_bytes] = '\0';
}

void util::ThreadRename(const std::string& name)
{
    SetThreadName(("b-" + name).c_str());
    SetInternalName(name);
}

void util::ThreadSetInternalName(const std::string& name)
{
    SetInternalName(name);
}
