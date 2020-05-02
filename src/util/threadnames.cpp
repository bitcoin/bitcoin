// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <thread>
#include <mutex>
#include <unordered_map>

#if (defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__))
#include <pthread.h>
#include <pthread_np.h>
#endif

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

/** A singleton to keep track of thread names. This is created at first use and
 * leaks at shutdown to avoid initialization/destruction order problems.
 */
class ThreadNames {
    std::mutex cs;
    std::unordered_map<std::thread::id, std::string> names;

public:
    //! Static function to ensure creation at first use.
    static ThreadNames *Instance()
    {
        static ThreadNames *self = new ThreadNames();
        return self;
    }

    std::string GetName()
    {
        const std::lock_guard<std::mutex> lock(cs);
        auto i = names.find(std::this_thread::get_id());
        if (i != names.end()) {
            return i->second;
        } else {
            return "";
        }
    }

    void SetName(std::string name)
    {
        const std::lock_guard<std::mutex> lock(cs);
        names[std::this_thread::get_id()] = std::move(name);
    }
};

std::string util::ThreadGetInternalName()
{
    return ThreadNames::Instance()->GetName();
}

void util::ThreadRename(std::string&& name)
{
    SetThreadName(("b-" + name).c_str());
    ThreadNames::Instance()->SetName(name);
}

void util::ThreadSetInternalName(std::string&& name)
{
    ThreadNames::Instance()->SetName(name);
}
