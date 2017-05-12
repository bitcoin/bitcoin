// winpipes.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "config.h"

#if !defined(NO_OS_DEPENDENCE) && defined(WINDOWS_PIPES_AVAILABLE)

#include "winpipes.h"
#include "wait.h"

// Windows 8, Windows Server 2012, and Windows Phone 8.1 need <synchapi.h> and <ioapiset.h>
#if defined(CRYPTOPP_WIN32_AVAILABLE)
# if ((WINVER >= 0x0602 /*_WIN32_WINNT_WIN8*/) || (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/))
#  include <synchapi.h>
#  include <ioapiset.h>
#  define USE_WINDOWS8_API
# endif
#endif

NAMESPACE_BEGIN(CryptoPP)

WindowsHandle::WindowsHandle(HANDLE h, bool own)
	: m_h(h), m_own(own)
{
}

WindowsHandle::~WindowsHandle()
{
	if (m_own)
	{
		try
		{
			CloseHandle();
		}
		catch (const Exception&)
		{
			CRYPTOPP_ASSERT(0);
		}
	}
}

bool WindowsHandle::HandleValid() const
{
	return m_h && m_h != INVALID_HANDLE_VALUE;
}

void WindowsHandle::AttachHandle(HANDLE h, bool own)
{
	if (m_own)
		CloseHandle();

	m_h = h;
	m_own = own;
	HandleChanged();
}

HANDLE WindowsHandle::DetachHandle()
{
	HANDLE h = m_h;
	m_h = INVALID_HANDLE_VALUE;
	HandleChanged();
	return h;
}

void WindowsHandle::CloseHandle()
{
	if (m_h != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_h);
		m_h = INVALID_HANDLE_VALUE;
		HandleChanged();
	}
}

// ********************************************************

void WindowsPipe::HandleError(const char *operation) const
{
	DWORD err = GetLastError();
	throw Err(GetHandle(), operation, err);
}

WindowsPipe::Err::Err(HANDLE s, const std::string& operation, int error)
	: OS_Error(IO_ERROR, "WindowsPipe: " + operation + " operation failed with error 0x" + IntToString(error, 16), operation, error)
	, m_h(s)
{
}

// *************************************************************

WindowsPipeReceiver::WindowsPipeReceiver()
	: m_lastResult(0), m_resultPending(false), m_eofReceived(false)
{
	m_event.AttachHandle(CreateEvent(NULL, true, false, NULL), true);
	CheckAndHandleError("CreateEvent", m_event.HandleValid());
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_overlapped.hEvent = m_event;
}

bool WindowsPipeReceiver::Receive(byte* buf, size_t bufLen)
{
	CRYPTOPP_ASSERT(!m_resultPending && !m_eofReceived);

	const HANDLE h = GetHandle();
	// don't queue too much at once, or we might use up non-paged memory
	if (ReadFile(h, buf, UnsignedMin((DWORD)128*1024, bufLen), &m_lastResult, &m_overlapped))
	{
		if (m_lastResult == 0)
			m_eofReceived = true;
	}
	else
	{
		switch (GetLastError())
		{
		default:
			CheckAndHandleError("ReadFile", false);
			// Fall through for non-fatal
		case ERROR_BROKEN_PIPE:
		case ERROR_HANDLE_EOF:
			m_lastResult = 0;
			m_eofReceived = true;
			break;
		case ERROR_IO_PENDING:
			m_resultPending = true;
		}
	}
	return !m_resultPending;
}

void WindowsPipeReceiver::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (m_resultPending)
		container.AddHandle(m_event, CallStack("WindowsPipeReceiver::GetWaitObjects() - result pending", &callStack));
	else if (!m_eofReceived)
		container.SetNoWait(CallStack("WindowsPipeReceiver::GetWaitObjects() - result ready", &callStack));
}

unsigned int WindowsPipeReceiver::GetReceiveResult()
{
	if (m_resultPending)
	{
#if defined(USE_WINDOWS8_API)
		BOOL result = GetOverlappedResultEx(GetHandle(), &m_overlapped, &m_lastResult, INFINITE, FALSE);
#else
		BOOL result = GetOverlappedResult(GetHandle(), &m_overlapped, &m_lastResult, FALSE);
#endif
		if (result)
		{
			if (m_lastResult == 0)
				m_eofReceived = true;
		}
		else
		{
			switch (GetLastError())
			{
			default:
				CheckAndHandleError("GetOverlappedResult", false);
				// Fall through for non-fatal
			case ERROR_BROKEN_PIPE:
			case ERROR_HANDLE_EOF:
				m_lastResult = 0;
				m_eofReceived = true;
			}
		}
		m_resultPending = false;
	}
	return m_lastResult;
}

// *************************************************************

WindowsPipeSender::WindowsPipeSender()
	: m_lastResult(0), m_resultPending(false)
{
	m_event.AttachHandle(CreateEvent(NULL, true, false, NULL), true);
	CheckAndHandleError("CreateEvent", m_event.HandleValid());
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_overlapped.hEvent = m_event;
}

void WindowsPipeSender::Send(const byte* buf, size_t bufLen)
{
	DWORD written = 0;
	const HANDLE h = GetHandle();
	// don't queue too much at once, or we might use up non-paged memory
	if (WriteFile(h, buf, UnsignedMin((DWORD)128*1024, bufLen), &written, &m_overlapped))
	{
		m_resultPending = false;
		m_lastResult = written;
	}
	else
	{
		if (GetLastError() != ERROR_IO_PENDING)
			CheckAndHandleError("WriteFile", false);

		m_resultPending = true;
	}
}

void WindowsPipeSender::GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack)
{
	if (m_resultPending)
		container.AddHandle(m_event, CallStack("WindowsPipeSender::GetWaitObjects() - result pending", &callStack));
	else
		container.SetNoWait(CallStack("WindowsPipeSender::GetWaitObjects() - result ready", &callStack));
}

unsigned int WindowsPipeSender::GetSendResult()
{
	if (m_resultPending)
	{
		const HANDLE h = GetHandle();
#if defined(USE_WINDOWS8_API)
		BOOL result = GetOverlappedResultEx(h, &m_overlapped, &m_lastResult, INFINITE, FALSE);
		CheckAndHandleError("GetOverlappedResultEx", result);
#else
		BOOL result = GetOverlappedResult(h, &m_overlapped, &m_lastResult, FALSE);
		CheckAndHandleError("GetOverlappedResult", result);
#endif
		m_resultPending = false;
	}
	return m_lastResult;
}

NAMESPACE_END

#endif
