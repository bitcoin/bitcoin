// hrtimer.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "hrtimer.h"
#include "misc.h"
#include <stddef.h>		// for NULL
#include <time.h>

#if defined(CRYPTOPP_WIN32_AVAILABLE)
#include <windows.h>
#elif defined(CRYPTOPP_UNIX_AVAILABLE)
#include <sys/time.h>
#include <sys/times.h>
#include <unistd.h>
#endif

#include <assert.h>

NAMESPACE_BEGIN(CryptoPP)

#ifndef CRYPTOPP_IMPORTS

double TimerBase::ConvertTo(TimerWord t, Unit unit)
{
	static unsigned long unitsPerSecondTable[] = {1, 1000, 1000*1000, 1000*1000*1000};

	// When 'unit' is an enum 'Unit', a Clang warning is generated.
	assert(static_cast<unsigned int>(unit) < COUNTOF(unitsPerSecondTable));
	return (double)CRYPTOPP_VC6_INT64 t * unitsPerSecondTable[unit] / CRYPTOPP_VC6_INT64 TicksPerSecond();
}

void TimerBase::StartTimer()
{
	m_last = m_start = GetCurrentTimerValue();
	m_started = true;
}

double TimerBase::ElapsedTimeAsDouble()
{
	if (m_stuckAtZero)
		return 0;

	if (m_started)
	{
		TimerWord now = GetCurrentTimerValue();
		if (m_last < now)	// protect against OS bugs where time goes backwards
			m_last = now;
		return ConvertTo(m_last - m_start, m_timerUnit);
	}

	StartTimer();
	return 0;
}

unsigned long TimerBase::ElapsedTime()
{
	double elapsed = ElapsedTimeAsDouble();
	assert(elapsed <= ULONG_MAX);
	return (unsigned long)elapsed;
}

TimerWord Timer::GetCurrentTimerValue()
{
#if defined(CRYPTOPP_WIN32_AVAILABLE)
	// Use the first union member to avoid an uninitialized warning
	LARGE_INTEGER now = {0,0};
	if (!QueryPerformanceCounter(&now))
		throw Exception(Exception::OTHER_ERROR, "Timer: QueryPerformanceCounter failed with error " + IntToString(GetLastError()));
	return now.QuadPart;
#elif defined(CRYPTOPP_UNIX_AVAILABLE)
	timeval now;
	gettimeofday(&now, NULL);
	return (TimerWord)now.tv_sec * 1000000 + now.tv_usec;
#else
	// clock_t now;
	return clock();
#endif
}

TimerWord Timer::TicksPerSecond()
{
#if defined(CRYPTOPP_WIN32_AVAILABLE)
	// Use the second union member to avoid an uninitialized warning
	static LARGE_INTEGER freq = {0,0};
	if (freq.QuadPart == 0)
	{
		if (!QueryPerformanceFrequency(&freq))
			throw Exception(Exception::OTHER_ERROR, "Timer: QueryPerformanceFrequency failed with error " + IntToString(GetLastError()));
	}
	return freq.QuadPart;
#elif defined(CRYPTOPP_UNIX_AVAILABLE)
	return 1000000;
#else
	return CLOCKS_PER_SEC;
#endif
}

#endif	// #ifndef CRYPTOPP_IMPORTS

TimerWord ThreadUserTimer::GetCurrentTimerValue()
{
#if defined(CRYPTOPP_WIN32_AVAILABLE)
	static bool getCurrentThreadImplemented = true;
	if (getCurrentThreadImplemented)
	{
		FILETIME now, ignored;
		if (!GetThreadTimes(GetCurrentThread(), &ignored, &ignored, &ignored, &now))
		{
			DWORD lastError = GetLastError();
			if (lastError == ERROR_CALL_NOT_IMPLEMENTED)
			{
				getCurrentThreadImplemented = false;
				goto GetCurrentThreadNotImplemented;
			}
			throw Exception(Exception::OTHER_ERROR, "ThreadUserTimer: GetThreadTimes failed with error " + IntToString(lastError));
		}
		return now.dwLowDateTime + ((TimerWord)now.dwHighDateTime << 32);
	}
GetCurrentThreadNotImplemented:
	return (TimerWord)clock() * (10*1000*1000 / CLOCKS_PER_SEC);
#elif defined(CRYPTOPP_UNIX_AVAILABLE)
	tms now;
	times(&now);
	return now.tms_utime;
#else
	return clock();
#endif
}

TimerWord ThreadUserTimer::TicksPerSecond()
{
#if defined(CRYPTOPP_WIN32_AVAILABLE)
	return 10*1000*1000;
#elif defined(CRYPTOPP_UNIX_AVAILABLE)
	static const long ticksPerSecond = sysconf(_SC_CLK_TCK);
	return ticksPerSecond;
#else
	return CLOCKS_PER_SEC;
#endif
}

NAMESPACE_END
