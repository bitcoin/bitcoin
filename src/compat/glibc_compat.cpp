// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <cstddef>
#include <cstdint>
// SYSCOIN
#define _GNU_SOURCE 1
#include <sys/types.h>
#include <unistd.h>
#ifndef WIN32
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#endif
#ifdef HAVE_SYS_GETRANDOM
#include <sys/syscall.h>
#include <linux/random.h>
#endif
// Prior to GLIBC_2.14, memcpy was aliased to memmove.
extern "C" void* memmove(void* a, const void* b, size_t c);
extern "C" void* memcpy(void* a, const void* b, size_t c)
{
    return memmove(a, b, c);
}

#if defined(__i386__) || defined(__arm__)

extern "C" int64_t __udivmoddi4(uint64_t u, uint64_t v, uint64_t* rp);

extern "C" int64_t __wrap___divmoddi4(int64_t u, int64_t v, int64_t* rp)
{
    int32_t c1 = 0, c2 = 0;
    int64_t uu = u, vv = v;
    int64_t w;
    int64_t r;

    if (uu < 0) {
        c1 = ~c1, c2 = ~c2, uu = -uu;
    }
    if (vv < 0) {
        c1 = ~c1, vv = -vv;
    }

    w = __udivmoddi4(uu, vv, (uint64_t*)&r);
    if (c1)
        w = -w;
    if (c2)
        r = -r;

    *rp = r;
    return w;
}
#endif

extern "C" float log2f_old(float x);
#ifdef __i386__
__asm(".symver log2f_old,log2f@GLIBC_2.1");
#elif defined(__amd64__)
__asm(".symver log2f_old,log2f@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver log2f_old,log2f@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver log2f_old,log2f@GLIBC_2.17");
#elif defined(__riscv)
__asm(".symver log2f_old,log2f@GLIBC_2.27");
#endif
extern "C" float __wrap_log2f(float x)
{
    return log2f_old(x);
}
// SYSCOIN wrapper for getrandom 2.25 glibc compatbility for up to 2.17 backwards
#ifndef WIN32
/** Fallback: get 32 bytes of system entropy from /dev/urandom. The most
 * compatible way to get cryptographic randomness on UNIX-ish platforms.
 */
static ssize_t GetDevURandom(unsigned char *ent32, size_t length)
{
    int f = open("/dev/urandom", O_RDONLY);
    if (f == -1) {
        return -1;
    }
    int have = 0;
    do {
        ssize_t n = read(f, ent32 + have, length - have);
        if (n <= 0 || n + have > length) {
            close(f);
            return n;
        }
        have += n;
    } while (have < length);
    close(f);
    return have;
}
#endif

/* Write LENGTH bytes of randomness starting at BUFFER.  Return 0 on
    success and -1 on failure.  */
extern "C" ssize_t __wrap_getrandom (void *buffer, size_t length, unsigned int flags)
    {
        ssize_t rv = -1;
        #if defined(HAVE_SYS_GETRANDOM)
        /* Linux. From the getrandom(2) man page:
        * "If the urandom source has been initialized, reads of up to 256 bytes
        * will always return as many bytes as requested and will not be
        * interrupted by signals."
        */
        rv = syscall(SYS_getrandom, buffer, length, flags);
        if (rv != length) {
            if (rv < 0 && errno == ENOSYS) {
                /* Fallback for kernel <3.17: the return value will be -1 and errno
                * ENOSYS if the syscall is not available, in that case fall back
                * to /dev/urandom.
                */
                return GetDevURandom((unsigned char*)buffer, length);
            }
        }
        #else 
            return GetDevURandom((unsigned char*)buffer, length);
        #endif
        return rv;
    }