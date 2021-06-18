// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

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

extern "C" double exp_old(double x);
extern "C" double log_old(double x);
extern "C" float log2f_old(float x);
extern "C" double pow_old(double base, double power);
#ifdef __i386__
__asm(".symver exp_old,exp@GLIBC_2.1");
__asm(".symver log_old,log@GLIBC_2.1");
__asm(".symver log2f_old,log2f@GLIBC_2.1");
__asm(".symver pow_old,pow@GLIBC_2.1");
#elif defined(__amd64__)
__asm(".symver exp_old,exp@GLIBC_2.2.5");
__asm(".symver log_old,log@GLIBC_2.2.5");
__asm(".symver log2f_old,log2f@GLIBC_2.2.5");
__asm(".symver pow_old,pow@GLIBC_2.2.5");
#elif defined(__arm__)
__asm(".symver exp_old,exp@GLIBC_2.4");
__asm(".symver log_old,log@GLIBC_2.4");
__asm(".symver log2f_old,log2f@GLIBC_2.4");
__asm(".symver pow_old,pow@GLIBC_2.4");
#elif defined(__aarch64__)
__asm(".symver exp_old,exp@GLIBC_2.17");
__asm(".symver log_old,log@GLIBC_2.17");
__asm(".symver log2f_old,log2f@GLIBC_2.17");
__asm(".symver pow_old,pow@GLIBC_2.17");
#elif defined(__powerpc64__)
#  ifdef WORDS_BIGENDIAN
__asm(".symver exp_old,exp@GLIBC_2.3");
__asm(".symver log_old,log@GLIBC_2.3");
__asm(".symver log2f_old,log2f@GLIBC_2.3");
__asm(".symver pow_old,pow@GLIBC_2.3");
#  else
__asm(".symver exp_old,exp@GLIBC_2.17");
__asm(".symver log_old,log@GLIBC_2.17");
__asm(".symver log2f_old,log2f@GLIBC_2.17");
__asm(".symver pow_old,pow@GLIBC_2.17");
#  endif
#elif defined(__riscv)
__asm(".symver exp_old,exp@GLIBC_2.27");
__asm(".symver log_old,log@GLIBC_2.27");
__asm(".symver log2f_old,log2f@GLIBC_2.27");
__asm(".symver pow_old,pow@GLIBC_2.27");
#endif
extern "C" double __wrap_exp(double x)
{
    return exp_old(x);
}
extern "C" double __wrap_log(double x)
{
    return log_old(x);
}
extern "C" float __wrap_log2f(float x)
{
    return log2f_old(x);
}
extern "C" double __wrap_pow(double base, double power)
{
    return pow_old(base, power);
}
