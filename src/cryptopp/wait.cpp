// wait.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#if CRYPTOPP_MSC_VERSION
# pragma warning(disable: 4189)
#endif

#include "wait.h"
#include "misc.h"
#include "smartptr.h"

#ifdef SOCKETS_AVAILABLE

#ifdef USE_BERKELEY_STYLE_SOCKETS
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

unsigned int WaitObjectContainer::MaxWaitObjects()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	return MAXIMUM_WAIT_OBJECTS * (MAXIMUM_WAIT_OBJECTS-1);
#else
	return FD_SETSIZE;
#endif
}

WaitObjectContainer::WaitObjectContainer(WaitObjectsTracer* tracer)
	: m_tracer(tracer), m_eventTimer(Timer::MILLISECONDS), m_lastResult(0)
	, m_sameResultCount(0), m_noWaitTimer(Timer::MILLISECONDS)
{
	Clear();
	m_eventTimer.StartTimer();
}

void WaitObjectContainer::Clear()
{
#ifdef USE_WINDOWS_STYLE_SOCKETS
	m_handles.clear();
#else
	m_maxFd = 0;
	FD_ZERO(&m_readfds);
	FD_ZERO(&m_writefds);
#endif
	m_noWait = false;
	m_firstEventTime = 0;
}

inline void WaitObjectContainer::SetLastResult(LastResultType result)
{
	if (result == m_lastResult)
		m_sameResultCount++;
	else
	{
		m_lastResult = result;
		m_sameResultCount = 0;
	}
}

void WaitObjectContainer::DetectNoWait(LastResultType result, CallStack const& callStack)
{
	if (result == m_lastResult && m_noWaitTimer.ElapsedTime() > 1000)
	{
		if (m_sameResultCount > m_noWaitTimer.ElapsedTime())
		{
			if (m_tracer)
			{
				std::string desc = "No wait loop detected - m_lastResult: ";
				desc.append(IntToString(m_lastResult)).append(", call stack:");
				for (CallStack const* cs = &callStack; cs; cs = cs->Prev())
					desc.append("\n- ").append(cs->Format());
				m_tracer->TraceNoWaitLoop(desc);
			}
			try { throw 0; } catch (...) {}		// help debugger break
		}

		m_noWaitTimer.StartTimer();
		m_sameResultCount = 0;
	}
}

void WaitObjectContainer::SetNoWait(CallStack const& callStack)
{
	DetectNoWait(LastResultType(LASTRESULT_NOWAIT), CallStack("WaitObjectContainer::SetNoWait()", &callStack));
	m_noWait = true;
}

void WaitObjectContainer::ScheduleEvent(double milliseconds, CallStack const& callStack)
{
	if (milliseconds <= 3)
		DetectNoWait(LastResultType(LASTRESULT_SCHEDULED), CallStack("WaitObjectContainer::ScheduleEvent()", &callStack));
	double thisEventTime = m_eventTimer.ElapsedTimeAsDouble() + milliseconds;
	if (!m_firstEventTime || thisEventTime < m_firstEventTime)
		m_firstEventTime = thisEventTime;
}

#ifdef USE_WINDOWS_STYLE_SOCKETS

struct WaitingThreadData
{
	bool waitingToWait, terminate;
	HANDLE startWaiting, stopWaiting;
	const HANDLE *waitHandles;
	unsigned int count;
	HANDLE threadHandle;
	DWORD threadId;
	DWORD* error;
};

WaitObjectContainer::~WaitObjectContainer()
{
	try		// don't let exceptions escape destructor
	{
		if (!m_threads.empty())
		{
			HANDLE threadHandles[MAXIMUM_WAIT_OBJECTS] = {0};
			
			unsigned int i;
			for (i=0; i<m_threads.size(); i++)
			{
				// Enterprise Analysis warning
				if(!m_threads[i]) continue;

				WaitingThreadData &thread = *m_threads[i];
				while (!thread.waitingToWait)	// spin until thread is in the initial "waiting to wait" state
					Sleep(0);
				thread.terminate = true;
				threadHandles[i] = thread.threadHandle;
			}

			BOOL bResult = PulseEvent(m_startWaiting);
			assert(bResult != 0); CRYPTOPP_UNUSED(bResult);
	
			// Enterprise Analysis warning
			DWORD dwResult = ::WaitForMultipleObjects((DWORD)m_threads.size(), threadHandles, TRUE, INFINITE);
			assert((dwResult >= WAIT_OBJECT_0) && (dwResult < (DWORD)m_threads.size()));

			for (i=0; i<m_threads.size(); i++)
			{
				// Enterprise Analysis warning
				if (!threadHandles[i]) continue;
									
				bResult = CloseHandle(threadHandles[i]);
				assert(bResult != 0);
			}

			bResult = CloseHandle(m_startWaiting);
			assert(bResult != 0);
			bResult = CloseHandle(m_stopWaiting);
			assert(bResult != 0);
		}
	}
	catch (const Exception&)
	{
		assert(0);
	}
}

void WaitObjectContainer::AddHandle(HANDLE handle, CallStack const& callStack)
{
	DetectNoWait(m_handles.size(), CallStack("WaitObjectContainer::AddHandle()", &callStack));
	m_handles.push_back(handle);
}

DWORD WINAPI WaitingThread(LPVOID lParam)
{
	member_ptr<WaitingThreadData> pThread((WaitingThreadData *)lParam);
	WaitingThreadData &thread = *pThread;
	std::vector<HANDLE> handles;

	while (true)
	{
		thread.waitingToWait = true;
		DWORD result = ::WaitForSingleObject(thread.startWaiting, INFINITE);
		assert(result != WAIT_FAILED);
		
		thread.waitingToWait = false;
		if (thread.terminate)
			break;
		if (!thread.count)
			continue;

		handles.resize(thread.count + 1);
		handles[0] = thread.stopWaiting;
		std::copy(thread.waitHandles, thread.waitHandles+thread.count, handles.begin()+1);

		result = ::WaitForMultipleObjects((DWORD)handles.size(), &handles[0], FALSE, INFINITE);
		assert(result != WAIT_FAILED);

		if (result == WAIT_OBJECT_0)
			continue;	// another thread finished waiting first, so do nothing
		SetEvent(thread.stopWaiting);
		if (!(result > WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + handles.size()))
		{
			assert(!"error in WaitingThread");	// break here so we can see which thread has an error
			*thread.error = ::GetLastError();
		}
	}

	return S_OK;	// return a value here to avoid compiler warning
}

void WaitObjectContainer::CreateThreads(unsigned int count)
{
	size_t currentCount = m_threads.size();
	if (currentCount == 0)
	{
		m_startWaiting = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		m_stopWaiting = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	if (currentCount < count)
	{
		m_threads.resize(count);
		for (size_t i=currentCount; i<count; i++)
		{
			// Enterprise Analysis warning
			if(!m_threads[i]) continue;
	
			m_threads[i] = new WaitingThreadData;
			WaitingThreadData &thread = *m_threads[i];
			thread.terminate = false;
			thread.startWaiting = m_startWaiting;
			thread.stopWaiting = m_stopWaiting;
			thread.waitingToWait = false;
			thread.threadHandle = CreateThread(NULL, 0, &WaitingThread, &thread, 0, &thread.threadId);
		}
	}
}

bool WaitObjectContainer::Wait(unsigned long milliseconds)
{
	if (m_noWait || (m_handles.empty() && !m_firstEventTime))
	{
		SetLastResult(LastResultType(LASTRESULT_NOWAIT));
		return true;
	}

	bool timeoutIsScheduledEvent = false;

	if (m_firstEventTime)
	{
		double timeToFirstEvent = SaturatingSubtract(m_firstEventTime, m_eventTimer.ElapsedTimeAsDouble());

		if (timeToFirstEvent <= milliseconds)
		{
			milliseconds = (unsigned long)timeToFirstEvent;
			timeoutIsScheduledEvent = true;
		}

		if (m_handles.empty() || !milliseconds)
		{
			if (milliseconds)
				Sleep(milliseconds);
			SetLastResult(timeoutIsScheduledEvent ? LASTRESULT_SCHEDULED : LASTRESULT_TIMEOUT);
			return timeoutIsScheduledEvent;
		}
	}

	if (m_handles.size() > MAXIMUM_WAIT_OBJECTS)
	{
		// too many wait objects for a single WaitForMultipleObjects call, so use multiple threads
		static const unsigned int WAIT_OBJECTS_PER_THREAD = MAXIMUM_WAIT_OBJECTS-1;
		unsigned int nThreads = (unsigned int)((m_handles.size() + WAIT_OBJECTS_PER_THREAD - 1) / WAIT_OBJECTS_PER_THREAD);
		if (nThreads > MAXIMUM_WAIT_OBJECTS)	// still too many wait objects, maybe implement recursive threading later?
			throw Err("WaitObjectContainer: number of wait objects exceeds limit");
		CreateThreads(nThreads);
		DWORD error = S_OK;
		
		for (unsigned int i=0; i<m_threads.size(); i++)
		{
			// Enterprise Analysis warning
			if(!m_threads[i]) continue;

			WaitingThreadData &thread = *m_threads[i];
			while (!thread.waitingToWait)	// spin until thread is in the initial "waiting to wait" state
				Sleep(0);
			if (i<nThreads)
			{
				thread.waitHandles = &m_handles[i*WAIT_OBJECTS_PER_THREAD];
				thread.count = UnsignedMin(WAIT_OBJECTS_PER_THREAD, m_handles.size() - i*WAIT_OBJECTS_PER_THREAD);
				thread.error = &error;
			}
			else
				thread.count = 0;
		}

		ResetEvent(m_stopWaiting);
		PulseEvent(m_startWaiting);

		DWORD result = ::WaitForSingleObject(m_stopWaiting, milliseconds);
		assert(result != WAIT_FAILED);

		if (result == WAIT_OBJECT_0)
		{
			if (error == S_OK)
				return true;
			else
				throw Err("WaitObjectContainer: WaitForMultipleObjects in thread failed with error " + IntToString(error));
		}
		SetEvent(m_stopWaiting);
		if (result == WAIT_TIMEOUT)
		{
			SetLastResult(timeoutIsScheduledEvent ? LASTRESULT_SCHEDULED : LASTRESULT_TIMEOUT);
			return timeoutIsScheduledEvent;
		}
		else
			throw Err("WaitObjectContainer: WaitForSingleObject failed with error " + IntToString(::GetLastError()));
	}
	else
	{
#if TRACE_WAIT
		static Timer t(Timer::MICROSECONDS);
		static unsigned long lastTime = 0;
		unsigned long timeBeforeWait = t.ElapsedTime();
#endif
		DWORD result = ::WaitForMultipleObjects((DWORD)m_handles.size(), &m_handles[0], FALSE, milliseconds);
#if TRACE_WAIT
		if (milliseconds > 0)
		{
			unsigned long timeAfterWait = t.ElapsedTime();
			OutputDebugString(("Handles " + IntToString(m_handles.size()) + ", Woke up by " + IntToString(result-WAIT_OBJECT_0) + ", Busied for " + IntToString(timeBeforeWait-lastTime) + " us, Waited for " + IntToString(timeAfterWait-timeBeforeWait) + " us, max " + IntToString(milliseconds) + "ms\n").c_str());
			lastTime = timeAfterWait;
		}
#endif
		if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + m_handles.size())
		{
			if (result == m_lastResult)
				m_sameResultCount++;
			else
			{
				m_lastResult = result;
				m_sameResultCount = 0;
			}
			return true;
		}
		else if (result == WAIT_TIMEOUT)
		{
			SetLastResult(timeoutIsScheduledEvent ? LASTRESULT_SCHEDULED : LASTRESULT_TIMEOUT);
			return timeoutIsScheduledEvent;
		}
		else
			throw Err("WaitObjectContainer: WaitForMultipleObjects failed with error " + IntToString(::GetLastError()));
	}
}

#else // #ifdef USE_WINDOWS_STYLE_SOCKETS

void WaitObjectContainer::AddReadFd(int fd, CallStack const& callStack)	// TODO: do something with callStack
{
	CRYPTOPP_UNUSED(callStack);
	FD_SET(fd, &m_readfds);
	m_maxFd = STDMAX(m_maxFd, fd);
}

void WaitObjectContainer::AddWriteFd(int fd, CallStack const& callStack) // TODO: do something with callStack
{
	CRYPTOPP_UNUSED(callStack);
	FD_SET(fd, &m_writefds);
	m_maxFd = STDMAX(m_maxFd, fd);
}

bool WaitObjectContainer::Wait(unsigned long milliseconds)
{
	if (m_noWait || (!m_maxFd && !m_firstEventTime))
		return true;

	bool timeoutIsScheduledEvent = false;

	if (m_firstEventTime)
	{
		double timeToFirstEvent = SaturatingSubtract(m_firstEventTime, m_eventTimer.ElapsedTimeAsDouble());
		if (timeToFirstEvent <= milliseconds)
		{
			milliseconds = (unsigned long)timeToFirstEvent;
			timeoutIsScheduledEvent = true;
		}
	}

	timeval tv, *timeout;

	if (milliseconds == INFINITE_TIME)
		timeout = NULL;
	else
	{
		tv.tv_sec = milliseconds / 1000;
		tv.tv_usec = (milliseconds % 1000) * 1000;
		timeout = &tv;
	}

	int result = select(m_maxFd+1, &m_readfds, &m_writefds, NULL, timeout);

	if (result > 0)
		return true;
	else if (result == 0)
		return timeoutIsScheduledEvent;
	else
		throw Err("WaitObjectContainer: select failed with error " + IntToString(errno));
}

#endif

// ********************************************************

std::string CallStack::Format() const
{
	return m_info;
}

std::string CallStackWithNr::Format() const
{
	return std::string(m_info) + " / nr: " + IntToString(m_nr);
}

std::string CallStackWithStr::Format() const
{
	return std::string(m_info) + " / " + std::string(m_z);
}

bool Waitable::Wait(unsigned long milliseconds, CallStack const& callStack)
{
	WaitObjectContainer container;
	GetWaitObjects(container, callStack);	// reduce clutter by not adding this func to stack
	return container.Wait(milliseconds);
}

NAMESPACE_END

#endif
