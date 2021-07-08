// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_RUN_COMMAND_H
#define BITCOIN_COMMON_RUN_COMMAND_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <util/fs.h>
#include <util/strencodings.h>

#include <string>

#if defined(ENABLE_EXTERNAL_SIGNER) && defined(BOOST_POSIX_API)
#include <fcntl.h>
#ifdef FD_CLOEXEC
#include <unistd.h>
#if defined(__GNUC__)
// Boost 1.78 requires the following workaround.
// See: https://github.com/boostorg/process/issues/235
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnarrowing"
#endif
#include <boost/process.hpp>
#include <boost/process/extend.hpp>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif // FD_CLOEXEC
#endif // ENABLE_EXTERNAL_SIGNER && BOOST_POSIX_API

class UniValue;

#if defined(ENABLE_EXTERNAL_SIGNER) && defined(BOOST_POSIX_API) && defined(FD_CLOEXEC)
/**
 * Ensure a boost::process::child has its non-std fds all closed when exec
 * is called.
 */
struct bpe_close_excess_fds : boost::process::extend::handler
{
    template<typename Executor>
    void on_exec_setup(Executor&exec) const
    {
        try {
            for (auto it : fs::directory_iterator("/dev/fd")) {
                int64_t fd;
                if (!ParseInt64(it.path().filename().native(), &fd)) continue;
                if (fd <= 2) continue;  // leave std{in,out,err} alone
                ::fcntl(fd, F_SETFD, ::fcntl(fd, F_GETFD) | FD_CLOEXEC);
            }
        } catch (...) {
            // TODO: maybe log this - but we're in a child process, so maybe non-trivial!
        }
    }
};
#define HAVE_BPE_CLOSE_EXCESS_FDS
#endif

/**
 * Execute a command which returns JSON, and parse the result.
 *
 * @param str_command The command to execute, including any arguments
 * @param str_std_in string to pass to stdin
 * @return parsed JSON
 */
UniValue RunCommandParseJSON(const std::string& str_command, const std::string& str_std_in="");

#endif // BITCOIN_COMMON_RUN_COMMAND_H
