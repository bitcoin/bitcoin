// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/config.h>
#include <mp/util.h>

#include <errno.h>
#include <kj/common.h>
#include <kj/string-tree.h>
#include <pthread.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <utility>
#include <vector>

#ifdef __linux__
#include <sys/syscall.h>
#endif

#ifdef HAVE_PTHREAD_GETTHREADID_NP
#include <pthread_np.h>
#endif // HAVE_PTHREAD_GETTHREADID_NP

namespace mp {
namespace {

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
    pthread_threadid_np(NULL, &tid);
    buffer << tid;
#elif defined(HAVE_PTHREAD_GETTHREADID_NP)
    buffer << pthread_getthreadid_np();
#else
    buffer << std::this_thread::get_id();
#endif

    return std::move(buffer).str();
}

std::string LogEscape(const kj::StringTree& string)
{
    const int MAX_SIZE = 1000;
    std::string result;
    string.visit([&](const kj::ArrayPtr<const char>& piece) {
        if (result.size() > MAX_SIZE) return;
        for (const char c : piece) {
            if (c == '\\') {
                result.append("\\\\");
            } else if (c < 0x20 || c > 0x7e) {
                char escape[4];
                snprintf(escape, 4, "\\%02x", c);
                result.append(escape);
            } else {
                result.push_back(c);
            }
            if (result.size() > MAX_SIZE) {
                result += "...";
                break;
            }
        }
    });
    return result;
}

int SpawnProcess(int& pid, FdToArgsFn&& fd_to_args)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        throw std::system_error(errno, std::system_category(), "socketpair");
    }

    pid = fork();
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork");
    }
    // Parent process closes the descriptor for socket 0, child closes the descriptor for socket 1.
    if (close(fds[pid ? 0 : 1]) != 0) {
        throw std::system_error(errno, std::system_category(), "close");
    }
    if (!pid) {
        // Child process must close all potentially open descriptors, except socket 0.
        const int maxFd = MaxFd();
        for (int fd = 3; fd < maxFd; ++fd) {
            if (fd != fds[0]) {
                close(fd);
            }
        }
        ExecProcess(fd_to_args(fds[0]));
    }
    return fds[1];
}

void ExecProcess(const std::vector<std::string>& args)
{
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);
    if (execvp(argv[0], argv.data()) != 0) {
        perror("execlp failed");
        _exit(1);
    }
}

int WaitProcess(int pid)
{
    int status;
    if (::waitpid(pid, &status, 0 /* options */) != pid) {
        throw std::system_error(errno, std::system_category(), "waitpid");
    }
    return status;
}

} // namespace mp
