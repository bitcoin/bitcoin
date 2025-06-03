// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <randomenv.h>

#include <clientversion.h>
#include <compat/compat.h>
#include <compat/cpuid.h>
#include <crypto/sha512.h>
#include <span.h>
#include <support/cleanse.h>
#include <util/time.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <climits>
#include <thread>
#include <vector>

#include <sys/types.h> // must go before a number of other headers

#ifdef WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif
#ifdef HAVE_IFADDRS
#include <ifaddrs.h>
#endif
#ifdef HAVE_SYSCTL
#include <sys/sysctl.h>
#if __has_include(<vm/vm_param.h>)
#include <vm/vm_param.h>
#endif
#if __has_include(<sys/resources.h>)
#include <sys/resources.h>
#endif
#if __has_include(<sys/vmmeter.h>)
#include <sys/vmmeter.h>
#endif
#endif
#if defined(HAVE_STRONG_GETAUXVAL)
#include <sys/auxv.h>
#endif

#ifndef _MSC_VER
extern char** environ; // NOLINT(readability-redundant-declaration): Necessary on some platforms
#endif

namespace {

/** Helper to easily feed data into a CSHA512.
 *
 * Note that this does not serialize the passed object (like stream.h's << operators do).
 * Its raw memory representation is used directly.
 */
template<typename T>
CSHA512& operator<<(CSHA512& hasher, const T& data) {
    static_assert(!std::is_same_v<std::decay_t<T>, char*>, "Calling operator<<(CSHA512, char*) is probably not what you want");
    static_assert(!std::is_same_v<std::decay_t<T>, unsigned char*>, "Calling operator<<(CSHA512, unsigned char*) is probably not what you want");
    static_assert(!std::is_same_v<std::decay_t<T>, const char*>, "Calling operator<<(CSHA512, const char*) is probably not what you want");
    static_assert(!std::is_same_v<std::decay_t<T>, const unsigned char*>, "Calling operator<<(CSHA512, const unsigned char*) is probably not what you want");
    hasher.Write((const unsigned char*)&data, sizeof(data));
    return hasher;
}

#ifndef WIN32
void AddSockaddr(CSHA512& hasher, const struct sockaddr *addr)
{
    if (addr == nullptr) return;
    switch (addr->sa_family) {
    case AF_INET:
        hasher.Write((const unsigned char*)addr, sizeof(sockaddr_in));
        break;
    case AF_INET6:
        hasher.Write((const unsigned char*)addr, sizeof(sockaddr_in6));
        break;
    default:
        hasher.Write((const unsigned char*)&addr->sa_family, sizeof(addr->sa_family));
    }
}

void AddFile(CSHA512& hasher, const char *path)
{
    struct stat sb = {};
    int f = open(path, O_RDONLY);
    size_t total = 0;
    if (f != -1) {
        unsigned char fbuf[4096];
        int n;
        hasher.Write((const unsigned char*)&f, sizeof(f));
        if (fstat(f, &sb) == 0) hasher << sb;
        do {
            n = read(f, fbuf, sizeof(fbuf));
            if (n > 0) hasher.Write(fbuf, n);
            total += n;
            /* not bothering with EINTR handling. */
        } while (n == sizeof(fbuf) && total < 1048576); // Read only the first 1 Mbyte
        close(f);
    }
}

void AddPath(CSHA512& hasher, const char *path)
{
    struct stat sb = {};
    if (stat(path, &sb) == 0) {
        hasher.Write((const unsigned char*)path, strlen(path) + 1);
        hasher << sb;
    }
}
#endif

#ifdef HAVE_SYSCTL
template<int... S>
void AddSysctl(CSHA512& hasher)
{
    int CTL[sizeof...(S)] = {S...};
    unsigned char buffer[65536];
    size_t siz = 65536;
    int ret = sysctl(CTL, sizeof...(S), buffer, &siz, nullptr, 0);
    if (ret == 0 || (ret == -1 && errno == ENOMEM)) {
        hasher << sizeof(CTL);
        hasher.Write((const unsigned char*)CTL, sizeof(CTL));
        if (siz > sizeof(buffer)) siz = sizeof(buffer);
        hasher << siz;
        hasher.Write(buffer, siz);
    }
}
#endif

#ifdef HAVE_GETCPUID
void inline AddCPUID(CSHA512& hasher, uint32_t leaf, uint32_t subleaf, uint32_t& ax, uint32_t& bx, uint32_t& cx, uint32_t& dx)
{
    GetCPUID(leaf, subleaf, ax, bx, cx, dx);
    hasher << leaf << subleaf << ax << bx << cx << dx;
}

void AddAllCPUID(CSHA512& hasher)
{
    uint32_t ax, bx, cx, dx;
    // Iterate over all standard leaves
    AddCPUID(hasher, 0, 0, ax, bx, cx, dx); // Returns max leaf in ax
    uint32_t max = ax;
    for (uint32_t leaf = 1; leaf <= max && leaf <= 0xFF; ++leaf) {
        uint32_t maxsub = 0;
        for (uint32_t subleaf = 0; subleaf <= 0xFF; ++subleaf) {
            AddCPUID(hasher, leaf, subleaf, ax, bx, cx, dx);
            // Iterate subleafs for leaf values 4, 7, 11, 13
            if (leaf == 4) {
                if ((ax & 0x1f) == 0) break;
            } else if (leaf == 7) {
                if (subleaf == 0) maxsub = ax;
                if (subleaf == maxsub) break;
            } else if (leaf == 11) {
                if ((cx & 0xff00) == 0) break;
            } else if (leaf == 13) {
                if (ax == 0 && bx == 0 && cx == 0 && dx == 0) break;
            } else {
                // For any other leaf, stop after subleaf 0.
                break;
            }
        }
    }
    // Iterate over all extended leaves
    AddCPUID(hasher, 0x80000000, 0, ax, bx, cx, dx); // Returns max extended leaf in ax
    uint32_t ext_max = ax;
    for (uint32_t leaf = 0x80000001; leaf <= ext_max && leaf <= 0x800000FF; ++leaf) {
        AddCPUID(hasher, leaf, 0, ax, bx, cx, dx);
    }
}
#endif
} // namespace

void RandAddDynamicEnv(CSHA512& hasher)
{
    // Various clocks
#ifdef WIN32
    FILETIME ftime;
    GetSystemTimeAsFileTime(&ftime);
    hasher << ftime;
#else
    struct timespec ts = {};
#    ifdef CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &ts);
    hasher << ts;
#    endif
#    ifdef CLOCK_REALTIME
    clock_gettime(CLOCK_REALTIME, &ts);
    hasher << ts;
#    endif
#    ifdef CLOCK_BOOTTIME
    clock_gettime(CLOCK_BOOTTIME, &ts);
    hasher << ts;
#    endif
    // gettimeofday is available on all UNIX systems, but only has microsecond precision.
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    hasher << tv;
#endif
    // Probably redundant, but also use all the standard library clocks:
    hasher << std::chrono::system_clock::now().time_since_epoch().count();
    hasher << std::chrono::steady_clock::now().time_since_epoch().count();
    hasher << std::chrono::high_resolution_clock::now().time_since_epoch().count();

#ifndef WIN32
    // Current resource usage.
    struct rusage usage = {};
    if (getrusage(RUSAGE_SELF, &usage) == 0) hasher << usage;
#endif

#ifdef __linux__
    AddFile(hasher, "/proc/diskstats");
    AddFile(hasher, "/proc/vmstat");
    AddFile(hasher, "/proc/schedstat");
    AddFile(hasher, "/proc/zoneinfo");
    AddFile(hasher, "/proc/meminfo");
    AddFile(hasher, "/proc/softirqs");
    AddFile(hasher, "/proc/stat");
    AddFile(hasher, "/proc/self/schedstat");
    AddFile(hasher, "/proc/self/status");
#endif

#ifdef HAVE_SYSCTL
#  ifdef CTL_KERN
#    if defined(KERN_PROC) && defined(KERN_PROC_ALL)
    AddSysctl<CTL_KERN, KERN_PROC, KERN_PROC_ALL>(hasher);
#    endif
#  endif
#  ifdef CTL_HW
#    ifdef HW_DISKSTATS
    AddSysctl<CTL_HW, HW_DISKSTATS>(hasher);
#    endif
#  endif
#  ifdef CTL_VM
#    ifdef VM_LOADAVG
    AddSysctl<CTL_VM, VM_LOADAVG>(hasher);
#    endif
#    ifdef VM_TOTAL
    AddSysctl<CTL_VM, VM_TOTAL>(hasher);
#    endif
#    ifdef VM_METER
    AddSysctl<CTL_VM, VM_METER>(hasher);
#    endif
#  endif
#endif

    // Stack and heap location
    void* addr = malloc(4097);
    hasher << &addr << addr;
    free(addr);
}

void RandAddStaticEnv(CSHA512& hasher)
{
    // Some compile-time static properties
    hasher << (CHAR_MIN < 0) << sizeof(void*) << sizeof(long) << sizeof(int);
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
    hasher << __GNUC__ << __GNUC_MINOR__ << __GNUC_PATCHLEVEL__;
#endif
#ifdef _MSC_VER
    hasher << _MSC_VER;
#endif
    hasher << __cplusplus;
#ifdef _XOPEN_VERSION
    hasher << _XOPEN_VERSION;
#endif
#ifdef __VERSION__
    const char* COMPILER_VERSION = __VERSION__;
    hasher.Write((const unsigned char*)COMPILER_VERSION, strlen(COMPILER_VERSION) + 1);
#endif

    // Bitcoin client version
    hasher << CLIENT_VERSION;

#if defined(HAVE_STRONG_GETAUXVAL)
    // Information available through getauxval()
#  ifdef AT_HWCAP
    hasher << getauxval(AT_HWCAP);
#  endif
#  ifdef AT_HWCAP2
    hasher << getauxval(AT_HWCAP2);
#  endif
#  ifdef AT_RANDOM
    const unsigned char* random_aux = (const unsigned char*)getauxval(AT_RANDOM);
    if (random_aux) hasher.Write(random_aux, 16);
#  endif
#  ifdef AT_PLATFORM
    const char* platform_str = (const char*)getauxval(AT_PLATFORM);
    if (platform_str) hasher.Write((const unsigned char*)platform_str, strlen(platform_str) + 1);
#  endif
#  ifdef AT_EXECFN
    const char* exec_str = (const char*)getauxval(AT_EXECFN);
    if (exec_str) hasher.Write((const unsigned char*)exec_str, strlen(exec_str) + 1);
#  endif
#endif // HAVE_STRONG_GETAUXVAL

#ifdef HAVE_GETCPUID
    AddAllCPUID(hasher);
#endif

    // Memory locations
    hasher << &hasher << &RandAddStaticEnv << &malloc << &errno << &environ;

    // Hostname
#ifdef WIN32
    constexpr DWORD max_size = MAX_COMPUTERNAME_LENGTH + 1;
    char hname[max_size];
    DWORD size = max_size;
    if (GetComputerNameA(hname, &size) != 0) {
        hasher.Write(UCharCast(hname), size);
    }
#else
    char hname[256];
    if (gethostname(hname, 256) == 0) {
        hasher.Write((const unsigned char*)hname, strnlen(hname, 256));
    }
#endif

#ifdef HAVE_IFADDRS
    // Network interfaces
    struct ifaddrs *ifad = nullptr;
    getifaddrs(&ifad);
    struct ifaddrs *ifit = ifad;
    while (ifit != nullptr) {
        hasher.Write((const unsigned char*)&ifit, sizeof(ifit));
        hasher.Write((const unsigned char*)ifit->ifa_name, strlen(ifit->ifa_name) + 1);
        hasher.Write((const unsigned char*)&ifit->ifa_flags, sizeof(ifit->ifa_flags));
        AddSockaddr(hasher, ifit->ifa_addr);
        AddSockaddr(hasher, ifit->ifa_netmask);
        AddSockaddr(hasher, ifit->ifa_dstaddr);
        ifit = ifit->ifa_next;
    }
    freeifaddrs(ifad);
#endif

#ifndef WIN32
    // UNIX kernel information
    struct utsname name;
    if (uname(&name) != -1) {
        hasher.Write((const unsigned char*)&name.sysname, strlen(name.sysname) + 1);
        hasher.Write((const unsigned char*)&name.nodename, strlen(name.nodename) + 1);
        hasher.Write((const unsigned char*)&name.release, strlen(name.release) + 1);
        hasher.Write((const unsigned char*)&name.version, strlen(name.version) + 1);
        hasher.Write((const unsigned char*)&name.machine, strlen(name.machine) + 1);
    }

    /* Path and filesystem provided data */
    AddPath(hasher, "/");
    AddPath(hasher, ".");
    AddPath(hasher, "/tmp");
    AddPath(hasher, "/home");
    AddPath(hasher, "/proc");
#ifdef __linux__
    AddFile(hasher, "/proc/cmdline");
    AddFile(hasher, "/proc/cpuinfo");
    AddFile(hasher, "/proc/version");
#endif
    AddFile(hasher, "/etc/passwd");
    AddFile(hasher, "/etc/group");
    AddFile(hasher, "/etc/hosts");
    AddFile(hasher, "/etc/resolv.conf");
    AddFile(hasher, "/etc/timezone");
    AddFile(hasher, "/etc/localtime");
#endif

    // For MacOS/BSDs, gather data through sysctl instead of /proc. Not all of these
    // will exist on every system.
#ifdef HAVE_SYSCTL
#  ifdef CTL_HW
#    ifdef HW_MACHINE
    AddSysctl<CTL_HW, HW_MACHINE>(hasher);
#    endif
#    ifdef HW_MODEL
    AddSysctl<CTL_HW, HW_MODEL>(hasher);
#    endif
#    ifdef HW_NCPU
    AddSysctl<CTL_HW, HW_NCPU>(hasher);
#    endif
#    ifdef HW_PHYSMEM
    AddSysctl<CTL_HW, HW_PHYSMEM>(hasher);
#    endif
#    ifdef HW_USERMEM
    AddSysctl<CTL_HW, HW_USERMEM>(hasher);
#    endif
#    ifdef HW_MACHINE_ARCH
    AddSysctl<CTL_HW, HW_MACHINE_ARCH>(hasher);
#    endif
#    ifdef HW_REALMEM
    AddSysctl<CTL_HW, HW_REALMEM>(hasher);
#    endif
#    ifdef HW_CPU_FREQ
    AddSysctl<CTL_HW, HW_CPU_FREQ>(hasher);
#    endif
#    ifdef HW_BUS_FREQ
    AddSysctl<CTL_HW, HW_BUS_FREQ>(hasher);
#    endif
#    ifdef HW_CACHELINE
    AddSysctl<CTL_HW, HW_CACHELINE>(hasher);
#    endif
#  endif
#  ifdef CTL_KERN
#    ifdef KERN_BOOTFILE
     AddSysctl<CTL_KERN, KERN_BOOTFILE>(hasher);
#    endif
#    ifdef KERN_BOOTTIME
     AddSysctl<CTL_KERN, KERN_BOOTTIME>(hasher);
#    endif
#    ifdef KERN_CLOCKRATE
     AddSysctl<CTL_KERN, KERN_CLOCKRATE>(hasher);
#    endif
#    ifdef KERN_HOSTID
     AddSysctl<CTL_KERN, KERN_HOSTID>(hasher);
#    endif
#    ifdef KERN_HOSTUUID
     AddSysctl<CTL_KERN, KERN_HOSTUUID>(hasher);
#    endif
#    ifdef KERN_HOSTNAME
     AddSysctl<CTL_KERN, KERN_HOSTNAME>(hasher);
#    endif
#    ifdef KERN_OSRELDATE
     AddSysctl<CTL_KERN, KERN_OSRELDATE>(hasher);
#    endif
#    ifdef KERN_OSRELEASE
     AddSysctl<CTL_KERN, KERN_OSRELEASE>(hasher);
#    endif
#    ifdef KERN_OSREV
     AddSysctl<CTL_KERN, KERN_OSREV>(hasher);
#    endif
#    ifdef KERN_OSTYPE
     AddSysctl<CTL_KERN, KERN_OSTYPE>(hasher);
#    endif
#    ifdef KERN_POSIX1
     AddSysctl<CTL_KERN, KERN_OSREV>(hasher);
#    endif
#    ifdef KERN_VERSION
     AddSysctl<CTL_KERN, KERN_VERSION>(hasher);
#    endif
#  endif
#endif

    // Env variables
    if (environ) {
        for (size_t i = 0; environ[i]; ++i) {
            hasher.Write((const unsigned char*)environ[i], strlen(environ[i]));
        }
    }

    // Process, thread, user, session, group, ... ids.
#ifdef WIN32
    hasher << GetCurrentProcessId() << GetCurrentThreadId();
#else
    hasher << getpid() << getppid() << getsid(0) << getpgid(0) << getuid() << geteuid() << getgid() << getegid();
#endif
    hasher << std::this_thread::get_id();
}
