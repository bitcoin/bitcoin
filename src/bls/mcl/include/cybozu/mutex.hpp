#pragma once
/**
	@file
	@brief mutex

	@author MITSUNARI Shigeo(@herumi)
	@author MITSUNARI Shigeo
*/

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#else
	#include <pthread.h>
	#include <time.h>
#endif
#include <assert.h>
#include <stdlib.h>

namespace cybozu {

class ConditionVariable;

namespace thread {

#ifdef _WIN32
	typedef HANDLE MutexHandle;
	inline void MutexInit(MutexHandle& mutex)
	{
//		mutex = CreateSemaphore(NULL /* no security */, 1 /* init */, 0x7FFFFFFF /* max */, NULL /* no name */);
		mutex = CreateMutex(NULL /* no security */, FALSE /* no owner */, NULL /* no name */);
	}
	inline void MutexLock(MutexHandle& mutex) { WaitForSingleObject(mutex, INFINITE); }
	/*
		return false if timeout
		@param msec [in] msec
	*/
	inline bool MutexLockTimeout(MutexHandle& mutex, int msec)
	{
		DWORD ret = WaitForSingleObject(mutex, msec);
		if (ret == WAIT_OBJECT_0) {
			return true;
		}
		if (ret == WAIT_TIMEOUT) {
			return false;
		}
		/* ret == WAIT_ABANDONED */
		assert(0);
		return false;
	}
	inline void MutexUnlock(MutexHandle& mutex)
	{
//		ReleaseSemaphore(mutex, 1, NULL);
		ReleaseMutex(mutex);
	}
	inline void MutexTerm(MutexHandle& mutex) { CloseHandle(mutex); }
#else
	typedef pthread_mutex_t MutexHandle;
	inline void MutexInit(MutexHandle& mutex)
	{
#if 1
		pthread_mutex_init(&mutex, NULL);
#else
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_TIMED_NP)) {
			perror("pthread_mutexattr_settype");
			exit(1);
		}
		pthread_mutex_init(&mutex, &attr);
		pthread_mutexattr_destroy(&attr);
#endif
	}
	inline void MutexLock(MutexHandle& mutex) { pthread_mutex_lock(&mutex); }
#if 0
	inline bool MutexLockTimeout(MutexHandle& mutex, int msec)
	{
		timespec absTime;
		clock_gettime(CLOCK_REALTIME, &absTime);
		absTime.tv_sec += msec / 1000;
		absTime.tv_nsec += msec % 1000;
		bool ret = pthread_mutex_timedlock(&mutex, &absTime) == 0;
		return ret;
	}
#endif
	inline void MutexUnlock(MutexHandle& mutex) { pthread_mutex_unlock(&mutex); }
	inline void MutexTerm(MutexHandle& mutex) { pthread_mutex_destroy(&mutex); }
#endif

template<class T>
class AutoLockT {
public:
	explicit AutoLockT(T &t)
		: t_(t)
	{
		t_.lock();
	}
	~AutoLockT()
	{
		t_.unlock();
	}
private:
	T& t_;
	AutoLockT& operator=(const AutoLockT&);
};

} // cybozu::thread

class Mutex {
	friend class cybozu::ConditionVariable;
public:
	Mutex()
	{
		thread::MutexInit(hdl_);
	}
	~Mutex()
	{
		thread::MutexTerm(hdl_);
	}
	void lock()
	{
		thread::MutexLock(hdl_);
	}
#if 0
	bool lockTimeout(int msec)
	{
		return thread::MutexLockTimeout(hdl_, msec);
	}
#endif
	void unlock()
	{
		thread::MutexUnlock(hdl_);
	}
private:
	Mutex(const Mutex&);
	Mutex& operator=(const Mutex&);
	thread::MutexHandle hdl_;
};

typedef cybozu::thread::AutoLockT<cybozu::Mutex> AutoLock;

} // cybozu
