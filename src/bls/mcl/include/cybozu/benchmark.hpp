#pragma once
/**
	@file
	@brief measure exec time of function
	@author MITSUNARI Shigeo
*/
#if defined(_MSC_VER) && (MSC_VER <= 1500)
	#include <cybozu/inttype.hpp>
#else
	#include <stdint.h>
#endif
#include <stdio.h>

#ifdef __EMSCRIPTEN__
	#define CYBOZU_BENCH_USE_GETTIMEOFDAY
#endif

#ifdef CYBOZU_BENCH_USE_GETTIMEOFDAY
	#include <sys/time.h>
#elif !defined(CYBOZU_BENCH_DONT_USE_RDTSC)
	#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
		#define CYBOZU_BENCH_USE_RDTSC
		#define CYBOZU_BENCH_USE_CPU_TIMER
	#endif
	#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
//		#define CYBOZU_BENCH_USE_MRC
//		#define CYBOZU_BENCH_USE_CPU_TIMER
	#endif
#endif


#include <assert.h>
#include <time.h>
#ifdef _MSC_VER
	#include <intrin.h>
	#include <sys/timeb.h>
#else
#endif

#ifndef CYBOZU_UNUSED
	#ifdef __GNUC__
		#define CYBOZU_UNUSED __attribute__((unused))
	#else
		#define CYBOZU_UNUSED
	#endif
#endif

namespace cybozu {

namespace bench {

static void (*g_putCallback)(double);

static inline void setPutCallback(void (*f)(double))
{
	g_putCallback = f;
}

} // cybozu::bench

class CpuClock {
public:
	static inline uint64_t getCpuClk()
	{
#ifdef CYBOZU_BENCH_USE_RDTSC
#ifdef _MSC_VER
		return __rdtsc();
#else
		unsigned int eax, edx;
		__asm__ volatile("rdtsc" : "=a"(eax), "=d"(edx));
		return ((uint64_t)edx << 32) | eax;
#endif
#elif defined(CYBOZU_BENCH_USE_MRC)
		uint32_t clk;
		__asm__ volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(clk));
		return clk;
#else
#ifdef _MSC_VER
		struct _timeb timeb;
		_ftime_s(&timeb);
		return uint64_t(timeb.time) * 1000000000 + timeb.millitm * 1000000;
#elif defined(CYBOZU_BENCH_USE_GETTIMEOFDAY)
		struct timeval tv;
		int ret CYBOZU_UNUSED = gettimeofday(&tv, 0);
		assert(ret == 0);
		return uint64_t(tv.tv_sec) * 1000000000 + tv.tv_usec * 1000;
#else
		struct timespec tp;
		int ret CYBOZU_UNUSED = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
		assert(ret == 0);
		return uint64_t(tp.tv_sec) * 1000000000 + tp.tv_nsec;
#endif
#endif
	}
	CpuClock()
		: clock_(0)
		, count_(0)
	{
	}
	void begin()
	{
		clock_ -= getCpuClk();
	}
	void end()
	{
		clock_ += getCpuClk();
		count_++;
	}
	int getCount() const { return count_; }
	uint64_t getClock() const { return clock_; }
	void clear() { count_ = 0; clock_ = 0; }
	void put(const char *msg = 0, int N = 1) const
	{
		double t = getClock() / double(getCount()) / N;
		if (msg && *msg) printf("%s ", msg);
		if (bench::g_putCallback) {
			bench::g_putCallback(t);
			return;
		}
#ifdef CYBOZU_BENCH_USE_CPU_TIMER
		if (t > 1e6) {
			printf("%7.3fMclk", t * 1e-6);
		} else if (t > 1e3) {
			printf("%7.3fKclk", t * 1e-3);
		} else {
			printf("%6.2f clk", t);
		}
#else
		if (t > 1e6) {
			printf("%7.3fmsec", t * 1e-6);
		} else if (t > 1e3) {
			printf("%7.3fusec", t * 1e-3);
		} else {
			printf("%6.2fnsec", t);
		}
#endif
		if (msg && *msg) printf("\n");
	}
	// adhoc constatns for CYBOZU_BENCH
#ifdef CYBOZU_BENCH_USE_CPU_TIMER
	static const int loopN1 = 1000;
	static const int loopN2 = 100;
	static const uint64_t maxClk = (uint64_t)1e8;
#else
	static const int loopN1 = 100;
	static const int loopN2 = 100;
	static const uint64_t maxClk = (uint64_t)1e8;
#endif
private:
	uint64_t clock_;
	int count_;
};

namespace bench {

static CpuClock g_clk;
static int CYBOZU_UNUSED g_loopNum;

} // cybozu::bench
/*
	loop counter is automatically determined
	CYBOZU_BENCH(<msg>, <func>, <param1>, <param2>, ...);
	if msg == "" then only set g_clk, g_loopNum
*/
#define CYBOZU_BENCH(msg, func, ...) \
{ \
	const uint64_t _cybozu_maxClk = cybozu::CpuClock::maxClk; \
	cybozu::CpuClock _cybozu_clk; \
	for (int _cybozu_i = 0; _cybozu_i < cybozu::CpuClock::loopN2; _cybozu_i++) { \
		_cybozu_clk.begin(); \
		for (int _cybozu_j = 0; _cybozu_j < cybozu::CpuClock::loopN1; _cybozu_j++) { func(__VA_ARGS__); } \
		_cybozu_clk.end(); \
		if (_cybozu_clk.getClock() > _cybozu_maxClk) break; \
	} \
	if (msg && *msg) _cybozu_clk.put(msg, cybozu::CpuClock::loopN1); \
	cybozu::bench::g_clk = _cybozu_clk; cybozu::bench::g_loopNum = cybozu::CpuClock::loopN1; \
}

/*
	double clk;
	CYBOZU_BENCH_T(clk, <func>, <param1>, <param2>, ...);
	clk is set by CYBOZU_BENCH_T
*/
#define CYBOZU_BENCH_T(clk, func, ...) \
{ \
	const uint64_t _cybozu_maxClk = cybozu::CpuClock::maxClk; \
	cybozu::CpuClock _cybozu_clk; \
	for (int _cybozu_i = 0; _cybozu_i < cybozu::CpuClock::loopN2; _cybozu_i++) { \
		_cybozu_clk.begin(); \
		for (int _cybozu_j = 0; _cybozu_j < cybozu::CpuClock::loopN1; _cybozu_j++) { func(__VA_ARGS__); } \
		_cybozu_clk.end(); \
		if (_cybozu_clk.getClock() > _cybozu_maxClk) break; \
	} \
	clk = _cybozu_clk.getClock() / (double)_cybozu_clk.getCount() / cybozu::CpuClock::loopN1; \
}

/*
	loop counter N is given
	CYBOZU_BENCH_C(<msg>, <counter>, <func>, <param1>, <param2>, ...);
	if msg == "" then only set g_clk, g_loopNum
*/
#define CYBOZU_BENCH_C(msg, _N, func, ...) \
{ \
	cybozu::CpuClock _cybozu_clk; \
	_cybozu_clk.begin(); \
	for (int _cybozu_j = 0; _cybozu_j < _N; _cybozu_j++) { func(__VA_ARGS__); } \
	_cybozu_clk.end(); \
	if (msg && *msg) _cybozu_clk.put(msg, _N); \
	cybozu::bench::g_clk = _cybozu_clk; cybozu::bench::g_loopNum = _N; \
}

} // cybozu
