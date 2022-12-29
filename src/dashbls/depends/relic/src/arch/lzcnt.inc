/*
Count leading zero bits.

David Reid - mackron@gmail.com

The lzcnt32 and lzcnt64 functions count the number of leading zero bits in a given 32- or 64-bit variable. When the input variable is 0, the
total size in bits will be returned (32 for lzcnt32 and 64 for lzcnt64).

For x86/64 platforms, this will use the LZCNT instruction if available. On ARM it will try the CLZ instruction. If these are unavailable it
will fall back to compiler-specific built-ins. If these are unavailable it'll fall back to the generic implementation.


License
=======

Public Domain (www.unlicense.org)
-------------------------------------------------
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>


Functions
---------
lzcnt32_generic
lzcnt64_generic
    Generic implementation.

lzcnt32_msvc_bsr
lzcnt64_msvc_bsr
    MSVC built-in implementation using _BitScanReverse/64(). Note that lzcnt64_mscv_bsr() is only available when compiling as 64-bit.

lzcnt32_gcc_builtin
lzcnt64_gcc_builtin
    GCC/Clang built-in implementation using __builtin_clzl/l(). Note that lzcnt64_gcc_builtin() is only available when compiling as 64-bit.

lzcnt32_msvc_x86
lzcnt64_msvc_x64
    MSVC implementation using the __lzcnt and __lzcnt64 intrinsic. This should emit a LZCNT instruction. Note that these are only available
    when targeting x86/64. lzcnt64_msvc_x64() is only available when compiling as 64-bit.

lzcnt32_gcc_x86
lzcnt64_gcc_x64
    GCC/Clang inline assembly implementation. This will emit the LZCNT instruction. Note that these are only available when targeting x86/x64
    and when compiled using a compiler that supports GCC style inline assembly.

lzcnt32_gcc_arm
lzcnt64_gcc_arm
    GCC/Clang inline assembly implementation. This will emit the CLZ instruction. Note that these are only available when targeting ARM
    architecture version 5 and above and when compiled using a compiler that supports GCC style inline assembly.

lzcnt32_hard
lzcnt64_hard
    High level helper for calling an hardware implementation. This will choose either lzcnt32_msvc_x86()/lzcnt64_msvc_x64() or lzcnt32_gcc_x86()/
    lzcnt64_gcc_x64() depending on the environment. Note that this is only available when targeting x86/64. lzcnt64_hard() is only available
    when compiling as 64-bit. You should only call this if has_lzcnt_hard() returns non-zero.

lzcnt32_soft
lzcnt64_soft
    High level helper for calling the best software implementation available for the current build environment.

lzcnt32
lzcnt64
    High level helper for calling either a hardware or software implementation depending on the build environment. This will always favor a
    hardware implementation. Do not call this in high performance code. The reason for this is that each it will call has_lzcnt_hard() each
    time which may be too fine grained for your purposes. You may be better off calling has_lzcnt_hard() once at a higher level.

has_lzcnt_hard
    Determines whether or not a hardware implementation of lzcnt is available. Use this to know whether or not you can call lzcnt32/64_hard().
    Note that this calls CPUID for each, so you may want to cache the result. Use HAS_LZCNT32/64_HARD to check for compile-time support.
*/
#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(__i386) || defined(_M_IX86)
    #define ARCH_X86
#elif defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X64
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 5) || (defined(_M_ARM) && _M_ARM >= 5) || defined(__ARM_FEATURE_CLZ) /* ARM (Architecture Version 5) */
    #define ARCH_ARM
#endif

#if defined(_WIN64) || defined(_LP64) || defined(__LP64__)
    #define ARCH_64BIT
#else
    #define ARCH_32BIT
#endif

#if defined(ARCH_X86) || defined(ARCH_X64)
    /* x86/64 */
    #if defined(_MSC_VER) && _MSC_VER >= 1500
        #define HAS_LZCNT32_HARD
        #if defined(ARCH_64BIT)
            #define HAS_LZCNT64_HARD
        #endif
    #elif defined(__GNUC__) || defined(__clang__)
        #define HAS_LZCNT32_HARD
        #if defined(ARCH_64BIT)
            #define HAS_LZCNT64_HARD
        #endif
    #endif
#elif defined(ARCH_ARM)
    /* ARM */
    #if defined(__GNUC__) || defined(__clang__)
        #define HAS_LZCNT32_HARD
        #if defined(ARCH_64BIT)
            #define HAS_LZCNT64_HARD
        #endif
    #endif
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1500 && (defined(ARCH_X86) || defined(ARCH_X64)) && !defined(__clang__)
    #define HAS_LZCNT_INTRINSIC
#elif (defined(__GNUC__) && ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7)))
    #define HAS_LZCNT_INTRINSIC
#elif defined(__clang__)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_clzll) || __has_builtin(__builtin_clzl)
            #define HAS_LZCNT_INTRINSIC
        #endif
    #endif
#endif

inline static unsigned int lzcnt32_generic(unsigned int x)
{
    unsigned int n;
    static unsigned int clz_table_4[] = {
        0,
        4,
        3, 3,
        2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1
    };

    if (x == 0) {
        return sizeof(x)*8;
    }

    n = clz_table_4[x >> (sizeof(x)*8 - 4)];
    if (n == 0) {
        if ((x & 0xFFFF0000) == 0) { n  = 16; x <<= 16; }
        if ((x & 0xFF000000) == 0) { n += 8;  x <<= 8;  }
        if ((x & 0xF0000000) == 0) { n += 4;  x <<= 4;  }
        n += clz_table_4[x >> (sizeof(x)*8 - 4)];
    }

    return n - 1;
}

inline static unsigned int lzcnt64_generic(unsigned long long x)
{
    unsigned int n;
    static unsigned int clz_table_4[] = {
        0,
        4,
        3, 3,
        2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1
    };

    if (x == 0) {
        return sizeof(x)*8;
    }

    n = clz_table_4[x >> (sizeof(x)*8 - 4)];
    if (n == 0) {
        if ((x & ((unsigned long long)0xFFFFFFFF << 32)) == 0) { n  = 32; x <<= 32; }
        if ((x & ((unsigned long long)0xFFFF0000 << 32)) == 0) { n += 16; x <<= 16; }
        if ((x & ((unsigned long long)0xFF000000 << 32)) == 0) { n += 8;  x <<= 8;  }
        if ((x & ((unsigned long long)0xF0000000 << 32)) == 0) { n += 4;  x <<= 4;  }
        n += clz_table_4[x >> (sizeof(x)*8 - 4)];
    }

    return n - 1;
}

/* Generic compiler specific intrinsics. */
#if defined(_MSC_VER)
static unsigned int lzcnt32_msvc_bsr(unsigned int x)
{
    unsigned long n;

    if (x == 0) {
        return 32;
    }

    _BitScanReverse(&n, x);

    return 32 - n - 1;
}

/* _BitScanReverse64() is only available on 64-bit builds. */
#if defined(ARCH_64BIT)
static unsigned int lzcnt64_msvc_bsr(unsigned long long x)
{
    unsigned long n;

    if (x == 0) {
        return 64;
    }

    _BitScanReverse64(&n, x);

    return 64 - n - 1;
}
#endif  /* ARCH_64BIT */
#elif (defined(__GNUC__) || defined(__clang__)) && defined(HAS_LZCNT_INTRINSIC)
static unsigned int lzcnt32_gcc_builtin(unsigned int x)
{
    if (x == 0) {
        return 32;
    }

    return (unsigned int)__builtin_clzl((unsigned long)x);
}

static unsigned int lzcnt64_gcc_builtin(unsigned long long x)
{
    if (x == 0) {
        return 64;
    }

    return (unsigned int)__builtin_clzll(x);
}
#endif

static int has_lzcnt_hard()
{
#if defined(ARCH_X86) || defined(ARCH_X64)
    int info[4] = {0};

    #if defined(_MSC_VER)
        __cpuid(info, 0x80000001);
    #elif defined(__GNUC__) || defined(__clang__)
        /*
        It looks like the -fPIC option uses the ebx register which GCC complains about. We can work around this by just using a different register, the
        specific register of which I'm letting the compiler decide on. The "k" prefix is used to specify a 32-bit register. The {...} syntax is for
        supporting different assembly dialects.

        What's basically happening is that we're saving and restoring the ebx register manually.
        */
        #if defined(ARCH_X86) && defined(__PIC__)
            __asm__ __volatile__ (
                "xchg{l} {%%}ebx, %k1;"
                "cpuid;"
                "xchg{l} {%%}ebx, %k1;"
                : "=a"(info[0]), "=&r"(info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(0x80000001), "c"(0)
            );
        #else
            __asm__ __volatile__ (
                "cpuid" : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(0x80000001), "c"(0)
            );
        #endif
    #endif

    return (info[2] & (1 << 5)) != 0;
#elif defined(ARCH_ARM)
    return 1;   /* The CLZ instruction is available starting from ARM architecture version 5. Our ARCH_ARM #define is only defined when targeting version 5 at compile time. */
#else
    return 0;   /* Hardware LZCNT is only supported in x86/64 and ARM for now. */
#endif
}

/* Intrinsics and inline-assembly. x86/64 has a hardware LZCNT instruction. You can only call these if has_lzcnt_hard() returns true. */
#if defined(HAS_LZCNT32_HARD)
    #if defined(ARCH_X86) || defined(ARCH_X64)
        #if defined(_MSC_VER) && !defined(__clang__)
            static unsigned int lzcnt32_msvc_x86(unsigned int x)
            {
                return __lzcnt(x);
            }
        #elif defined(__GNUC__) || defined(__clang__)
            static unsigned int lzcnt32_gcc_x86(unsigned int x)
            {
                /*
                att:   lzcntl [out], [in]
                intel: lzcnt  [in], [out]
                */
                unsigned int r;
                __asm__ __volatile__ (
                    "lzcnt{l %1, %0| %0, %1}" : "=r"(r) : "r"(x)
                );

                return r;
            }
        #endif
    #endif
    #if defined(ARCH_ARM)
        #if defined(__GNUC__) || defined(__clang__)
            static unsigned int lzcnt32_gcc_arm(unsigned int x)
            {
                unsigned int r;
                __asm__ __volatile__ (
                #if defined(ARCH_32BIT)
                    "clz %[out], %[in]" : [out]"=r"(r) : [in]"r"(x)
                #else
                    "clz %w[out], %w[in]" : [out]"=r"(r) : [in]"r"(x)
                #endif
                );

                return r;
            }
        #endif
    #endif

    inline static unsigned int lzcnt32_hard(unsigned int x)
    {
        #if defined(ARCH_X86) || defined(ARCH_X64)
            #if defined(_MSC_VER) && !defined(__clang__)
                return lzcnt32_msvc_x86(x);
            #elif defined(__GNUC__) || defined(__clang__)
                return lzcnt32_gcc_x86(x);
            #else
                #error "This compiler does not support the lzcnt intrinsic."
            #endif
        #elif defined(ARCH_ARM)
            #if defined(__GNUC__) || defined(__clang__)
                return lzcnt32_gcc_arm(x);
            #else
                #error "This compiler does not support the clz intrinsic."
            #endif
        #else
            #error "The build target does not support a native instruction."
        #endif
    }
#endif

#if defined(HAS_LZCNT64_HARD)
    #if defined(ARCH_X86) || defined(ARCH_X64)
        #if defined(_MSC_VER) && !defined(__clang__)
            static unsigned int lzcnt64_msvc_x64(unsigned long long x)
            {
                return (unsigned int)__lzcnt64(x);
            }
        #elif defined(__GNUC__) || defined(__clang__)
            static unsigned int lzcnt64_gcc_x64(unsigned long long x)
            {
                /*
                att:   lzcnt [out], [in]
                intel: lzcnt [in], [out]
                */
                unsigned long long r;
                __asm__ __volatile__ (
                    "lzcnt{ %1, %0| %0, %1}" : "=r"(r) : "r"(x)
                );

                return r;
            }
        #endif
    #endif
    #if defined(ARCH_ARM)
        #if defined(__GNUC__) || defined(__clang__)
            static unsigned int lzcnt64_gcc_arm(unsigned long long x)
            {
                unsigned long long r;
                __asm__ __volatile__ (
                    "clz %[out], %[in]" : [out]"=r"(r) : [in]"r"(x)
                );

                return r;
            }
        #endif
    #endif

    static unsigned int lzcnt64_hard(unsigned long long x)
    {
    #if defined(ARCH_X64)
        #if defined(_MSC_VER) && !defined(__clang__)
            return lzcnt64_msvc_x64(x);
        #elif defined(__GNUC__) || defined(__clang__)
            return lzcnt64_gcc_x64(x);
        #else
            #error "This compiler does not support the lzcnt intrinsic."
        #endif
    #elif defined(ARCH_ARM) && defined(ARCH_64BIT)
        #if defined(__GNUC__) || defined(__clang__)
            return lzcnt64_gcc_arm(x);
        #else
            #error "This compiler does not support the clz intrinsic."
        #endif
    #else
        #error "The build target does not support a native instruction."
    #endif
    }
#endif


inline static unsigned int lzcnt32_soft(unsigned int x)
{
#if defined(_MSC_VER)
    return lzcnt32_msvc_bsr(x);
#elif defined(HAS_LZCNT_INTRINSIC)
    return lzcnt32_gcc_builtin(x);
#else
    return lzcnt32_generic(x);
#endif
}

inline static unsigned int lzcnt64_soft(unsigned long long x)
{
#if defined(ARCH_64BIT)
    #if defined(_MSC_VER)
        return lzcnt64_msvc_bsr(x);
    #elif defined(HAS_LZCNT_INTRINSIC)
        return lzcnt64_gcc_builtin(x);
    #else
        return lzcnt64_generic(x);
    #endif
#else
    return lzcnt64_generic(x);
#endif
}
