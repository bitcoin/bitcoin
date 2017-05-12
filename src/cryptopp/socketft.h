#ifndef CRYPTOPP_SOCKETFT_H
#define CRYPTOPP_SOCKETFT_H

#include "config.h"

#if !defined(NO_OS_DEPENDENCE) && defined(SOCKETS_AVAILABLE)

#include "cryptlib.h"
#include "network.h"
#include "queue.h"

#ifdef USE_WINDOWS_STYLE_SOCKETS
#	if defined(_WINSOCKAPI_) && !defined(_WINSOCK2API_)
#		error Winsock 1 is not supported by this library. Please include this file or winsock2.h before windows.h.
#	endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "winpipes.h"
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

NAMESPACE_BEGIN(CryptoPP)

#ifdef USE_WINDOWS_STYLE_SOCKETS
typedef ::SOCKET socket_t;
#else
typedef int socket_t;
const socket_t INVALID_SOCKET = -1;
// cygwin 1.1.4 doesn't have SHUT_RD
const int SD_RECEIVE = 0;
const int SD_SEND = 1;
const int SD_BOTH = 2;
const int SOCKET_ERROR = -1;
#endif

#ifndef socklen_t
typedef TYPE_OF_SOCKLEN_T socklen_t;	// see config.h
#endif

//! wrapper for Windows or Berkeley Sockets
class Socket
{
public:
	//! exception thrown by Socket class
	class Err : public OS_Error
	{
	public:
		Err(socket_t s, const std::string& operation, int error);
		socket_t GetSocket() const {return m_s;}

	private:
		socket_t m_s;
	};

	Socket(socket_t s = INVALID_SOCKET, bool own=false) : m_s(s), m_own(own) {}
	Socket(const Socket &s) : m_s(s.m_s), m_own(false) {}
	virtual ~Socket();

	bool GetOwnership() const {return m_own;}
	void SetOwnership(bool own) {m_own = own;}

	operator socket_t() {return m_s;}
	socket_t GetSocket() const {return m_s;}
	void AttachSocket(socket_t s, bool own=false);
	socket_t DetachSocket();
	void CloseSocket();

	void Create(int nType = SOCK_STREAM);
	void Bind(unsigned int port, const char *addr=NULL);
	void Bind(const sockaddr* psa, socklen_t saLen);
	void Listen(int backlog=5);
	// the next three functions return false if the socket is in nonblocking mode
	// and the operation cannot be completed immediately
	bool Connect(const char *addr, unsigned int port);
	bool Connect(const sockaddr* psa, socklen_t saLen);
	bool Accept(Socket& s, sockaddr *psa=NULL, socklen_t *psaLen=NULL);
	void GetSockName(sockaddr *psa, socklen_t *psaLen);
	void GetPeerName(sockaddr *psa, socklen_t *psaLen);
	unsigned int Send(const byte* buf, size_t bufLen, int flags=0);
	unsigned int Receive(byte* buf, size_t bufLen, int flags=0);
	void ShutDown(int how = SD_SEND);

	void IOCtl(long cmd, unsigned long *argp);
	bool SendReady(const timeval *timeout);
	bool ReceiveReady(const timeval *timeout);

	virtual void HandleError(const char *operation) const;
	void CheckAndHandleError_int(const char *operation, int result) const
		{if (result == SOCKET_ERROR) HandleError(operation);}
	void CheckAndHandleError(const char *operation, socket_t result) const
		{if (result == static_cast<socket_t>(SOCKET_ERROR)) HandleError(operation);}
#ifdef USE_WINDOWS_STYLE_SOCKETS
	void CheckAndHandleError(const char *operation, BOOL result) const
		{if (!result) HandleError(operation);}
	void CheckAndHandleError(const char *operation, bool result) const
		{if (!result) HandleError(operation);}
#endif

	//! look up the port number given its name, returns 0 if not found
	static unsigned int PortNameToNumber(const char *name, const char *protocol="tcp");
	//! start Windows Sockets 2
	static void StartSockets();
	//! calls WSACleanup for Windows Sockets
	static void ShutdownSockets();
	//! returns errno or WSAGetLastError
	static int GetLastError();
	//! sets errno or calls WSASetLastError
	static void SetLastError(int errorCode);

protected:
	virtual void SocketChanged() {}

	socket_t m_s;
	bool m_own;
};

class SocketsInitializer
{
public:
	SocketsInitializer() {Socket::StartSockets();}
	~SocketsInitializer() {try {Socket::ShutdownSockets();} catch (const Exception&) {CRYPTOPP_ASSERT(0);}}
};

class SocketReceiver : public NetworkReceiver
{
public:
	SocketReceiver(Socket &s);

#ifdef USE_BERKELEY_STYLE_SOCKETS
	bool MustWaitToReceive() {return true;}
#else
	~SocketReceiver();
	bool MustWaitForResult() {return true;}
#endif
	bool Receive(byte* buf, size_t bufLen);
	unsigned int GetReceiveResult();
	bool EofReceived() const {return m_eofReceived;}

	unsigned int GetMaxWaitObjectCount() const {return 1;}
	void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack);

private:
	Socket &m_s;

#ifdef USE_WINDOWS_STYLE_SOCKETS
	WindowsHandle m_event;
	OVERLAPPED m_overlapped;
	DWORD m_lastResult;
	bool m_resultPending;
#else
	unsigned int m_lastResult;
#endif

	bool m_eofReceived;
};

class SocketSender : public NetworkSender
{
public:
	SocketSender(Socket &s);

#ifdef USE_BERKELEY_STYLE_SOCKETS
	bool MustWaitToSend() {return true;}
#else
	~SocketSender();
	bool MustWaitForResult() {return true;}
	bool MustWaitForEof() { return true; }
	bool EofSent();
#endif
	void Send(const byte* buf, size_t bufLen);
	unsigned int GetSendResult();
	void SendEof();

	unsigned int GetMaxWaitObjectCount() const {return 1;}
	void GetWaitObjects(WaitObjectContainer &container, CallStack const& callStack);

private:
	Socket &m_s;
#ifdef USE_WINDOWS_STYLE_SOCKETS
	WindowsHandle m_event;
	OVERLAPPED m_overlapped;
	DWORD m_lastResult;
	bool m_resultPending;
#else
	unsigned int m_lastResult;
#endif
};

//! socket-based implementation of NetworkSource
class SocketSource : public NetworkSource, public Socket
{
public:
	SocketSource(socket_t s = INVALID_SOCKET, bool pumpAll = false, BufferedTransformation *attachment = NULL)
		: NetworkSource(attachment), Socket(s), m_receiver(*this)
	{
		if (pumpAll)
			PumpAll();
	}

private:
	NetworkReceiver & AccessReceiver() {return m_receiver;}
	SocketReceiver m_receiver;
};

//! socket-based implementation of NetworkSink
class SocketSink : public NetworkSink, public Socket
{
public:
	SocketSink(socket_t s=INVALID_SOCKET, unsigned int maxBufferSize=0, unsigned int autoFlushBound=16*1024)
		: NetworkSink(maxBufferSize, autoFlushBound), Socket(s), m_sender(*this) {}

	void SendEof() {ShutDown(SD_SEND);}

private:
	NetworkSender & AccessSender() {return m_sender;}
	SocketSender m_sender;
};

NAMESPACE_END

#endif	// SOCKETS_AVAILABLE

#endif  // CRYPTOPP_SOCKETFT_H
