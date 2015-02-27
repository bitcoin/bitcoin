// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "random.h"

#include "crypto/common.h"
#include "crypto/fortuna.h"
#include "crypto/sha512.h"
#include "crypto/aes.h"

#ifdef WIN32
#include "compat.h" // for Windows API
#include <wincrypt.h>
#endif

#include <stdio.h>

#include "serialize.h" // for begin_ptr(vec)
#include "utiltime.h" // for GetTime()

#include <limits>

#ifndef WIN32
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#endif

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

extern char **environ;

namespace
{
/** Get a high resolution time value, with no guarantees for frequency or monotonicity. */
inline uint64_t GetCycleCounter()
{
#if defined(_MSC_VER)
    return __rdtsc();
#elif defined(__i386__)
    uint64_t r;
    __asm__ volatile ("rdtsc" : "=A"(r));
    return r;
#elif defined(__x86_64__) || defined(__amd64__)
    uint64_t r1, r2;
    __asm__ volatile ("rdtsc" : "=a"(r1), "=d"(r2));
    return (r2 << 32) | r1;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
#endif
}


/** Get 32 bytes of system entropy. */
void GetOSEntropy(unsigned char *ent32)
{
#ifdef WIN32
    HCRYPTPROV hProvider;
    int ret = CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    assert(ret);
    ret = CryptGenRandom(hProvider, 32, ent32);
    assert(ret);
    CryptReleaseContext(hProvider, 0);
#else
    int f = open("/dev/urandom", O_RDONLY);
    assert(f != -1);
    int n = read(f, ent32, 32);
    assert(n == 32);
    close(f);
#endif
}

#ifdef WIN32
void AddPerformanceData(CSHA512* acc)
{
    std::vector<unsigned char> vData(250000, 0);
    long ret = 0;
    unsigned long nSize = 0;
    const size_t nMaxSize = 10000000; // Bail out at more than 10MB of performance data
    while (true) {
        nSize = vData.size();
        ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, begin_ptr(vData), &nSize);
        if (ret != ERROR_MORE_DATA || vData.size() >= nMaxSize)
            break;
        vData.resize(std::max((vData.size() * 3) / 2, nMaxSize)); // Grow size of buffer exponentially
    }
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    if (ret == ERROR_SUCCESS) {
        acc.Write(begin_ptr(vData), nSize);
        memset(begin_ptr(vData), 0, nSize);
    } else {
        static bool warned = false; // Warn only once
        if (!warned) {
            LogPrintf("%s: Warning: RegQueryValueExA(HKEY_PERFORMANCE_DATA) failed with code %i\n", __func__, ret);
            warned = true;
        }
    }
}
#else
void AddStatStruct(CSHA512& acc, const struct stat& sb)
{
    acc.Write((const unsigned char*)&sb.st_dev, sizeof(sb.st_dev));
    acc.Write((const unsigned char*)&sb.st_ino, sizeof(sb.st_ino));
    acc.Write((const unsigned char*)&sb.st_mode, sizeof(sb.st_mode));
    acc.Write((const unsigned char*)&sb.st_nlink, sizeof(sb.st_nlink));
    acc.Write((const unsigned char*)&sb.st_uid, sizeof(sb.st_uid));
    acc.Write((const unsigned char*)&sb.st_gid, sizeof(sb.st_gid));
    acc.Write((const unsigned char*)&sb.st_size, sizeof(sb.st_size));
    acc.Write((const unsigned char*)&sb.st_blksize, sizeof(sb.st_blksize));
    acc.Write((const unsigned char*)&sb.st_blocks, sizeof(sb.st_blocks));
    acc.Write((const unsigned char*)&sb.st_atime, sizeof(sb.st_atime));
    acc.Write((const unsigned char*)&sb.st_mtime, sizeof(sb.st_mtime));
    acc.Write((const unsigned char*)&sb.st_ctime, sizeof(sb.st_ctime));
}

void AddSockAddr(CSHA512& acc, const struct sockaddr *addr)
{
    if (addr == NULL) {
        return;
    }
    int family = addr->sa_family;
    switch (family) {
    case AF_INET:
        acc.Write((const unsigned char*)addr, sizeof(sockaddr_in));
        break;
    case AF_INET6:
        acc.Write((const unsigned char*)addr, sizeof(sockaddr_in6));
        break;
    default:
        acc.Write((const unsigned char*)&family, sizeof(family));
    }
}

void AddFileData(CSHA512& acc, const char *path)
{
    struct stat sb;
    int f = open(path, O_RDONLY);
    if (f != -1) {
        unsigned char fbuf[1024];
        int n;
        acc.Write((const unsigned char*)&f, sizeof(f));
        if (fstat(f, &sb) == 0) {
            AddStatStruct(acc, sb);
        }
        do {
            n = read(f, fbuf, sizeof(fbuf));
            if (n > 0) acc.Write(fbuf, n);
            /* not bothering with EINTR handling. */
        } while (n == sizeof(fbuf));
        close(f);
    }
}

void AddPathData(CSHA512& acc, const char *path)
{
    struct stat sb;
    if (stat(path, &sb) == 0) {
        acc.Write((const unsigned char*)path, strlen(path) + 1);
        AddStatStruct(acc, sb);
    }
}
#endif

void AddTimeData(CSHA512& acc)
{
    uint64_t cycles = GetCycleCounter();
    acc.Write((const unsigned char*)&cycles, sizeof(cycles));
#ifndef WIN32
    struct timeval tv;
    struct timezone tz;
    struct timespec tp;
    if (gettimeofday(&tv, &tz) == 0) {
        acc.Write((const unsigned char*)&tv.tv_sec, sizeof(tv.tv_sec));
        acc.Write((const unsigned char*)&tz.tz_minuteswest, sizeof(tz.tz_minuteswest));
        acc.Write((const unsigned char*)&tz.tz_dsttime, sizeof(tz.tz_dsttime));
    }
    if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
        acc.Write((const unsigned char*)&tp.tv_sec, sizeof(tp.tv_sec));
        acc.Write((const unsigned char*)&tp.tv_nsec, sizeof(tp.tv_nsec));
    }
#else
    int64_t nCounter;
    QueryPerformanceCounter((LARGE_INTEGER*)&nCounter);
    acc.Write((const unsigned char*)&nCounter, sizeof(nCounter));
#endif
}

void AddBenchmarkData(CSHA512& acc)
{
    AddTimeData(acc);

    /* Extract 512 bit of state from the accumulator. */
    unsigned char c[64];
    acc.Finalize(c);
    acc.Reset();

    /* First use a xorshift128+ PRNG to quickly expand 128 bits into 16 MiB */
    uint32_t *buf = (uint32_t*)malloc(sizeof(uint32_t) * 4 * 1024 * 1024);
    uint64_t times[1024];
    uint64_t s0 = ReadLE64(c), s1 = ReadLE64(c + 8);
    for (int i = 0; i < 2 * 1024 * 1024; i++) {
        uint64_t x = s0, y = s1;
        s0 = y;
        x ^= x << 23;
        x ^= x >> 17;
        x ^= y ^ (y >> 26);
        s1 = x;
        buf[2 * i] = (x + y) >> 32;
        buf[2 * i + 1] = (x + y);
        if ((i % 2048) == 2047) {
            times[i / 2048] = GetCycleCounter();
        }
    }
    acc.Write((unsigned char*)times, sizeof(times));

    AddTimeData(acc);

    /* Do some AES256 encryption */
    AES256Encrypt enc(c + 20);
    unsigned char data[16] = {0};
    for (int i = 0; i < 16384; i++) {
        enc.Encrypt(data, data);
        if ((i % 1024) == 1023) {
            times[i / 1024] = GetCycleCounter();
        }
    }
    acc.Write((unsigned char*)times, sizeof(uint64_t) * 16);
    acc.Write(data, sizeof(data));

    AddTimeData(acc);

    /* Do a random walk through that 16 MiB, to measure cache effects */
    uint32_t pos = ReadLE32(c + 36) % (4 * 1024 * 1024);
    for (int i = 0; i < 1048576; i++) {
        pos = buf[pos] % (4 * 1024 * 1024);
        if ((i % 1024) == 1023) {
            times[i / 1024] = GetCycleCounter();
        }
    }
    acc.Write((unsigned char*)times, sizeof(times));
    acc.Write((unsigned char*)(&pos), sizeof(pos));

    AddTimeData(acc);

    /* Reintroduce 512 bits of initially extracted state. */
    acc.Write(c, sizeof(c));

    memset(buf, 0, sizeof(uint32_t) * 4 * 1024 * 1024);
    free(buf);
    memset(c, 0, sizeof(c));
}


#if defined(__i386__) || defined(__x86_64__) || defined(__amd64__)
void inline AddCPUIDData(CSHA512& acc, uint32_t leaf, uint32_t subleaf, uint32_t &ax, uint32_t &cx)
{
    acc.Write((unsigned char*)&leaf, sizeof(leaf));
    acc.Write((unsigned char*)&subleaf, sizeof(subleaf));
    uint32_t bx = 0, dx = 0;
    __asm__ volatile("cpuid;" : "=a"(ax), "=b"(bx), "=c"(cx), "=d"(dx) : "a"(leaf), "c"(subleaf));
    acc.Write((unsigned char*)&ax, sizeof(ax));
    acc.Write((unsigned char*)&bx, sizeof(bx));
    acc.Write((unsigned char*)&cx, sizeof(cx));
    acc.Write((unsigned char*)&dx, sizeof(dx));
}

void AddCPUID(CSHA512& acc)
{
    uint32_t ax, cx;
    // Iterate basic leafs.
    AddCPUIDData(acc, 0, 0, ax, cx);
    uint32_t max = ax;
    for (uint32_t leaf = 1; leaf <= max; leaf++) {
        // Iterate subleafs of leaf 4, 11, and 13.
        if (leaf == 4 || leaf == 11 || leaf == 13) {
            for (uint32_t subleaf = 0; ; subleaf++) {
                AddCPUIDData(acc, leaf, subleaf, ax, cx);
                if ((leaf == 4 || leaf == 13) && ax == 0) break;
                if (leaf == 11 && (cx & 0xFF00) == 0) break;
            }
        } else {
            AddCPUIDData(acc, leaf, 0, ax, cx);
        }
    }
    // Iterate extended leafs.
    AddCPUIDData(acc, 0x80000000, 0, ax, cx);
    uint32_t max_ext = ax;
    for (uint32_t leaf = 0x80000001UL; leaf <= max_ext; leaf++) {
        AddCPUIDData(acc, leaf, 0, ax, cx);
    }
}
#else
void AddCPUID(CSHA512& acc) {}
#endif

void GetEnvironmentEntropy(unsigned char *ent64, bool slow)
{
    CSHA512 acc;

    /* Current time */
    AddTimeData(acc);

    /* Basic compile-time static properties. */
    uint32_t x;
    x = ((CHAR_MIN < 0) << 30) + (sizeof(void *) << 16) + (sizeof(long) << 8) + sizeof(int);
    acc.Write((const unsigned char*)&x, sizeof(x));
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
    x = (__GNUC_PATCHLEVEL__ << 16) + (__GNUC_MINOR__ << 8) + __GNUC__;
    acc.Write((const unsigned char*)&x, sizeof(x));
#endif
#if defined(_MSC_VER)
    x = _MSC_VER;
    acc.Write((const unsigned char*)&x, sizeof(x));
#endif
#if defined(__STDC_VERSION__)
    x = __STDC_VERSION__;
    acc.Write((const unsigned char*)&x, sizeof(x));
#endif
#if defined(_XOPEN_VERSION)
    x = _XOPEN_VERSION;
    acc.Write((const unsigned char*)&x, sizeof(x));
#endif

    /* On i386 and x86_64: CPUID data */
    AddCPUID(acc);

#ifdef WIN32
    /* On Windows: performance data */
    if (slow) {
        AddPerformanceData(acc);
    }
#else

    /* UNIX kernel information */
    struct utsname name;
    if (uname(&name) != -1) {
        acc.Write((const unsigned char*)&name.sysname, strlen(name.sysname) + 1);
        acc.Write((const unsigned char*)&name.nodename, strlen(name.nodename) + 1);
        acc.Write((const unsigned char*)&name.release, strlen(name.release) + 1);
        acc.Write((const unsigned char*)&name.version, strlen(name.version) + 1);
        acc.Write((const unsigned char*)&name.machine, strlen(name.machine) + 1);
    }

    /* Path and filesystem provided data */
    AddPathData(acc, "/");
    AddPathData(acc, ".");
    AddPathData(acc, "/tmp");
    AddPathData(acc, "/home");
    AddPathData(acc, "/proc");
    AddFileData(acc, "/proc/cpuinfo");
    AddFileData(acc, "/proc/meminfo");
    AddFileData(acc, "/proc/softirqs");
    AddFileData(acc, "/proc/zoneinfo");
    AddFileData(acc, "/proc/stat");
    AddFileData(acc, "/proc/version");
    AddFileData(acc, "/proc/self/status");
    AddFileData(acc, "/etc/passwd");
    AddFileData(acc, "/etc/group");
    AddFileData(acc, "/etc/hosts");
    AddFileData(acc, "/etc/resolv.conf");
    AddFileData(acc, "/etc/timezone");
    AddFileData(acc, "/etc/localtime");
    AddFileData(acc, "/etc/hostconfig");

    /* TODO: sysctl's for OSX to fetch information not available from /proc */

    /* Process and user ids */
    pid_t pid;
    uid_t uid;
    gid_t gid;
    pid = getpid();
    acc.Write((const unsigned char*)&pid, sizeof(pid));
    pid = getppid();
    acc.Write((const unsigned char*)&pid, sizeof(pid));
    pid = getsid(0);
    acc.Write((const unsigned char*)&pid, sizeof(pid));
    pid = getpgid(0);
    acc.Write((const unsigned char*)&pid, sizeof(pid));
    uid = getuid();
    acc.Write((const unsigned char*)&uid, sizeof(uid));
    uid = geteuid();
    acc.Write((const unsigned char*)&uid, sizeof(uid));
    gid = getgid();
    acc.Write((const unsigned char*)&gid, sizeof(gid));
    gid = getegid();
    acc.Write((const unsigned char*)&gid, sizeof(gid));

    /* Resource usage. */
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        acc.Write((const unsigned char*)&usage.ru_utime.tv_sec, sizeof(usage.ru_utime.tv_sec));
        acc.Write((const unsigned char*)&usage.ru_utime.tv_usec, sizeof(usage.ru_utime.tv_usec));
        acc.Write((const unsigned char*)&usage.ru_stime.tv_sec, sizeof(usage.ru_stime.tv_sec));
        acc.Write((const unsigned char*)&usage.ru_stime.tv_usec, sizeof(usage.ru_stime.tv_usec));
        acc.Write((const unsigned char*)&usage.ru_maxrss, sizeof(usage.ru_maxrss));
        acc.Write((const unsigned char*)&usage.ru_minflt, sizeof(usage.ru_minflt));
        acc.Write((const unsigned char*)&usage.ru_majflt, sizeof(usage.ru_majflt));
        acc.Write((const unsigned char*)&usage.ru_inblock, sizeof(usage.ru_inblock));
        acc.Write((const unsigned char*)&usage.ru_oublock, sizeof(usage.ru_oublock));
        acc.Write((const unsigned char*)&usage.ru_nvcsw, sizeof(usage.ru_nvcsw));
        acc.Write((const unsigned char*)&usage.ru_nivcsw, sizeof(usage.ru_nivcsw));
    }

    /* Network interfaces */
    struct ifaddrs *ifad = NULL;
    getifaddrs(&ifad);
    struct ifaddrs *ifit = ifad;
    while (ifit != NULL) {
        acc.Write((const unsigned char*)(&ifit), sizeof(ifit));
        acc.Write((const unsigned char*)ifit->ifa_name, strlen(ifit->ifa_name) + 1);
        acc.Write((const unsigned char*)&ifit->ifa_flags, sizeof(ifit->ifa_flags));
        AddSockAddr(acc, ifit->ifa_addr);
        AddSockAddr(acc, ifit->ifa_netmask);
        AddSockAddr(acc, ifit->ifa_dstaddr);
        ifit = ifit->ifa_next;
    }
    freeifaddrs(ifad);
#endif

    /* Hostname */
    char hname[256];
    if (gethostname(hname, 256) == 0) {
        acc.Write((const unsigned char*)hname, strnlen(hname, 256));
    }

    /* Memory locations */
    void *addr;
    addr = (void*)&GetEnvironmentEntropy;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)&printf;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)&malloc;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)&errno;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)&environ;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)&addr;
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    addr = (void*)malloc(4097);
    acc.Write((const unsigned char*)&addr, sizeof(addr));
    free(addr);

    /* Environment variables. */
    if (environ) {
        for (x = 0; environ[x]; x++) {
            acc.Write((const unsigned char*)environ[x], strlen(environ[x]) + 1);
        }
    }

    /* Current time */
    AddTimeData(acc);

    /* OS provided entropy. */
    unsigned char osent[32];
    GetOSEntropy(osent);
    acc.Write(osent, sizeof(osent));

    /* Benchmarks */
    AddBenchmarkData(acc);

    if (slow) {
        /* Strengthen for 0.5 seconds. */
        int64_t start = GetTimeMicros();
        int count = 0;
        do {
            for (int j = 0; j < 1024; j++) {
                uint64_t cycles = GetCycleCounter();
                acc.Finalize(ent64);
                acc.Reset();
                acc.Write((unsigned char*)&cycles, sizeof(cycles));
                acc.Write(osent, sizeof(osent));
                acc.Write((unsigned char*)&j, sizeof(j));
                acc.Write((unsigned char*)&count, sizeof(count));
                acc.Write(ent64, 64);
            }
            AddTimeData(acc);
            count++;
        } while(GetTimeMicros() < start + 500000);
    }

    /* Output */
    acc.Finalize(ent64);
}

boost::mutex cs_prng;
Fortuna prng;

boost::mutex cs_seedtime;
int64_t seedtime = 0;
}

void RandEvent(RandEventSource source, const unsigned char* data, size_t size)
{
    uint64_t now = GetCycleCounter();

    unsigned char buf[32] = {};
    if (size + sizeof(now) >= 32) {
        CSHA256().Write((const unsigned char*)&now, sizeof(now)).Write(data, size).Finalize(buf);
        boost::unique_lock<boost::mutex> lock(cs_prng);
        prng.AddEvent(source, buf, sizeof(buf));
    } else {
        memcpy(buf, &now, sizeof(now));
        memcpy(buf + sizeof(now), data, size);
        boost::unique_lock<boost::mutex> lock(cs_prng);
        prng.AddEvent(source, buf, size + sizeof(now));
    }
    memset(buf, 0, sizeof(buf));
}

void RandSeed(const unsigned char* data, size_t size)
{
    boost::unique_lock<boost::mutex> lock(cs_prng);
    prng.Seed(data, size);
}

void RandSeedSystem(bool slow)
{
    int64_t now = GetTimeMicros();
    {
        boost::unique_lock<boost::mutex> lock(cs_seedtime);
        if (!slow && seedtime + 60 * 10 * 1000000 > now) {
            return;
        }
        seedtime = now;
    }

    unsigned char buf[64];
    GetEnvironmentEntropy(buf, slow);

    boost::unique_lock<boost::mutex> lock(cs_prng);
    prng.Seed(buf, sizeof(buf));
}

void GetRandBytes(unsigned char* buf, int num)
{
    int64_t now = GetTimeMicros();
    boost::unique_lock<boost::mutex> lock(cs_prng);
    prng.Generate(now, buf, num);
}

uint64_t GetRand(uint64_t nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64_t nRange = (std::numeric_limits<uint64_t>::max() / nMax) * nMax;
    uint64_t nRand = 0;
    do {
        GetRandBytes((unsigned char*)&nRand, sizeof(nRand));
    } while (nRand >= nRange);
    return (nRand % nMax);
}

int GetRandInt(int nMax)
{
    return GetRand(nMax);
}

uint256 GetRandHash()
{
    uint256 hash;
    GetRandBytes((unsigned char*)&hash, sizeof(hash));
    return hash;
}

uint32_t insecure_rand_Rz = 11;
uint32_t insecure_rand_Rw = 11;
void seed_insecure_rand(bool fDeterministic)
{
    // The seed values have some unlikely fixed points which we avoid.
    if (fDeterministic) {
        insecure_rand_Rz = insecure_rand_Rw = 11;
    } else {
        uint32_t tmp;
        do {
            GetRandBytes((unsigned char*)&tmp, 4);
        } while (tmp == 0 || tmp == 0x9068ffffU);
        insecure_rand_Rz = tmp;
        do {
            GetRandBytes((unsigned char*)&tmp, 4);
        } while (tmp == 0 || tmp == 0x464fffffU);
        insecure_rand_Rw = tmp;
    }
}
