// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/util.h>

#include <kj/test.h>

#include <chrono>
#include <compare>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

// Poll for child process exit using waitpid(..., WNOHANG) until the child exits
// or timeout expires. Returns true if the child exited and status_out was set.
// Returns false on timeout or error.
static bool WaitPidWithTimeout(int pid, std::chrono::milliseconds timeout, int& status_out)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const int r = ::waitpid(pid, &status_out, WNOHANG);
        if (r == pid) return true;
        if (r == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
            continue;
        }
        // waitpid error
        return false;
    }
    return false;
}

} // namespace

KJ_TEST("SpawnProcess does not run callback in child")
{
    // This test is designed to fail deterministically if fd_to_args is invoked
    // in the post-fork child: a mutex held by another parent thread at fork
    // time appears locked forever in the child.
    std::mutex target_mutex;
    std::mutex control_mutex;
    std::condition_variable control_cv;
    bool locked{false};
    bool release{false};

    // Holds target_mutex until the releaser thread updates release
    std::thread locker([&] {
        std::unique_lock<std::mutex> target_lock(target_mutex);
        {
            std::lock_guard<std::mutex> g(control_mutex);
            locked = true;
        }
        control_cv.notify_one();

        std::unique_lock<std::mutex> control_lock(control_mutex);
        control_cv.wait(control_lock, [&] { return release; });
    });

    // Wait for target_mutex to be held by the locker thread.
    {
        std::unique_lock<std::mutex> l(control_mutex);
        control_cv.wait(l, [&] { return locked; });
    }

    // Release the lock shortly after SpawnProcess starts.
    std::thread releaser([&] {
        // In the unlikely event a CI machine overshoots this delay, a
        // regression could be missed. This is preferable to spurious
        // test failures.
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        {
            std::lock_guard<std::mutex> g(control_mutex);
            release = true;
        }
        control_cv.notify_one();
    });

    int pid{-1};
    const int fd{mp::SpawnProcess(pid, [&](int child_fd) -> std::vector<std::string> {
        // If this callback runs in the post-fork child, target_mutex appears
        // locked forever (the owning thread does not exist), so this deadlocks.
        std::lock_guard<std::mutex> g(target_mutex);
        return {"true", std::to_string(child_fd)};
    })};
    ::close(fd);

    int status{0};
    // Give the child up to 1 second to exit. If it does not, terminate it and
    // reap it to avoid leaving a zombie behind.
    const bool exited{WaitPidWithTimeout(pid, std::chrono::milliseconds{1000}, status)};
    if (!exited) {
        ::kill(pid, SIGKILL);
        ::waitpid(pid, &status, /*options=*/0);
    }

    releaser.join();
    locker.join();

    KJ_EXPECT(exited, "Timeout waiting for child process to exit");
    KJ_EXPECT(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
