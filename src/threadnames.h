// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THREADNAMES_H
#define BITCOIN_THREADNAMES_H

#include <map>
#include <memory>
#include <mutex>
#include <thread>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h> // For HAVE_SYS_PRCTL_H
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h> // For PR_GET_NAME
#endif

#if (defined(PR_GET_NAME) || defined(MAC_OSX))
#define CAN_READ_PROCESS_NAME
#endif

/**
 * Keeps a map of thread IDs to string names and handles system-level thread naming.
 */
class ThreadNameRegistry
{
public:
    /**
     * @return the name of the current thread, falling back to the process
     *     name if a value has not been explicitly set with Rename.
     */
    std::string GetName();

    /**
     * Name the current thread; doesn't allow colliding names unless `expect_multiple` is true.
     *
     * @param[in] name             The desired name of the process.
     * @param[in] expect_multiple  If true, allow name reuse by appending an ordered ".[n]" suffix
     *                             to the given name.
     *
     * @return true if the name was registered successfully.
     */
    bool Rename(std::string name, bool expect_multiple = false);

    /**
     * Rename the current thread at the system level, e.g. `prctrl(PR_SET_NAME, ...)`.
     */
    void RenameProcess(const char* name);

    /**
     * @return the system's name for the current thread.
     */
    std::string GetProcessName();

private:
    std::mutex m_map_lock;
    std::map<std::thread::id, std::string> m_id_to_name;
    /**
     * The number of times this name has been used to identify a thread;
     * used to generate numeric suffix.
     */
    std::map<std::string, size_t> m_name_to_count;

};

extern std::unique_ptr<ThreadNameRegistry> g_thread_names;

#endif // BITCOIN_THREADNAMES_H
