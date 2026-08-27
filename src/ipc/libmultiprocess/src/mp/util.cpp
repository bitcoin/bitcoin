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
#include <optional>
#include <pthread.h>
#include <csignal>
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

#if __has_include(<sys/prctl.h>)
#include <sys/prctl.h>
#endif

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

//! Report an error and exit from the post-fork child of a multi-threaded
//! process, where only async-signal-safe calls (like write and _exit) are
//! allowed. Accepting only a reference to a char array (in practice a string
//! literal) ensures no allocation is needed at the call site.
template <std::size_t N>
[[noreturn]] void ChildFail(const char (&msg)[N]) noexcept
{
    const ssize_t written = ::write(STDERR_FILENO, msg, N - 1);
    (void)written;
    _exit(126);
}

enum class SpawnErrorOp
{
    CLOSE,
    EXECVP,
    READ,
    FCNTL,
};

struct SpawnError {
    SpawnErrorOp which;
    int err;
};

const char* SpawnErrorName(SpawnErrorOp which)
{
    switch (which) {
    case SpawnErrorOp::CLOSE: return "close";
    case SpawnErrorOp::EXECVP: return "execvp";
    case SpawnErrorOp::READ: return "read";
    case SpawnErrorOp::FCNTL: return "fcntl";
    }
    return "unknown";
}

// Read the child's error report. Returns nullopt on success, or a SpawnError to
// throw on failure. Success is a clean EOF: read() returns 0 with nothing
// buffered because the child's write end was closed by a successful exec (via
// FD_CLOEXEC). A fully-read struct is the failure the child reported. A read()
// error, or an EOF partway through the struct (the child died mid-report), is
// surfaced as a SpawnErrorOp::READ failure rather than being mistaken for
// success. A single read() is not guaranteed to return all sizeof(SpawnError)
// bytes (the socket is SOCK_STREAM, which has no message boundaries) and may be
// interrupted by a signal, so loop until the whole struct is read.
//
// Note that a clean EOF is also what happens if the child is killed before it
// reaches exec, so this function reports success in that case. There is no good
// way to distinguish the two here, but callers will still see the failure when
// they call WaitProcess and get the child's exit status.
std::optional<SpawnError> ReadSpawnResult(int fd)
{
    SpawnError error{};
    char* buf = reinterpret_cast<char*>(&error);
    size_t remaining = sizeof(error);
    while (remaining > 0) {
        const ssize_t n = ::read(fd, buf, remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            return SpawnError{.which = SpawnErrorOp::READ, .err = errno};
        }
        if (n == 0) {
            if (remaining == sizeof(error)) return std::nullopt; // clean EOF: success
            return SpawnError{.which = SpawnErrorOp::READ, .err = EPROTO}; // torn report
        }
        buf += n;
        remaining -= static_cast<size_t>(n);
    }
    return error;
}

// Write the whole SpawnError to fd, retrying short writes and EINTR so the
// parent never sees a torn struct. Runs in the post-fork child, so it must stay
// async-signal-safe: it only calls write() and does not allocate or throw. This
// is best-effort -- if the write cannot complete there is nothing useful the
// child can do, so it stops and lets the caller _exit().
void WriteSpawnError(int fd, const SpawnError& error)
{
    const char* buf = reinterpret_cast<const char*>(&error);
    size_t remaining = sizeof(error);
    while (remaining > 0) {
        const ssize_t n = ::write(fd, buf, remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        buf += n;
        remaining -= static_cast<size_t>(n);
    }
    if (remaining > 0) {
        // The parent's read end is gone (e.g. the parent exited before the
        // child could report), so the structured error can't be delivered.
        // Leave a breadcrumb on stderr and exit. The exit code is irrelevant
        // here since no live parent remains to wait on it.
        ChildFail("SpawnProcess(child): failed and could not report error to parent\n");
    }
}

// Get rid of a child process the parent is abandoning because SpawnProcess is
// about to throw, so it is not left behind as a zombie. The child may still be
// alive, so kill it first: waiting without that could block for as long as the
// spawned program runs.
void KillAndReapChild(ProcessId pid)
{
    (void)::kill(pid, SIGKILL);
    while (::waitpid(pid, /*status=*/nullptr, /*options=*/0) == -1 && errno == EINTR) {}
}

} // namespace

// Copied from https://github.com/bitcoin/bitcoin/blob/d3e40af2597/src/util/threadnames.cpp#L21-L36
void SetOsThreadName(const char* name)
{
#if defined(PR_SET_NAME)
    // Only the first 15 characters are used (16 - NUL terminator)
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#elif defined(HAVE_PTHREAD_SETNAME_NP_3ARG)
    pthread_setname_np(pthread_self(), "%s", const_cast<char*>(name));
#elif defined(HAVE_PTHREAD_GETTHREADID_NP)
    pthread_set_name_np(pthread_self(), name);
#elif defined(__APPLE__)
    pthread_setname_np(name);
#else
    // Prevent warnings for unused parameters...
    (void)name;
#endif
}

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

std::tuple<ProcessId, SocketId> SpawnProcess(SpawnConnectInfoToArgsFn&& connect_info_to_args)
{
    auto fds{SocketPair()};
    // Only used for the child to report errors back to the parent, so a one-way
    // pipe would be sufficient, but reuse the existing SocketPair() helper.
    auto error_fds{SocketPair()};

    // Evaluate the callback and build the argv array before forking.
    //
    // The parent process may be multi-threaded and holding internal library
    // locks at fork time. In that case, running code that allocates memory or
    // takes locks in the child between fork() and exec() can deadlock
    // indefinitely. Precomputing arguments in the parent avoids this.
    const std::vector<std::string> args{connect_info_to_args(std::to_string(fds[0]))};
    const std::vector<char*> argv{MakeArgv(args)};

    ProcessId pid = fork();
    if (pid == -1) {
        const int err = errno;
        (void)close(fds[0]);
        (void)close(fds[1]);
        (void)close(error_fds[0]);
        (void)close(error_fds[1]);
        throw std::system_error(err, std::system_category(), "fork");
    }
    // Parent process closes the descriptor for socket 0, child closes the
    // descriptor for socket 1. On failure, the parent throws, but the child
    // must _exit(126) (post-fork child must not throw).
    if (close(fds[pid ? 0 : 1]) != 0) {
        const int err = errno;
        if (pid) {
            (void)close(fds[1]);
            (void)close(error_fds[0]);
            (void)close(error_fds[1]);
            KillAndReapChild(pid);
            throw std::system_error(err, std::system_category(), "close");
        }
        WriteSpawnError(error_fds[0], {.which = SpawnErrorOp::CLOSE, .err = err});
        _exit(126);
    }

    if (!pid) {
        // Child process must close all potentially open descriptors, except
        // socket 0 and the error-reporting socket 0. Do not throw, allocate, or
        // do non-fork-safe work here.
        const int maxFd = MaxFd();
        for (int fd = 3; fd < maxFd; ++fd) {
            if (fd != fds[0] && fd != error_fds[0]) {
                close(fd);
            }
        }

        // Clear FD_CLOEXEC on socket 0 so it survives exec. Doing this here
        // rather than in the parent before forking avoids a window where a
        // concurrent fork in another thread would leak the descriptor into an
        // unrelated child process (which would not clear it).
        // fcntl is async-signal-safe.
        const int fds0_flags = fcntl(fds[0], F_GETFD);
        if (fds0_flags == -1 || fcntl(fds[0], F_SETFD, fds0_flags & ~FD_CLOEXEC) == -1) {
            const int err = errno;
            WriteSpawnError(error_fds[0], {.which = SpawnErrorOp::FCNTL, .err = err});
            _exit(126);
        }

        execvp(argv[0], argv.data());

        const int err = errno;
        WriteSpawnError(error_fds[0], {.which = SpawnErrorOp::EXECVP, .err = err});
        _exit(127);
    }

    // Close the parent's copy of the child's write end.
    if (close(error_fds[0]) != 0) {
        const int err = errno;
        (void)close(error_fds[1]);
        (void)close(fds[1]);
        KillAndReapChild(pid);
        throw std::system_error(err, std::system_category(), "close");
    }

    const std::optional<SpawnError> error{ReadSpawnResult(error_fds[1])};
    (void)close(error_fds[1]);
    if (error) {
        (void)close(fds[1]);
        KillAndReapChild(pid);
        throw std::system_error(error->err, std::system_category(), SpawnErrorName(error->which));
    }
    return {pid, fds[1]};
}

SocketId StartSpawned(const SpawnConnectInfo& connect_info)
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
