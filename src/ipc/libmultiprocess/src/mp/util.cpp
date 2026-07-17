// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/config.h>
#include <mp/util.h>

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/string-tree.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <system_error>
#include <thread> // NOLINT(misc-include-cleaner) // IWYU pragma: keep
#include <unistd.h>
#include <utility>
#include <vector>

#ifdef __linux__
#include <sys/syscall.h>
#endif

#ifdef HAVE_PTHREAD_GETTHREADID_NP
#include <pthread_np.h>
#endif // HAVE_PTHREAD_GETTHREADID_NP

extern "C" char **environ; // NOLINT(readability-redundant-declaration)

namespace mp {
namespace {

std::vector<char*> MakeArgv(const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    return argv;
}

//! Return highest possible file descriptor.
size_t MaxFd()
{
    struct rlimit nofile;
    if (getrlimit(RLIMIT_NOFILE, &nofile) == 0) {
        return nofile.rlim_cur - 1;
    } else {
        return 1023;
    }
}

} // namespace

std::string ThreadName(const char* exe_name)
{
    char thread_name[16] = {0};
#ifdef HAVE_PTHREAD_GETNAME_NP
    pthread_getname_np(pthread_self(), thread_name, sizeof(thread_name));
#endif // HAVE_PTHREAD_GETNAME_NP

    std::ostringstream buffer;
    buffer << (exe_name ? exe_name : "") << "-" << getpid() << "/";

    if (thread_name[0] != '\0') {
        buffer << thread_name << "-";
    }

    // Prefer platform specific thread ids over the standard C++11 ones because
    // the former are shorter and are the same as what gdb prints "LWP ...".
#ifdef __linux__
    buffer << syscall(SYS_gettid);
#elif defined(HAVE_PTHREAD_THREADID_NP)
    uint64_t tid = 0;
    pthread_threadid_np(nullptr, &tid);
    buffer << tid;
#elif defined(HAVE_PTHREAD_GETTHREADID_NP)
    buffer << pthread_getthreadid_np();
#else
    buffer << std::this_thread::get_id();
#endif

    return std::move(buffer).str();
}

std::string LogEscape(const kj::StringTree& string, size_t max_size)
{
    std::string result;
    string.visit([&](const kj::ArrayPtr<const char>& piece) {
        if (result.size() > max_size) return;
        for (const char c : piece) {
            if (c == '\\') {
                result.append("\\\\");
            } else if (c < 0x20 || c > 0x7e) {
                char escape[4];
                snprintf(escape, sizeof(escape), "\\%02x", static_cast<unsigned char>(c));
                result.append(escape);
            } else {
                result.push_back(c);
            }
            if (result.size() > max_size) {
                result += "...";
                break;
            }
        }
    });
    return result;
}

std::tuple<ProcessId, SocketId> SpawnProcess(const std::function<std::vector<std::string>(std::string)>& spawn_argv)
{
    auto fds{SocketPair()};

    // Evaluate the callback and build the argv array before forking.
    //
    // The parent process may be multi-threaded and holding internal library
    // locks at fork time. In that case, running code that allocates memory or
    // takes locks in the child between fork() and exec() can deadlock
    // indefinitely. Precomputing arguments in the parent avoids this.
    const std::vector<std::string> args{spawn_argv(std::to_string(fds[0]))};
    const std::vector<char*> argv{MakeArgv(args)};

    // Clear FD_CLOEXEC on fds[0] before forking so it survives exec in the child.
    int fds0_flags;
    KJ_SYSCALL(fds0_flags = fcntl(fds[0], F_GETFD));
    KJ_SYSCALL(fcntl(fds[0], F_SETFD, fds0_flags & ~FD_CLOEXEC));

    ProcessId pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork");
    }
    // Parent process closes the descriptor for socket 0, child closes the
    // descriptor for socket 1. On failure, the parent throws, but the child
    // must _exit(126) (post-fork child must not throw).
    if (close(fds[pid ? 0 : 1]) != 0) {
        if (pid) {
            (void)close(fds[1]);
            throw std::system_error(errno, std::system_category(), "close");
        }
        static constexpr char msg[] = "SpawnProcess(child): close(fds[1]) failed\n";
        const ssize_t writeResult = ::write(STDERR_FILENO, msg, sizeof(msg) - 1);
        (void)writeResult;
        _exit(126);
    }

    if (!pid) {
        // Child process must close all potentially open descriptors, except
        // socket 0. Do not throw, allocate, or do non-fork-safe work here.
        const int maxFd = MaxFd();
        for (int fd = 3; fd < maxFd; ++fd) {
            if (fd != fds[0]) {
                close(fd);
            }
        }

        execvp(argv[0], argv.data());
        // NOTE: perror() is not async-signal-safe; calling it here in a
        // post-fork child may deadlock in multithreaded parents.
        // TODO: Report errors to the parent via a pipe (e.g. write errno)
        // so callers can get diagnostics without relying on perror().
        perror("execvp failed");
        _exit(127);
    }
    return {pid, fds[1]};
}

SocketId StartSpawned(const std::string& connect_info)
{
    try {
        return std::stoi(connect_info);
    } catch (const std::exception&) {
        throw std::system_error(EINVAL, std::system_category(),
            std::string("StartSpawned: invalid connect_info '") + connect_info + "'");
    }
}

std::array<SocketId, 2> SocketPair()
{
    int pair[2];
    KJ_SYSCALL(socketpair(AF_UNIX, SOCK_STREAM, 0, pair));
    KJ_SYSCALL(fcntl(pair[0], F_SETFD, FD_CLOEXEC));
    KJ_SYSCALL(fcntl(pair[1], F_SETFD, FD_CLOEXEC));
    return {pair[0], pair[1]};
}

ProcessId StartProcess(const std::vector<std::string>& args)
{
    const std::vector<char*> argv{MakeArgv(args)};
    ProcessId pid;
    if (int err = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), ::environ)) {
        KJ_FAIL_SYSCALL("posix_spawnp", err, args.front());
    }
    return pid;
}

int WaitProcess(ProcessId pid)
{
    int status;
    if (::waitpid(pid, &status, /*options=*/0) != pid) {
        throw std::system_error(errno, std::system_category(), "waitpid");
    }
    return status;
}

} // namespace mp
