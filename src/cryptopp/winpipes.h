#ifndef CRYPTOPP_WINPIPES_H
#define CRYPTOPP_WINPIPES_H

#include "config.h"

#if !defined(NO_OS_DEPENDENCE) && defined(WINDOWS_PIPES_AVAILABLE)

#include "cryptlib.h"
#include "network.h"
#include "queue.h"
#include <winsock2.h>

NAMESPACE_BEGIN(CryptoPP)

//! \brief Windows Handle
class WindowsHandle
{
public:
	WindowsHandle(HANDLE h = INVALID_HANDLE_VALUE, bool own=false);
	WindowsHandle(const WindowsHandle &h) : m_h(h.m_h), m_own(false) {}
	virtual ~WindowsHandle();

	bool GetOwnership() const {return m_own;}
	void SetOwnership(bool own) {m_own = own;}

	operator HANDLE() const {return m_h;}
	HANDLE GetHandle() const {return m_h;}
	bool HandleValid() const;
	void AttachHandle(HANDLE h, bool own=false);
	HANDLE DetachHandle();
	void CloseHandle();

protected:
	virtual void HandleChanged() {}

	HANDLE m_h;
	bool m_own;
};

//! \brief Windows Pipe
class WindowsPipe
{
public:
	class Err : public OS_Error
	{
	public:
		Err(HANDLE h, const std::string& operation, int error);
		HANDLE GetHandle() const {return m_h;}

	private:
		HANDLE m_h;
	};

protected:
	virtual HANDLE GetHandle() const =0;
	virtual void HandleError(const char *operation) const;
	void CheckAndHandleError(const char *operation, BOOL result) const
		{if (!result) HandleError(operation);}
};

//! \brief Pipe-based implementation of NetworkReceiver
class WindowsPipeReceiver : public WindowsPipe, public NetworkReceiver
{
public:
	WindowsPipeReceiver();

	bool MustWaitForResult() {return true;}
	bool Receive(byte* buf, size_t bufLen);
	unsigned int GetReceiveResult();
	bool EofReceived() const {return m_eofReceived;}

	HANDLE GetHandle() const {return m_event;}
	unsigned int GetMaxWaitObjectCount() const {return 1;}
	void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack);

private:
	WindowsHandle m_event;
	OVERLAPPED m_overlapped;
	DWORD m_lastResult;
	bool m_resultPending;
	bool m_eofReceived;
};

//! \brief Pipe-based implementation of NetworkSender
class WindowsPipeSender : public WindowsPipe, public NetworkSender
{
public:
	WindowsPipeSender();

	bool MustWaitForResult() {return true;}
	void Send(const byte* buf, size_t bufLen);
	unsigned int GetSendResult();
	bool MustWaitForEof() { return false; }
	void SendEof() {}

	HANDLE GetHandle() const {return m_event;}
	unsigned int GetMaxWaitObjectCount() const {return 1;}
	void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack);

private:
	WindowsHandle m_event;
	OVERLAPPED m_overlapped;
	DWORD m_lastResult;
	bool m_resultPending;
};

//! \brief Windows Pipe Source
class WindowsPipeSource : public WindowsHandle, public NetworkSource, public WindowsPipeReceiver
{
public:
	WindowsPipeSource(HANDLE h=INVALID_HANDLE_VALUE, bool pumpAll=false, BufferedTransformation *attachment=NULL)
		: WindowsHandle(h), NetworkSource(attachment)
	{
		if (pumpAll)
			PumpAll();
	}

	using NetworkSource::GetMaxWaitObjectCount;
	using NetworkSource::GetWaitObjects;

private:
	HANDLE GetHandle() const {return WindowsHandle::GetHandle();}
	NetworkReceiver & AccessReceiver() {return *this;}
};

//! \brief Windows Pipe Sink
class WindowsPipeSink : public WindowsHandle, public NetworkSink, public WindowsPipeSender
{
public:
	WindowsPipeSink(HANDLE h=INVALID_HANDLE_VALUE, unsigned int maxBufferSize=0, unsigned int autoFlushBound=16*1024)
		: WindowsHandle(h), NetworkSink(maxBufferSize, autoFlushBound) {}

	using NetworkSink::GetMaxWaitObjectCount;
	using NetworkSink::GetWaitObjects;

private:
	HANDLE GetHandle() const {return WindowsHandle::GetHandle();}
	NetworkSender & AccessSender() {return *this;}
};

NAMESPACE_END

#endif // WINDOWS_PIPES_AVAILABLE

#endif
