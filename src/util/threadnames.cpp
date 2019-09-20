// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitgreen-config.h>
#endif

#include <atomic>
#include <map>
#include <thread>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

#include <ctpl.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/time.h>
#include <util/threadnames.h>

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

static thread_local std::string g_thread_name;
const std::string& util::ThreadGetInternalName() { return g_thread_name; }
//! Set the in-memory internal name for this thread. Does not affect the process
//! name.
static void SetInternalName(std::string name) { g_thread_name = std::move(name); }

// Without thread_local available, don't handle internal name at all.
#else

static const std::string empty_string;
const std::string& util::ThreadGetInternalName() { return empty_string; }
static void SetInternalName(std::string name) { }
#endif

void util::ThreadRename(std::string&& name)
{
    SetThreadName(("bitgreen-" + name).c_str());
    SetInternalName(std::move(name));
}

void RenameThreadPool(ctpl::thread_pool& tp, const char* baseName)
{
    auto cond = std::make_shared<std::condition_variable>();
    auto mutex = std::make_shared<std::mutex>();
    std::atomic<int> doneCnt(0);
    std::map<int, std::future<void> > futures;

    for (int i = 0; i < tp.size(); i++) {
        futures[i] = tp.push([baseName, i, cond, mutex, &doneCnt](int threadId) {
            util::ThreadRename(strprintf("%s-%d", baseName, i).c_str());
            std::unique_lock<std::mutex> l(*mutex);
            doneCnt++;
            cond->wait(l);
        });
    }

    do {
        // Always sleep to let all threads acquire locks
        MilliSleep(10);
        // `doneCnt` should be at least `futures.size()` if tp size was increased (for whatever reason),
        // or at least `tp.size()` if tp size was decreased and queue was cleared
        // (which can happen on `stop()` if we were not fast enough to get all jobs to their threads).
    } while (doneCnt < futures.size() && doneCnt < tp.size());

    cond->notify_all();

    // Make sure no one is left behind, just in case
    for (auto& pair : futures) {
        auto& f = pair.second;
        if (f.valid() && f.wait_for(std::chrono::milliseconds(2000)) == std::future_status::timeout) {
            LogPrintf("%s: %s-%d timed out\n", __func__, baseName, pair.first);
            // Notify everyone again
            cond->notify_all();
            break;
        }
    }
}
