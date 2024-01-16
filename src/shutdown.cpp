// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <shutdown.h>

#include <logging.h>
#include <node/ui_interface.h>
#include <warnings.h>

#include <config/bitcoin-config.h>

#include <assert.h>
#include <atomic>
#ifdef WIN32
#include <condition_variable>
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif

bool AbortNode(const std::string& strMessage, bilingual_str user_message)
{
    SetMiscWarning(Untranslated(strMessage));
    LogPrintf("*** %s\n", strMessage);
    if (user_message.empty()) {
        user_message = _("A fatal internal error occurred, see debug.log for details");
    }
    AbortError(user_message);
    StartShutdown();
    return false;
}

static std::atomic<bool> fRequestShutdown(false);
static std::atomic<bool> fRequestRestart(false);

#ifdef WIN32
/** On windows it is possible to simply use a condition variable. */
std::mutex g_shutdown_mutex;
std::condition_variable g_shutdown_cv;
#else
/** On UNIX-like operating systems use the self-pipe trick.
 * Index 0 will be the read end of the pipe, index 1 the write end.
 */
static int g_shutdown_pipe[2] = {-1, -1};
#endif

bool InitShutdownState()
{
#ifndef WIN32
#if HAVE_O_CLOEXEC && HAVE_DECL_PIPE2
    // If we can, make sure that the file descriptors are closed on exec()
    // to prevent interference.
    if (pipe2(g_shutdown_pipe, O_CLOEXEC) != 0) {
        return false;
    }
#else
    if (pipe(g_shutdown_pipe) != 0) {
        return false;
    }
#endif
#endif
    return true;
}

void StartShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    fRequestShutdown = true;
    g_shutdown_cv.notify_one();
#else
    // This must be reentrant and safe for calling in a signal handler, so using a condition variable is not safe.
    // Make sure that the token is only written once even if multiple threads call this concurrently or in
    // case of a reentrant signal.
    if (!fRequestShutdown.exchange(true)) {
        // Write an arbitrary byte to the write end of the shutdown pipe.
        const char token = 'x';
        while (true) {
            int result = write(g_shutdown_pipe[1], &token, 1);
            if (result < 0) {
                // Failure. It's possible that the write was interrupted by another signal.
                // Other errors are unexpected here.
                assert(errno == EINTR);
            } else {
                assert(result == 1);
                break;
            }
        }
    }
#endif
}
void StartRestart()
{
    fRequestRestart = true;
    StartShutdown();
}

void AbortShutdown()
{
    if (fRequestShutdown) {
        // Cancel existing shutdown by waiting for it, this will reset condition flags and remove
        // the shutdown token from the pipe.
        WaitForShutdown();
    }
    fRequestShutdown = false;
}

bool ShutdownRequested()
{
    return fRequestShutdown;
}

bool RestartRequested()
{
    return fRequestRestart;
}

void WaitForShutdown()
{
#ifdef WIN32
    std::unique_lock<std::mutex> lk(g_shutdown_mutex);
    g_shutdown_cv.wait(lk, [] { return fRequestShutdown.load(); });
#else
    char token;
    while (true) {
        int result = read(g_shutdown_pipe[0], &token, 1);
        if (result < 0) {
            // Failure. Check if the read was interrupted by a signal.
            // Other errors are unexpected here.
            assert(errno == EINTR);
        } else {
            assert(result == 1);
            break;
        }
    }
#endif
}
