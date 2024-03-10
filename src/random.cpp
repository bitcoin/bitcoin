// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <random.h>

#include <compat/compat.h>
#include <compat/cpuid.h>
#include <crypto/chacha20.h>
#include <crypto/sha256.h>
#include <crypto/sha512.h>
#include <logging.h>
#include <randomenv.h>
#include <span.h>
#include <support/allocators/secure.h>
#include <support/cleanse.h>
#include <sync.h>
#include <util/time.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <thread>

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#else
#include <fcntl.h>
#include <sys/time.h>
#endif

#if defined(HAVE_GETRANDOM) || (defined(HAVE_GETENTROPY_RAND) && defined(MAC_OSX))
#include <sys/random.h>
#endif

#ifdef HAVE_SYSCTL_ARND
#include <sys/sysctl.h>
#endif
#if defined(HAVE_STRONG_GETAUXVAL) && defined(__aarch64__)
#include <sys/auxv.h>
#endif

[[noreturn]] static void RandFailure()
{
    LogPrintf("Failed to read randomness, aborting\n");
    std::abort();
}

static inline int64_t GetPerformanceCounter() noexcept
{
    // Read the hardware time stamp counter when available.
    // See https://en.wikipedia.org/wiki/Time_Stamp_Counter for more information.
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    return __rdtsc();
#elif !defined(_MSC_VER) && defined(__i386__)
    uint64_t r = 0;
    __asm__ volatile ("rdtsc" : "=A"(r)); // Constrain the r variable to the eax:edx pair.
    return r;
#elif !defined(_MSC_VER) && (defined(__x86_64__) || defined(__amd64__))
    uint64_t r1 = 0, r2 = 0;
    __asm__ volatile ("rdtsc" : "=a"(r1), "=d"(r2)); // Constrain r1 to rax and r2 to rdx.
    return (r2 << 32) | r1;
#else
    // Fall back to using standard library clock (usually microsecond or nanosecond precision)
    return std::chrono::high_resolution_clock::now().time_since_epoch().count();
#endif
}

#ifdef HAVE_GETCPUID
static bool g_rdrand_supported = false;
static bool g_rdseed_supported = false;
static constexpr uint32_t CPUID_F1_ECX_RDRAND = 0x40000000;
static constexpr uint32_t CPUID_F7_EBX_RDSEED = 0x00040000;
#ifdef bit_RDRND
static_assert(CPUID_F1_ECX_RDRAND == bit_RDRND, "Unexpected value for bit_RDRND");
#endif
#ifdef bit_RDSEED
static_assert(CPUID_F7_EBX_RDSEED == bit_RDSEED, "Unexpected value for bit_RDSEED");
#endif

static void InitHardwareRand()
{
    uint32_t eax, ebx, ecx, edx;
    GetCPUID(1, 0, eax, ebx, ecx, edx);
    if (ecx & CPUID_F1_ECX_RDRAND) {
        g_rdrand_supported = true;
    }
    GetCPUID(7, 0, eax, ebx, ecx, edx);
    if (ebx & CPUID_F7_EBX_RDSEED) {
        g_rdseed_supported = true;
    }
}

static void ReportHardwareRand()
{
    // This must be done in a separate function, as InitHardwareRand() may be indirectly called
    // from global constructors, before logging is initialized.
    if (g_rdseed_supported) {
        LogPrintf("Using RdSeed as an additional entropy source\n");
    }
    if (g_rdrand_supported) {
        LogPrintf("Using RdRand as an additional entropy source\n");
    }
}

/** Read 64 bits of entropy using rdrand.
 *
 * Must only be called when RdRand is supported.
 */
static uint64_t GetRdRand() noexcept
{
    // RdRand may very rarely fail. Invoke it up to 10 times in a loop to reduce this risk.
#ifdef __i386__
    uint8_t ok;
    // Initialize to 0 to silence a compiler warning that r1 or r2 may be used
    // uninitialized. Even if rdrand fails (!ok) it will set the output to 0,
    // but there is no way that the compiler could know that.
    uint32_t r1 = 0, r2 = 0;
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r1), "=q"(ok) :: "cc"); // rdrand %eax
        if (ok) break;
    }
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r2), "=q"(ok) :: "cc"); // rdrand %eax
        if (ok) break;
    }
    return (((uint64_t)r2) << 32) | r1;
#elif defined(__x86_64__) || defined(__amd64__)
    uint8_t ok;
    uint64_t r1 = 0; // See above why we initialize to 0.
    for (int i = 0; i < 10; ++i) {
        __asm__ volatile (".byte 0x48, 0x0f, 0xc7, 0xf0; setc %1" : "=a"(r1), "=q"(ok) :: "cc"); // rdrand %rax
        if (ok) break;
    }
    return r1;
#else
#error "RdRand is only supported on x86 and x86_64"
#endif
}

/** Read 64 bits of entropy using rdseed.
 *
 * Must only be called when RdSeed is supported.
 */
static uint64_t GetRdSeed() noexcept
{
    // RdSeed may fail when the HW RNG is overloaded. Loop indefinitely until enough entropy is gathered,
    // but pause after every failure.
#ifdef __i386__
    uint8_t ok;
    uint32_t r1, r2;
    do {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r1), "=q"(ok) :: "cc"); // rdseed %eax
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    do {
        __asm__ volatile (".byte 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r2), "=q"(ok) :: "cc"); // rdseed %eax
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    return (((uint64_t)r2) << 32) | r1;
#elif defined(__x86_64__) || defined(__amd64__)
    uint8_t ok;
    uint64_t r1;
    do {
        __asm__ volatile (".byte 0x48, 0x0f, 0xc7, 0xf8; setc %1" : "=a"(r1), "=q"(ok) :: "cc"); // rdseed %rax
        if (ok) break;
        __asm__ volatile ("pause");
    } while(true);
    return r1;
#else
#error "RdSeed is only supported on x86 and x86_64"
#endif
}

#elif defined(__aarch64__) && defined(HWCAP2_RNG)

static bool g_rndr_supported = false;

static void InitHardwareRand()
{
    if (getauxval(AT_HWCAP2) & HWCAP2_RNG) {
        g_rndr_supported = true;
    }
}

static void ReportHardwareRand()
{
    // This must be done in a separate function, as InitHardwareRand() may be indirectly called
    // from global constructors, before logging is initialized.
    if (g_rndr_supported) {
        LogPrintf("Using RNDR and RNDRRS as additional entropy sources\n");
    }
}

/** Read 64 bits of entropy using rndr.
 *
 * Must only be called when RNDR is supported.
 */
static uint64_t GetRNDR() noexcept
{
    uint8_t ok;
    uint64_t r1;
    do {
        // https://developer.arm.com/documentation/ddi0601/2022-12/AArch64-Registers/RNDR--Random-Number
        __asm__ volatile("mrs %0, s3_3_c2_c4_0; cset %w1, ne;"
                         : "=r"(r1), "=r"(ok)::"cc");
        if (ok) break;
        __asm__ volatile("yield");
    } while (true);
    return r1;
}

/** Read 64 bits of entropy using rndrrs.
 *
 * Must only be called when RNDRRS is supported.
 */
static uint64_t GetRNDRRS() noexcept
{
    uint8_t ok;
    uint64_t r1;
    do {
        // https://developer.arm.com/documentation/ddi0601/2022-12/AArch64-Registers/RNDRRS--Reseeded-Random-Number
        __asm__ volatile("mrs %0, s3_3_c2_c4_1; cset %w1, ne;"
                         : "=r"(r1), "=r"(ok)::"cc");
        if (ok) break;
        __asm__ volatile("yield");
    } while (true);
    return r1;
}

#else
/* Access to other hardware random number generators could be added here later,
 * assuming it is sufficiently fast (in the order of a few hundred CPU cycles).
 * Slower sources should probably be invoked separately, and/or only from
 * RandAddPeriodic (which is called once a minute).
 */
static void InitHardwareRand() {}
static void ReportHardwareRand() {}
#endif

/** Add 64 bits of entropy gathered from hardware to hasher. Do nothing if not supported. */
static void SeedHardwareFast(CSHA512& hasher) noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
    if (g_rdrand_supported) {
        uint64_t out = GetRdRand();
        hasher.Write((const unsigned char*)&out, sizeof(out));
        return;
    }
#elif defined(__aarch64__) && defined(HWCAP2_RNG)
    if (g_rndr_supported) {
        uint64_t out = GetRNDR();
        hasher.Write((const unsigned char*)&out, sizeof(out));
        return;
    }
#endif
}

/** Add 256 bits of entropy gathered from hardware to hasher. Do nothing if not supported. */
static void SeedHardwareSlow(CSHA512& hasher) noexcept {
#if defined(__x86_64__) || defined(__amd64__) || defined(__i386__)
    // When we want 256 bits of entropy, prefer RdSeed over RdRand, as it's
    // guaranteed to produce independent randomness on every call.
    if (g_rdseed_supported) {
        for (int i = 0; i < 4; ++i) {
            uint64_t out = GetRdSeed();
            hasher.Write((const unsigned char*)&out, sizeof(out));
        }
        return;
    }
    // When falling back to RdRand, XOR the result of 1024 results.
    // This guarantees a reseeding occurs between each.
    if (g_rdrand_supported) {
        for (int i = 0; i < 4; ++i) {
            uint64_t out = 0;
            for (int j = 0; j < 1024; ++j) out ^= GetRdRand();
            hasher.Write((const unsigned char*)&out, sizeof(out));
        }
        return;
    }
#elif defined(__aarch64__) && defined(HWCAP2_RNG)
    if (g_rndr_supported) {
        for (int i = 0; i < 4; ++i) {
            uint64_t out = GetRNDRRS();
            hasher.Write((const unsigned char*)&out, sizeof(out));
        }
        return;
    }
#endif
}

/** Use repeated SHA512 to strengthen the randomness in seed32, and feed into hasher. */
static void Strengthen(const unsigned char (&seed)[32], SteadyClock::duration dur, CSHA512& hasher) noexcept
{
    CSHA512 inner_hasher;
    inner_hasher.Write(seed, sizeof(seed));

    // Hash loop
    unsigned char buffer[64];
    const auto stop{SteadyClock::now() + dur};
    do {
        for (int i = 0; i < 1000; ++i) {
            inner_hasher.Finalize(buffer);
            inner_hasher.Reset();
            inner_hasher.Write(buffer, sizeof(buffer));
        }
        // Benchmark operation and feed it into outer hasher.
        int64_t perf = GetPerformanceCounter();
        hasher.Write((const unsigned char*)&perf, sizeof(perf));
    } while (SteadyClock::now() < stop);

    // Produce output from inner state and feed it to outer hasher.
    inner_hasher.Finalize(buffer);
    hasher.Write(buffer, sizeof(buffer));
    // Try to clean up.
    inner_hasher.Reset();
    memory_cleanse(buffer, sizeof(buffer));
}

#ifndef WIN32
/** Fallback: get 32 bytes of system entropy from /dev/urandom. The most
 * compatible way to get cryptographic randomness on UNIX-ish platforms.
 */
[[maybe_unused]] static void GetDevURandom(unsigned char *ent32)
{
    int f = open("/dev/urandom", O_RDONLY);
    if (f == -1) {
        RandFailure();
    }
    int have = 0;
    do {
        ssize_t n = read(f, ent32 + have, NUM_OS_RANDOM_BYTES - have);
        if (n <= 0 || n + have > NUM_OS_RANDOM_BYTES) {
            close(f);
            RandFailure();
        }
        have += n;
    } while (have < NUM_OS_RANDOM_BYTES);
    close(f);
}
#endif

/** Get 32 bytes of system entropy. */
void GetOSRand(unsigned char *ent32)
{
#if defined(WIN32)
    HCRYPTPROV hProvider;
    int ret = CryptAcquireContextW(&hProvider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!ret) {
        RandFailure();
    }
    ret = CryptGenRandom(hProvider, NUM_OS_RANDOM_BYTES, ent32);
    if (!ret) {
        RandFailure();
    }
    CryptReleaseContext(hProvider, 0);
#elif defined(HAVE_GETRANDOM)
    /* Linux. From the getrandom(2) man page:
     * "If the urandom source has been initialized, reads of up to 256 bytes
     * will always return as many bytes as requested and will not be
     * interrupted by signals."
     */
    if (getrandom(ent32, NUM_OS_RANDOM_BYTES, 0) != NUM_OS_RANDOM_BYTES) {
        RandFailure();
    }
#elif defined(__OpenBSD__)
    /* OpenBSD. From the arc4random(3) man page:
       "Use of these functions is encouraged for almost all random number
        consumption because the other interfaces are deficient in either
        quality, portability, standardization, or availability."
       The function call is always successful.
     */
    arc4random_buf(ent32, NUM_OS_RANDOM_BYTES);
#elif defined(HAVE_GETENTROPY_RAND) && defined(MAC_OSX)
    if (getentropy(ent32, NUM_OS_RANDOM_BYTES) != 0) {
        RandFailure();
    }
#elif defined(HAVE_SYSCTL_ARND)
    /* FreeBSD, NetBSD and similar. It is possible for the call to return less
     * bytes than requested, so need to read in a loop.
     */
    static int name[2] = {CTL_KERN, KERN_ARND};
    int have = 0;
    do {
        size_t len = NUM_OS_RANDOM_BYTES - have;
        if (sysctl(name, std::size(name), ent32 + have, &len, nullptr, 0) != 0) {
            RandFailure();
        }
        have += len;
    } while (have < NUM_OS_RANDOM_BYTES);
#else
    /* Fall back to /dev/urandom if there is no specific method implemented to
     * get system entropy for this OS.
     */
    GetDevURandom(ent32);
#endif
}

namespace {

class RNGState {
    Mutex m_mutex;
    /* The RNG state consists of 256 bits of entropy, taken from the output of
     * one operation's SHA512 output, and fed as input to the next one.
     * Carrying 256 bits of entropy should be sufficient to guarantee
     * unpredictability as long as any entropy source was ever unpredictable
     * to an attacker. To protect against situations where an attacker might
     * observe the RNG's state, fresh entropy is always mixed when
     * GetStrongRandBytes is called.
     */
    unsigned char m_state[32] GUARDED_BY(m_mutex) = {0};
    uint64_t m_counter GUARDED_BY(m_mutex) = 0;
    bool m_strongly_seeded GUARDED_BY(m_mutex) = false;

    Mutex m_events_mutex;
    CSHA256 m_events_hasher GUARDED_BY(m_events_mutex);

public:
    RNGState() noexcept
    {
        InitHardwareRand();
    }

    ~RNGState() = default;

    void AddEvent(uint32_t event_info) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_events_mutex)
    {
        LOCK(m_events_mutex);

        m_events_hasher.Write((const unsigned char *)&event_info, sizeof(event_info));
        // Get the low four bytes of the performance counter. This translates to roughly the
        // subsecond part.
        uint32_t perfcounter = (GetPerformanceCounter() & 0xffffffff);
        m_events_hasher.Write((const unsigned char*)&perfcounter, sizeof(perfcounter));
    }

    /**
     * Feed (the hash of) all events added through AddEvent() to hasher.
     */
    void SeedEvents(CSHA512& hasher) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_events_mutex)
    {
        // We use only SHA256 for the events hashing to get the ASM speedups we have for SHA256,
        // since we want it to be fast as network peers may be able to trigger it repeatedly.
        LOCK(m_events_mutex);

        unsigned char events_hash[32];
        m_events_hasher.Finalize(events_hash);
        hasher.Write(events_hash, 32);

        // Re-initialize the hasher with the finalized state to use later.
        m_events_hasher.Reset();
        m_events_hasher.Write(events_hash, 32);
    }

    /** Extract up to 32 bytes of entropy from the RNG state, mixing in new entropy from hasher.
     *
     * If this function has never been called with strong_seed = true, false is returned.
     */
    bool MixExtract(unsigned char* out, size_t num, CSHA512&& hasher, bool strong_seed) noexcept EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        assert(num <= 32);
        unsigned char buf[64];
        static_assert(sizeof(buf) == CSHA512::OUTPUT_SIZE, "Buffer needs to have hasher's output size");
        bool ret;
        {
            LOCK(m_mutex);
            ret = (m_strongly_seeded |= strong_seed);
            // Write the current state of the RNG into the hasher
            hasher.Write(m_state, 32);
            // Write a new counter number into the state
            hasher.Write((const unsigned char*)&m_counter, sizeof(m_counter));
            ++m_counter;
            // Finalize the hasher
            hasher.Finalize(buf);
            // Store the last 32 bytes of the hash output as new RNG state.
            memcpy(m_state, buf + 32, 32);
        }
        // If desired, copy (up to) the first 32 bytes of the hash output as output.
        if (num) {
            assert(out != nullptr);
            memcpy(out, buf, num);
        }
        // Best effort cleanup of internal state
        hasher.Reset();
        memory_cleanse(buf, 64);
        return ret;
    }
};

RNGState& GetRNGState() noexcept
{
    // This idiom relies on the guarantee that static variable are initialized
    // on first call, even when multiple parallel calls are permitted.
    static std::vector<RNGState, secure_allocator<RNGState>> g_rng(1);
    return g_rng[0];
}
}

/* A note on the use of noexcept in the seeding functions below:
 *
 * None of the RNG code should ever throw any exception.
 */

static void SeedTimestamp(CSHA512& hasher) noexcept
{
    int64_t perfcounter = GetPerformanceCounter();
    hasher.Write((const unsigned char*)&perfcounter, sizeof(perfcounter));
}

static void SeedFast(CSHA512& hasher) noexcept
{
    unsigned char buffer[32];

    // Stack pointer to indirectly commit to thread/callstack
    const unsigned char* ptr = buffer;
    hasher.Write((const unsigned char*)&ptr, sizeof(ptr));

    // Hardware randomness is very fast when available; use it always.
    SeedHardwareFast(hasher);

    // High-precision timestamp
    SeedTimestamp(hasher);
}

static void SeedSlow(CSHA512& hasher, RNGState& rng) noexcept
{
    unsigned char buffer[32];

    // Everything that the 'fast' seeder includes
    SeedFast(hasher);

    // OS randomness
    GetOSRand(buffer);
    hasher.Write(buffer, sizeof(buffer));

    // Add the events hasher into the mix
    rng.SeedEvents(hasher);

    // High-precision timestamp.
    //
    // Note that we also commit to a timestamp in the Fast seeder, so we indirectly commit to a
    // benchmark of all the entropy gathering sources in this function).
    SeedTimestamp(hasher);
}

/** Extract entropy from rng, strengthen it, and feed it into hasher. */
static void SeedStrengthen(CSHA512& hasher, RNGState& rng, SteadyClock::duration dur) noexcept
{
    // Generate 32 bytes of entropy from the RNG, and a copy of the entropy already in hasher.
    unsigned char strengthen_seed[32];
    rng.MixExtract(strengthen_seed, sizeof(strengthen_seed), CSHA512(hasher), false);
    // Strengthen the seed, and feed it into hasher.
    Strengthen(strengthen_seed, dur, hasher);
}

static void SeedPeriodic(CSHA512& hasher, RNGState& rng) noexcept
{
    // Everything that the 'fast' seeder includes
    SeedFast(hasher);

    // High-precision timestamp
    SeedTimestamp(hasher);

    // Add the events hasher into the mix
    rng.SeedEvents(hasher);

    // Dynamic environment data (performance monitoring, ...)
    auto old_size = hasher.Size();
    RandAddDynamicEnv(hasher);
    LogPrint(BCLog::RAND, "Feeding %i bytes of dynamic environment data into RNG\n", hasher.Size() - old_size);

    // Strengthen for 10 ms
    SeedStrengthen(hasher, rng, 10ms);
}

static void SeedStartup(CSHA512& hasher, RNGState& rng) noexcept
{
    // Gather 256 bits of hardware randomness, if available
    SeedHardwareSlow(hasher);

    // Everything that the 'slow' seeder includes.
    SeedSlow(hasher, rng);

    // Dynamic environment data (performance monitoring, ...)
    auto old_size = hasher.Size();
    RandAddDynamicEnv(hasher);

    // Static environment data
    RandAddStaticEnv(hasher);
    LogPrint(BCLog::RAND, "Feeding %i bytes of environment data into RNG\n", hasher.Size() - old_size);

    // Strengthen for 100 ms
    SeedStrengthen(hasher, rng, 100ms);
}

enum class RNGLevel {
    FAST, //!< Automatically called by GetRandBytes
    SLOW, //!< Automatically called by GetStrongRandBytes
    PERIODIC, //!< Called by RandAddPeriodic()
};

static void ProcRand(unsigned char* out, int num, RNGLevel level) noexcept
{
    // Make sure the RNG is initialized first (as all Seed* function possibly need hwrand to be available).
    RNGState& rng = GetRNGState();

    assert(num <= 32);

    CSHA512 hasher;
    switch (level) {
    case RNGLevel::FAST:
        SeedFast(hasher);
        break;
    case RNGLevel::SLOW:
        SeedSlow(hasher, rng);
        break;
    case RNGLevel::PERIODIC:
        SeedPeriodic(hasher, rng);
        break;
    }

    // Combine with and update state
    if (!rng.MixExtract(out, num, std::move(hasher), false)) {
        // On the first invocation, also seed with SeedStartup().
        CSHA512 startup_hasher;
        SeedStartup(startup_hasher, rng);
        rng.MixExtract(out, num, std::move(startup_hasher), true);
    }
}

void GetRandBytes(Span<unsigned char> bytes) noexcept { ProcRand(bytes.data(), bytes.size(), RNGLevel::FAST); }
void GetStrongRandBytes(Span<unsigned char> bytes) noexcept { ProcRand(bytes.data(), bytes.size(), RNGLevel::SLOW); }
void RandAddPeriodic() noexcept { ProcRand(nullptr, 0, RNGLevel::PERIODIC); }
void RandAddEvent(const uint32_t event_info) noexcept { GetRNGState().AddEvent(event_info); }

bool g_mock_deterministic_tests{false};

uint64_t GetRandInternal(uint64_t nMax) noexcept
{
    return FastRandomContext(g_mock_deterministic_tests).randrange(nMax);
}

uint256 GetRandHash() noexcept
{
    uint256 hash;
    GetRandBytes(hash);
    return hash;
}

void FastRandomContext::RandomSeed() noexcept
{
    uint256 seed = GetRandHash();
    rng.SetKey(MakeByteSpan(seed));
    requires_seed = false;
}

void FastRandomContext::fillrand(Span<std::byte> output) noexcept
{
    if (requires_seed) RandomSeed();
    rng.Keystream(output);
}

FastRandomContext::FastRandomContext(const uint256& seed) noexcept : requires_seed(false), rng(MakeByteSpan(seed)) {}

bool Random_SanityCheck()
{
    uint64_t start = GetPerformanceCounter();

    /* This does not measure the quality of randomness, but it does test that
     * GetOSRand() overwrites all 32 bytes of the output given a maximum
     * number of tries.
     */
    static constexpr int MAX_TRIES{1024};
    uint8_t data[NUM_OS_RANDOM_BYTES];
    bool overwritten[NUM_OS_RANDOM_BYTES] = {}; /* Tracks which bytes have been overwritten at least once */
    int num_overwritten;
    int tries = 0;
    /* Loop until all bytes have been overwritten at least once, or max number tries reached */
    do {
        memset(data, 0, NUM_OS_RANDOM_BYTES);
        GetOSRand(data);
        for (int x=0; x < NUM_OS_RANDOM_BYTES; ++x) {
            overwritten[x] |= (data[x] != 0);
        }

        num_overwritten = 0;
        for (int x=0; x < NUM_OS_RANDOM_BYTES; ++x) {
            if (overwritten[x]) {
                num_overwritten += 1;
            }
        }

        tries += 1;
    } while (num_overwritten < NUM_OS_RANDOM_BYTES && tries < MAX_TRIES);
    if (num_overwritten != NUM_OS_RANDOM_BYTES) return false; /* If this failed, bailed out after too many tries */

    // Check that GetPerformanceCounter increases at least during a GetOSRand() call + 1ms sleep.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    uint64_t stop = GetPerformanceCounter();
    if (stop == start) return false;

    // We called GetPerformanceCounter. Use it as entropy.
    CSHA512 to_add;
    to_add.Write((const unsigned char*)&start, sizeof(start));
    to_add.Write((const unsigned char*)&stop, sizeof(stop));
    GetRNGState().MixExtract(nullptr, 0, std::move(to_add), false);

    return true;
}

static constexpr std::array<std::byte, ChaCha20::KEYLEN> ZERO_KEY{};

FastRandomContext::FastRandomContext(bool fDeterministic) noexcept : requires_seed(!fDeterministic), rng(ZERO_KEY)
{
    // Note that despite always initializing with ZERO_KEY, requires_seed is set to true if not
    // fDeterministic. That means the rng will be reinitialized with a secure random key upon first
    // use.
}

FastRandomContext& FastRandomContext::operator=(FastRandomContext&& from) noexcept
{
    requires_seed = from.requires_seed;
    rng = from.rng;
    from.requires_seed = true;
    static_cast<RandomMixin<FastRandomContext>&>(*this) = std::move(from);
    return *this;
}

void RandomInit()
{
    // Invoke RNG code to trigger initialization (if not already performed)
    ProcRand(nullptr, 0, RNGLevel::FAST);

    ReportHardwareRand();
}

std::chrono::microseconds GetExponentialRand(std::chrono::microseconds now, std::chrono::seconds average_interval)
{
    double unscaled = -std::log1p(GetRand(uint64_t{1} << 48) * -0.0000000000000035527136788 /* -1/2^48 */);
    return now + std::chrono::duration_cast<std::chrono::microseconds>(unscaled * average_interval + 0.5us);
}
