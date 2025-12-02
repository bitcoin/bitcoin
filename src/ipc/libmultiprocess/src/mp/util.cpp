// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mp/config.h>
#include <mp/util.h>

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/string-tree.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <system_error>
#include <thread> // NOLINT(misc-include-cleaner) // IWYU pragma: keep
#include <unistd.h>
#include <utility>
#include <vector>

#ifdef WIN32
#include <windows.h>
#include <winsock2.h>
#else
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#endif

#ifdef __linux__
#include <sys/syscall.h>
#endif

#ifdef HAVE_PTHREAD_GETTHREADID_NP
#include <pthread_np.h>
#endif // HAVE_PTHREAD_GETTHREADID_NP

namespace fs = std::filesystem;

#ifdef WIN32
// Forward-declare internal capnp function.
namespace kj { namespace _ { int win32Socketpair(SOCKET socks[2]); } }
#endif

namespace mp {
namespace {

#ifndef WIN32
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
#endif

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
#elif defined(WIN32)
    buffer << GetCurrentThreadId();
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
                snprintf(escape, 4, "\\%02x", c);
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

std::array<SocketId, 2> SocketPair()
{
#ifdef WIN32
    SOCKET pair[2];
    KJ_WINSOCK(kj::_::win32Socketpair(pair));
#else
    int pair[2];
    KJ_SYSCALL(socketpair(AF_UNIX, SOCK_STREAM, 0, pair));
#endif
    return {pair[0], pair[1]};
}

//! Generate command line that the executable being invoked will split up using
//! the CommandLineToArgvW function, which expects arguments with spaces to be
//! quoted, quote characters to be backslash-escaped, and backslashes to also be
//! backslash-escaped, but only if they precede a quote character.
std::string CommandLineFromArgv(const std::vector<std::string>& argv)
{
    std::string out;
    for (const auto& arg : argv) {
        if (!out.empty()) out += " ";
        if (!arg.empty() && arg.find_first_of(" \t\"") == std::string::npos) {
            // Argument has no quotes or spaces so escaping not necessary.
            out += arg;
        } else {
            out += '"'; // Start with a quote
            for (size_t i = 0; i < arg.size(); ++i) {
                if (arg[i] == '\\') {
                    // Count consecutive backslashes
                    size_t backslash_count = 0;
                    while (i < arg.size() && arg[i] == '\\') {
                        ++backslash_count;
                        ++i;
                    }
                    if (i < arg.size() && arg[i] == '"') {
                        // Backslashes before a quote need to be doubled
                        out.append(backslash_count * 2 + 1, '\\');
                        out.push_back('"');
                    } else {
                        // Otherwise, backslashes remain as-is
                        out.append(backslash_count, '\\');
                        --i; // Compensate for the outer loop's increment
                    }
                } else if (arg[i] == '"') {
                    // Escape double quotes with a backslash
                    out.push_back('\\');
                    out.push_back('"');
                } else {
                    out.push_back(arg[i]);
                }
            }
            out += '"'; // End with a quote
        }
    }
    return out;
}

ProcessId SpawnProcess(SocketId socket, ConnectInfoToArgsFn&& connect_info_to_args)
{
#ifndef WIN32
    int pid{fork()};
    if (pid == -1) {
        throw std::system_error(errno, std::system_category(), "fork");
    }
    if (!pid) {
        // Child process must close all potentially open descriptors, except socket 0.
        const int maxFd = MaxFd();
        for (int fd = 3; fd < maxFd; ++fd) {
            if (fd != socket) {
                close(fd);
            }
        }

        int flags = fcntl(socket, F_GETFD);
        if (flags == -1) throw std::system_error(errno, std::system_category(), "fcntl F_GETFD");
        if (flags & FD_CLOEXEC) {
            flags &= ~FD_CLOEXEC;
            if (fcntl(socket, F_SETFD, flags) == -1) throw std::system_error(errno, std::system_category(), "fcntl F_SETFD");
        }

        ExecProcess(connect_info_to_args(std::to_string(socket)));
    }
    return pid;
#else
    // Create windows pipe to send pipe.ends[0] over to child process.
    static std::atomic<int> counter{1};
    ConnectInfo pipe_path{"\\\\.\\pipe\\mp-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(counter.fetch_add(1))};
    HANDLE pipe{CreateNamedPipeA(pipe_path.c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 0, 0, 0, nullptr)};
    KJ_WIN32(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed");

    // Start child process
    std::string cmd{CommandLineFromArgv(connect_info_to_args(pipe_path))};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    KJ_WIN32(CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi), "CreateProcess failed");
    CloseHandle(pi.hThread); // not needed

    // Duplicate socket for the child (now that we know its PID)
    WSAPROTOCOL_INFO info{};
    KJ_WINSOCK(WSADuplicateSocket(socket, pi.dwProcessId, &info), "WSADuplicateSocket failed");

    // Send socket to the child via the pipe
    KJ_WIN32(ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe failed");
    DWORD wr;
    KJ_WIN32(WriteFile(pipe, &info, sizeof(info), &wr, nullptr) && wr == sizeof(info), "WriteFile(pipe) failed");
    CloseHandle(pipe);

    return reinterpret_cast<ProcessId>(pi.hProcess);
#endif
}

SocketId StartSpawned(const ConnectInfo& connect_info)
{
#ifndef WIN32
    return std::stoi(connect_info);
#else
    HANDLE pipe = CreateFileA(connect_info.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    KJ_WIN32(pipe != INVALID_HANDLE_VALUE, "CreateFile(pipe) failed");

    WSAPROTOCOL_INFO info{};
    DWORD rd;
    KJ_WIN32(ReadFile(pipe, &info, sizeof(info), &rd, nullptr) && rd == sizeof(info), "ReadFile(pipe) failed");
    CloseHandle(pipe);

    WSADATA dontcare;
    KJ_WIN32(WSAStartup(MAKEWORD(2, 2), &dontcare) != 0, "WSAStartup() failed");

    SOCKET socket{WSASocketA(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT)};
    KJ_WINSOCK(socket, "WSASocket(FROM_PROTOCOL_INFO) failed");
    return socket;
#endif
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
        perror("execvp failed");
        if (errno == ENOENT && !args.empty()) {
            std::cerr << "Missing executable: " << fs::weakly_canonical(args.front()) << '\n';
        }
        _exit(1);
    }
}

int WaitProcess(ProcessId pid)
{
#ifndef WIN32
    int status;
    if (::waitpid(pid, &status, 0 /* options */) != pid) {
        throw std::system_error(errno, std::system_category(), "waitpid");
    }
    return status;
#else
    HANDLE handle{reinterpret_cast<HANDLE>(pid)};
    DWORD result{WaitForSingleObject(handle, INFINITE)};
    KJ_WIN32(result != WAIT_OBJECT_0, "WaitForSingleObject(child) failed");
    KJ_WIN32(GetExitCodeProcess(handle, &result), "GetExitCodeProcess failed");
    CloseHandle(handle);
    return result;
#endif
}

} // namespace mp
