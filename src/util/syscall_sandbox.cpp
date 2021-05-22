// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <util/syscall_sandbox.h>

#if defined(USE_SYSCALL_SANDBOX)
#include <logging.h>
#include <tinyformat.h>
#include <util/threadnames.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/unistd.h>
#include <signal.h>
#include <sys/prctl.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <exception>
#include <new>
#include <set>
#include <string>
#include <vector>

#ifndef SYS_SECCOMP
#define SYS_SECCOMP 1
#endif
#define syscall_nr (offsetof(struct seccomp_data, nr))
#define arch_nr (offsetof(struct seccomp_data, arch))
#if defined(__x86_64__)
#define REG_SYSCALL REG_RAX
#define ARCH_NR AUDIT_ARCH_X86_64
#else
#error Syscall sandbox is an experimental feature currently available only under Linux x86-64.
#endif

#if !defined(USE_SYSCALL_SANDBOX_MODE_KILL_WITHOUT_DEBUG)
namespace {
// The syscall sandbox feature is currently a Linux x86_64-only feature.
std::string GetLinuxSyscallName(const uint32_t syscall_number)
{
    // Linux x86_64 syscalls listed in syscall number order without gaps.
    static const std::vector<std::string> SYSCALL_NAMES{"read", "write", "open", "close", "stat", "fstat", "lstat", "poll", "lseek", "mmap", "mprotect", "munmap", "brk", "rt_sigaction", "rt_sigprocmask", "rt_sigreturn", "ioctl", "pread64", "pwrite64", "readv", "writev", "access", "pipe", "select", "sched_yield", "mremap", "msync", "mincore", "madvise", "shmget", "shmat", "shmctl", "dup", "dup2", "pause", "nanosleep", "getitimer", "alarm", "setitimer", "getpid", "sendfile", "socket", "connect", "accept", "sendto", "recvfrom", "sendmsg", "recvmsg", "shutdown", "bind", "listen", "getsockname", "getpeername", "socketpair", "setsockopt", "getsockopt", "clone", "fork", "vfork", "execve", "exit", "wait4", "kill", "uname", "semget", "semop", "semctl", "shmdt", "msgget", "msgsnd", "msgrcv", "msgctl", "fcntl", "flock", "fsync", "fdatasync", "truncate", "ftruncate", "getdents", "getcwd", "chdir", "fchdir", "rename", "mkdir", "rmdir", "creat", "link", "unlink", "symlink", "readlink", "chmod", "fchmod", "chown", "fchown", "lchown", "umask", "gettimeofday", "getrlimit", "getrusage", "sysinfo", "times", "ptrace", "getuid", "syslog", "getgid", "setuid", "setgid", "geteuid", "getegid", "setpgid", "getppid", "getpgrp", "setsid", "setreuid", "setregid", "getgroups", "setgroups", "setresuid", "getresuid", "setresgid", "getresgid", "getpgid", "setfsuid", "setfsgid", "getsid", "capget", "capset", "rt_sigpending", "rt_sigtimedwait", "rt_sigqueueinfo", "rt_sigsuspend", "sigaltstack", "utime", "mknod", "uselib", "personality", "ustat", "statfs", "fstatfs", "sysfs", "getpriority", "setpriority", "sched_setparam", "sched_getparam", "sched_setscheduler", "sched_getscheduler", "sched_get_priority_max", "sched_get_priority_min", "sched_rr_get_interval", "mlock", "munlock", "mlockall", "munlockall", "vhangup", "modify_ldt", "pivot_root", "_sysctl", "prctl", "arch_prctl", "adjtimex", "setrlimit", "chroot", "sync", "acct", "settimeofday", "mount", "umount2", "swapon", "swapoff", "reboot", "sethostname", "setdomainname", "iopl", "ioperm", "create_module", "init_module", "delete_module", "get_kernel_syms", "query_module", "quotactl", "nfsservctl", "getpmsg", "putpmsg", "afs_syscall", "tuxcall", "security", "gettid", "readahead", "setxattr", "lsetxattr", "fsetxattr", "getxattr", "lgetxattr", "fgetxattr", "listxattr", "llistxattr", "flistxattr", "removexattr", "lremovexattr", "fremovexattr", "tkill", "time", "futex", "sched_setaffinity", "sched_getaffinity", "set_thread_area", "io_setup", "io_destroy", "io_getevents", "io_submit", "io_cancel", "get_thread_area", "lookup_dcookie", "epoll_create", "epoll_ctl_old", "epoll_wait_old", "remap_file_pages", "getdents64", "set_tid_address", "restart_syscall", "semtimedop", "fadvise64", "timer_create", "timer_settime", "timer_gettime", "timer_getoverrun", "timer_delete", "clock_settime", "clock_gettime", "clock_getres", "clock_nanosleep", "exit_group", "epoll_wait", "epoll_ctl", "tgkill", "utimes", "vserver", "mbind", "set_mempolicy", "get_mempolicy", "mq_open", "mq_unlink", "mq_timedsend", "mq_timedreceive", "mq_notify", "mq_getsetattr", "kexec_load", "waitid", "add_key", "request_key", "keyctl", "ioprio_set", "ioprio_get", "inotify_init", "inotify_add_watch", "inotify_rm_watch", "migrate_pages", "openat", "mkdirat", "mknodat", "fchownat", "futimesat", "newfstatat", "unlinkat", "renameat", "linkat", "symlinkat", "readlinkat", "fchmodat", "faccessat", "pselect6", "ppoll", "unshare", "set_robust_list", "get_robust_list", "splice", "tee", "sync_file_range", "vmsplice", "move_pages", "utimensat", "epoll_pwait", "signalfd", "timerfd_create", "eventfd", "fallocate", "timerfd_settime", "timerfd_gettime", "accept4", "signalfd4", "eventfd2", "epoll_create1", "dup3", "pipe2", "inotify_init1", "preadv", "pwritev", "rt_tgsigqueueinfo", "perf_event_open", "recvmmsg", "fanotify_init", "fanotify_mark", "prlimit64", "name_to_handle_at", "open_by_handle_at", "clock_adjtime", "syncfs", "sendmmsg", "setns", "getcpu", "process_vm_readv", "process_vm_writev", "kcmp", "finit_module", "sched_setattr", "sched_getattr", "renameat2", "seccomp", "getrandom", "memfd_create", "kexec_file_load", "bpf", "execveat", "userfaultfd", "membarrier", "mlock2", "copy_file_range", "preadv2", "pwritev2", "pkey_mprotect", "pkey_alloc", "pkey_free", "statx"};
    assert(SYSCALL_NAMES.size() == 333 && SYSCALL_NAMES[0] == "read" && SYSCALL_NAMES[332] == "statx" && "Syscalls must be listed in syscall number order without gaps.");
    return syscall_number < SYSCALL_NAMES.size() ? SYSCALL_NAMES[syscall_number] : "*unknown*";
}

void SyscallSandboxDebugSignalHandler(int, siginfo_t* info, void* void_context)
{
    if (info->si_code != SYS_SECCOMP) {
        return;
    }
    const ucontext_t* ctx = static_cast<ucontext_t*>(void_context);
    if (ctx == nullptr) {
        return;
    }
    std::set_new_handler(std::terminate);
    const unsigned int syscall_number = ctx->uc_mcontext.gregs[REG_SYSCALL];
    const std::string syscall_name = GetLinuxSyscallName(syscall_number);
    const std::string thread_name = !util::ThreadGetInternalName().empty() ? util::ThreadGetInternalName() : "*unnamed*";
    const std::string error_message = strprintf("ERROR: The syscall \"%s\" (syscall number %d) is not allowed by the syscall sandbox in thread \"%s\". Please report. Exiting.", syscall_name, syscall_number, thread_name);
    tfm::format(std::cerr, "%s\n", error_message);
    LogPrintf("%s\n", error_message);
    std::terminate();
}

void InstallSyscallSandboxDebugHandler()
{
    static std::atomic<bool> syscall_reporter_installed{false};
    if (syscall_reporter_installed.exchange(true)) {
        return;
    }
    LogPrint(BCLog::UTIL, "Installing syscall sandbox debug handler\n");
    struct sigaction action = {};
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGSYS);
    action.sa_sigaction = &SyscallSandboxDebugSignalHandler;
    action.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSYS, &action, nullptr) < 0) {
        perror("sigaction");
        std::terminate();
    }
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr)) {
        perror("sigprocmask");
        std::terminate();
    }
}
} // namespace
#endif

namespace {
enum class SyscallSandboxDefaultAction {
    DEBUG_SIGNAL_HANDLER,
    KILL_PROCESS,
};

class SeccompPolicyBuilder
{
    std::set<uint32_t> allowed_syscalls;

public:
    SeccompPolicyBuilder()
    {
        // Allowed by default.
        AllowAddressSpaceAccess();
        AllowEpoll();
        AllowEventFd();
        AllowFutex();
        AllowGeneralIo();
        AllowGetRandom();
        AllowGetSimpleId();
        AllowGetTime();
        AllowGlobalProcessEnvironment();
        AllowGlobalSystemStatus();
        AllowKernelInternalApi();
        AllowNetworkSocketInformation();
        AllowOperationOnExistingFileDescriptor();
        AllowPipe();
        AllowPrctl();
        AllowProcessStartOrDeath();
        AllowScheduling();
        AllowSignalHandling();
        AllowSleep();
        AllowUmask();
    }

    void AllowAddressSpaceAccess()
    {
        allowed_syscalls.insert(__NR_brk);     // change data segment size
        allowed_syscalls.insert(__NR_madvise); // give advice about use of memory
#if defined(__NR_membarrier)
        allowed_syscalls.insert(__NR_membarrier); // issue memory barriers on a set of threads
#endif
        allowed_syscalls.insert(__NR_mlock);    // lock memory
        allowed_syscalls.insert(__NR_mmap);     // map files or devices into memory
        allowed_syscalls.insert(__NR_mprotect); // set protection on a region of memory
        allowed_syscalls.insert(__NR_munlock);  // unlock memory
        allowed_syscalls.insert(__NR_munmap);   // unmap files or devices into memory
    }

    void AllowEpoll()
    {
        allowed_syscalls.insert(__NR_epoll_create1); // open an epoll file descriptor
        allowed_syscalls.insert(__NR_epoll_ctl);     // control interface for an epoll file descriptor
        allowed_syscalls.insert(__NR_epoll_pwait);   // wait for an I/O event on an epoll file descriptor
        allowed_syscalls.insert(__NR_epoll_wait);    // wait for an I/O event on an epoll file descriptor
    }

    void AllowEventFd()
    {
        allowed_syscalls.insert(__NR_eventfd2); // create a file descriptor for event notification
    }

    void AllowFileSystem()
    {
        allowed_syscalls.insert(__NR_access);     // check user's permissions for a file
        allowed_syscalls.insert(__NR_chdir);      // change working directory
        allowed_syscalls.insert(__NR_chmod);      // change permissions of a file
        allowed_syscalls.insert(__NR_fallocate);  // manipulate file space
        allowed_syscalls.insert(__NR_fchmod);     // change permissions of a file
        allowed_syscalls.insert(__NR_fchown);     // change ownership of a file
        allowed_syscalls.insert(__NR_fdatasync);  // synchronize a file's in-core state with storage device
        allowed_syscalls.insert(__NR_flock);      // apply or remove an advisory lock on an open file
        allowed_syscalls.insert(__NR_fstat);      // get file status
        allowed_syscalls.insert(__NR_fsync);      // synchronize a file's in-core state with storage device
        allowed_syscalls.insert(__NR_ftruncate);  // truncate a file to a specified length
        allowed_syscalls.insert(__NR_getcwd);     // get current working directory
        allowed_syscalls.insert(__NR_getdents);   // get directory entries
        allowed_syscalls.insert(__NR_getdents64); // get directory entries
        allowed_syscalls.insert(__NR_lstat);      // get file status
        allowed_syscalls.insert(__NR_mkdir);      // create a directory
        allowed_syscalls.insert(__NR_open);       // open and possibly create a file
        allowed_syscalls.insert(__NR_openat);     // open and possibly create a file
        allowed_syscalls.insert(__NR_readlink);   // read value of a symbolic link
        allowed_syscalls.insert(__NR_rename);     // change the name or location of a file
        allowed_syscalls.insert(__NR_rmdir);      // delete a directory
        allowed_syscalls.insert(__NR_stat);       // get file status
        allowed_syscalls.insert(__NR_statfs);     // get filesystem statistics
        allowed_syscalls.insert(__NR_statx);      // get file status (extended)
        allowed_syscalls.insert(__NR_unlink);     // delete a name and possibly the file it refers to
    }

    void AllowFutex()
    {
        allowed_syscalls.insert(__NR_futex);           // fast user-space locking
        allowed_syscalls.insert(__NR_set_robust_list); // set list of robust futexes
    }

    void AllowGeneralIo()
    {
        allowed_syscalls.insert(__NR_ioctl);    // control device
        allowed_syscalls.insert(__NR_lseek);    // reposition read/write file offset
        allowed_syscalls.insert(__NR_poll);     // wait for some event on a file descriptor
        allowed_syscalls.insert(__NR_ppoll);    // wait for some event on a file descriptor
        allowed_syscalls.insert(__NR_pread64);  // read from a file descriptor at a given offset
        allowed_syscalls.insert(__NR_pwrite64); // write to a file descriptor at a given offset
        allowed_syscalls.insert(__NR_read);     // read from a file descriptor
        allowed_syscalls.insert(__NR_readv);    // read data into multiple buffers
        allowed_syscalls.insert(__NR_recvfrom); // receive a message from a socket
        allowed_syscalls.insert(__NR_recvmsg);  // receive a message from a socket
        allowed_syscalls.insert(__NR_select);   // synchronous I/O multiplexing
        allowed_syscalls.insert(__NR_sendmmsg); // send multiple messages on a socket
        allowed_syscalls.insert(__NR_sendmsg);  // send a message on a socket
        allowed_syscalls.insert(__NR_sendto);   // send a message on a socket
        allowed_syscalls.insert(__NR_write);    // write to a file descriptor
        allowed_syscalls.insert(__NR_writev);   // write data into multiple buffers
    }

    void AllowGetRandom()
    {
#if defined(__NR_getrandom)
        allowed_syscalls.insert(__NR_getrandom); // obtain a series of random bytes
#endif
    }

    void AllowGetSimpleId()
    {
        allowed_syscalls.insert(__NR_getegid);   // get group identity
        allowed_syscalls.insert(__NR_geteuid);   // get user identity
        allowed_syscalls.insert(__NR_getgid);    // get group identity
        allowed_syscalls.insert(__NR_getpgid);   // get process group
        allowed_syscalls.insert(__NR_getpid);    // get process identification
        allowed_syscalls.insert(__NR_getppid);   // get process identification
        allowed_syscalls.insert(__NR_getresgid); // get real, effective and saved group IDs
        allowed_syscalls.insert(__NR_getresuid); // get real, effective and saved user IDs
        allowed_syscalls.insert(__NR_getsid);    // get session ID
        allowed_syscalls.insert(__NR_gettid);    // get thread identification
        allowed_syscalls.insert(__NR_getuid);    // get user identity
    }

    void AllowGetTime()
    {
        allowed_syscalls.insert(__NR_clock_getres);  // find the resolution (precision) of the specified clock
        allowed_syscalls.insert(__NR_clock_gettime); // retrieve the time of the specified clock
    }

    void AllowGlobalProcessEnvironment()
    {
        allowed_syscalls.insert(__NR_getrlimit); // get resource limits
        allowed_syscalls.insert(__NR_getrusage); // get resource usage
        allowed_syscalls.insert(__NR_prlimit64); // get/set resource limits
    }

    void AllowGlobalSystemStatus()
    {
        allowed_syscalls.insert(__NR_sysinfo); // return system information
        allowed_syscalls.insert(__NR_uname);   // get name and information about current kernel
    }

    void AllowKernelInternalApi()
    {
        allowed_syscalls.insert(__NR_restart_syscall); // restart a system call after interruption by a stop signal
    }

    void AllowNetwork()
    {
        allowed_syscalls.insert(__NR_accept);     // accept a connection on a socket
        allowed_syscalls.insert(__NR_accept4);    // accept a connection on a socket
        allowed_syscalls.insert(__NR_bind);       // bind a name to a socket
        allowed_syscalls.insert(__NR_connect);    // initiate a connection on a socket
        allowed_syscalls.insert(__NR_listen);     // listen for connections on a socket
        allowed_syscalls.insert(__NR_setsockopt); // set options on sockets
        allowed_syscalls.insert(__NR_socket);     // create an endpoint for communication
        allowed_syscalls.insert(__NR_socketpair); // create a pair of connected sockets
    }

    void AllowNetworkSocketInformation()
    {
        allowed_syscalls.insert(__NR_getpeername); // get name of connected peer socket
        allowed_syscalls.insert(__NR_getsockname); // get socket name
        allowed_syscalls.insert(__NR_getsockopt);  // get options on sockets
    }

    void AllowOperationOnExistingFileDescriptor()
    {
        allowed_syscalls.insert(__NR_close);    // close a file descriptor
        allowed_syscalls.insert(__NR_dup);      // duplicate a file descriptor
        allowed_syscalls.insert(__NR_dup2);     // duplicate a file descriptor
        allowed_syscalls.insert(__NR_fcntl);    // manipulate file descriptor
        allowed_syscalls.insert(__NR_shutdown); // shut down part of a full-duplex connection
    }

    void AllowPipe()
    {
        allowed_syscalls.insert(__NR_pipe);  // create pipe
        allowed_syscalls.insert(__NR_pipe2); // create pipe
    }

    void AllowPrctl()
    {
        allowed_syscalls.insert(__NR_arch_prctl); // set architecture-specific thread state
        allowed_syscalls.insert(__NR_prctl);      // operations on a process
    }

    void AllowProcessStartOrDeath()
    {
        allowed_syscalls.insert(__NR_clone);      // create a child process
        allowed_syscalls.insert(__NR_exit);       // terminate the calling process
        allowed_syscalls.insert(__NR_exit_group); // exit all threads in a process
        allowed_syscalls.insert(__NR_fork);       // create a child process
        allowed_syscalls.insert(__NR_tgkill);     // send a signal to a thread
        allowed_syscalls.insert(__NR_wait4);      // wait for process to change state, BSD style
    }

    void AllowScheduling()
    {
        allowed_syscalls.insert(__NR_sched_getaffinity);  // set a thread's CPU affinity mask
        allowed_syscalls.insert(__NR_sched_getparam);     // get scheduling parameters
        allowed_syscalls.insert(__NR_sched_getscheduler); // get scheduling policy/parameters
        allowed_syscalls.insert(__NR_sched_setscheduler); // set scheduling policy/parameters
        allowed_syscalls.insert(__NR_sched_yield);        // yield the processor
    }

    void AllowSignalHandling()
    {
        allowed_syscalls.insert(__NR_rt_sigaction);   // examine and change a signal action
        allowed_syscalls.insert(__NR_rt_sigprocmask); // examine and change blocked signals
        allowed_syscalls.insert(__NR_rt_sigreturn);   // return from signal handler and cleanup stack frame
        allowed_syscalls.insert(__NR_sigaltstack);    // set and/or get signal stack context
    }

    void AllowSleep()
    {
        allowed_syscalls.insert(__NR_clock_nanosleep); // high-resolution sleep with specifiable clock
        allowed_syscalls.insert(__NR_nanosleep);       // high-resolution sleep
    }

    void AllowUmask()
    {
        allowed_syscalls.insert(__NR_umask); // set file mode creation mask
    }

    std::vector<sock_filter> BuildFilter(const SyscallSandboxDefaultAction default_action)
    {
        std::vector<sock_filter> bpf_policy;
        // See https://outflux.net/teach-seccomp/step-2/seccomp-bpf.h for background on
        // the following seccomp-bpf instructions.
        //
        // See VALIDATE_ARCHITECTURE in seccomp-bpf.h referenced above.
        bpf_policy.push_back(BPF_STMT(BPF_LD + BPF_W + BPF_ABS, arch_nr));
        bpf_policy.push_back(BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ARCH_NR, 1, 0));
        bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS));
        // See EXAMINE_SYSCALL in seccomp-bpf.h referenced above.
        bpf_policy.push_back(BPF_STMT(BPF_LD + BPF_W + BPF_ABS, syscall_nr));
        for (const uint32_t allowed_syscall : allowed_syscalls) {
            // See ALLOW_SYSCALL in seccomp-bpf.h referenced above.
            bpf_policy.push_back(BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, allowed_syscall, 0, 1));
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_ALLOW));
        }
        switch (default_action) {
        case SyscallSandboxDefaultAction::KILL_PROCESS:
            // See KILL_PROCESS in seccomp-bpf.h referenced above.
            //
            // Note that we're using SECCOMP_RET_KILL_PROCESS (kill the process) instead
            // of SECCOMP_RET_KILL_THREAD (kill the thread). The SECCOMP_RET_KILL_PROCESS
            // action was introduced in Linux 4.14.
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_KILL_PROCESS));
            break;
        case SyscallSandboxDefaultAction::DEBUG_SIGNAL_HANDLER:
            // Disallow syscall and force a SIGSYS to trigger syscall debug reporter.
            bpf_policy.push_back(BPF_STMT(BPF_RET + BPF_K, SECCOMP_RET_TRAP));
            break;
        }
        return bpf_policy;
    }
};

std::atomic<bool> syscall_filter_installation_disabled{false};
} // namespace
#endif

void EnableSyscallSandbox(const SyscallSandboxPolicy syscall_policy)
{
#if defined(USE_SYSCALL_SANDBOX)
    if (syscall_filter_installation_disabled) {
        return;
    }

#if !defined(USE_SYSCALL_SANDBOX_MODE_KILL_WITHOUT_DEBUG)
    InstallSyscallSandboxDebugHandler();
#endif

    SeccompPolicyBuilder seccomp_policy_builder;
    switch (syscall_policy) {
    case SyscallSandboxPolicy::INITIALIZATION: // Thread: main thread (state: init)
        // SyscallSandboxPolicy::INITIALIZATION is the first policy loaded.
        //
        // Subsequently loaded policies can reduce the abilities further, but
        // abilities can never be regained.
        //
        // SyscallSandboxPolicy::INITIALIZATION must thus be a superset of all
        // other policies.
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_DNS_SEED: // Thread: dnsseed
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_LOAD_BLOCKS: // Thread: loadblk
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_TOR_CONTROL: // Thread: torcontrol
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::INITIALIZATION_MAP_PORT: // Thread: mapport
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::MESSAGE_HANDLER: // Thread: msghand
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::NET: // Thread: net
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_ADD_CONNECTION: // Thread: addcon
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_HTTP_SERVER: // Thread: http
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_HTTP_SERVER_WORKER: // Thread: httpworker.<N>
        seccomp_policy_builder.AllowFileSystem();
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::NET_OPEN_CONNECTION: // Thread: opencon
        seccomp_policy_builder.AllowNetwork();
        break;
    case SyscallSandboxPolicy::SCHEDULER: // Thread: scheduler
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::TX_INDEX: // Thread: txindex
        seccomp_policy_builder.AllowFileSystem();
        break;
    case SyscallSandboxPolicy::VALIDATION_SCRIPT_CHECK: // Thread: scriptch.<N>
        break;
    case SyscallSandboxPolicy::SHUTOFF: // Thread: main thread (state: shutoff)
        seccomp_policy_builder.AllowFileSystem();
        break;
    }

#if defined(USE_SYSCALL_SANDBOX_MODE_KILL_WITHOUT_DEBUG)
    const SyscallSandboxDefaultAction default_action = SyscallSandboxDefaultAction::KILL_PROCESS;
#else
    const SyscallSandboxDefaultAction default_action = SyscallSandboxDefaultAction::DEBUG_SIGNAL_HANDLER;
#endif

    std::vector<sock_filter> filter = seccomp_policy_builder.BuildFilter(default_action);
    const sock_fprog prog = {
        .len = static_cast<uint16_t>(filter.size()),
        .filter = filter.data(),
    };
    // Do not allow abilities to be regained after being dropped.
    if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) {
        perror("prctl(PR_SET_NO_NEW_PRIVS)");
        std::terminate();
    }
    // Install seccomp-bpf syscall filter.
    if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) {
        perror("prctl(PR_SET_SECCOMP)");
        std::terminate();
    }

    const std::string thread_name = !util::ThreadGetInternalName().empty() ? util::ThreadGetInternalName() : "*unnamed*";
    LogPrint(BCLog::UTIL, "Syscall filter installed for thread \"%s\"\n", thread_name);
#endif
}

void DisableFurtherSyscallSandboxRestrictions()
{
#if defined(USE_SYSCALL_SANDBOX)
    if (!syscall_filter_installation_disabled.exchange(true)) {
        LogPrint(BCLog::UTIL, "Disabling further syscall sandbox restrictions\n");
    }
#endif
}
