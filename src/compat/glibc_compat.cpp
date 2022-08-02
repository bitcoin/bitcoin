// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <cstdarg>
#include <cstddef>
#include <cstdint>

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

extern "C" float exp_old(float x);
extern "C" float exp2_old(float x);
extern "C" float log_old(float x);
extern "C" float log2_old(float x);
extern "C" float log2f_old(float x);
extern "C" float pow_old(float x, float y);
extern "C" int fcntl_old(int fd, int cmd, ...);

#ifdef __i386__
#define SYMVER "GLIBC_2.1"
#elif defined(__amd64__)
#define SYMVER "GLIBC_2.2.5"
#elif defined(__arm__)
#define SYMVER "GLIBC_2.4"
#elif defined(__aarch64__)
#define SYMVER "GLIBC_2.17"
#elif defined(__riscv)
#define SYMVER "GLIBC_2.27"
#endif // __i386__

#define SYMVER_OLD(FUNC) __asm__(".symver " #FUNC "_old," #FUNC "@" SYMVER)

SYMVER_OLD(exp2);
SYMVER_OLD(log2);
SYMVER_OLD(log2f);

#ifdef __i386__
#undef SYMVER
#undef SYMVER_OLD
#define SYMVER "GLIBC_2.0"
#define SYMVER_OLD(FUNC) __asm__(".symver " #FUNC "_old," #FUNC "@" SYMVER)
#endif // __i386__

SYMVER_OLD(exp);
SYMVER_OLD(log);
SYMVER_OLD(pow);
SYMVER_OLD(fcntl);

extern "C" float __wrap_exp(float x)                { return exp_old(x); }
extern "C" float __wrap_exp2(float x)               { return exp2_old(x); }
extern "C" float __wrap_log(float x)                { return log_old(x); }
extern "C" float __wrap_log2(float x)               { return log2_old(x); }
extern "C" float __wrap_log2f(float x)              { return log2f_old(x); }
extern "C" float __wrap_pow(float x, float y)       { return pow_old(x, y); }

extern "C" int __wrap_fcntl(int fd, int cmd, ...)
{
    va_list va;
    void *arg;

    va_start(va, cmd);
    arg = va_arg(va, void *);
    va_end(va);

    return fcntl_old(fd, cmd, arg);
}
