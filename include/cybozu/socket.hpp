#pragma once
/**
	@file
	@brief tiny socket class

	@author MITSUNARI Shigeo(@herumi)
	@author MITSUNARI Shigeo
	@remark mingw requires -lws2_32 option
*/
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h> // for socklen_t
	#ifdef _MSC_VER
		#pragma comment(lib, "ws2_32.lib")
		#pragma comment(lib, "iphlpapi.lib")
		#pragma warning(push)
		#pragma warning(disable : 4127) // constant condition
	#endif
#else
	#include <unistd.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <memory.h>
	#include <signal.h>
#endif
#ifndef NDEBUG
	#include <stdio.h>
#endif

#include <cybozu/atomic.hpp>
#include <cybozu/exception.hpp>
#include <cybozu/itoa.hpp>
#include <string>

#ifdef __linux__
//	#define CYBOZU_SOCKET_USE_EPOLL
	#include <sys/epoll.h>
#endif

namespace cybozu {

#ifdef _MSC_VER
struct NetErrorNo : public cybozu::ErrorNo {
	NetErrorNo(NativeErrorNo err)
		: cybozu::ErrorNo(err)
	{
	}
	NetErrorNo()
		: cybozu::ErrorNo(WSAGetLastError())
	{
	}
};
#else
typedef cybozu::ErrorNo NetErrorNo;
#endif

#ifdef CYBOZU_SOCKET_USE_EPOLL

namespace experimental {

struct EpollEvent {
	struct epoll_event ev_;
	EpollEvent()
	{
		memset(&ev_, 0, sizeof(ev_));
	}
	void set(int fd, uint32_t events = EPOLLIN)
	{
		ev_.events = events;
		ev_.data.fd = fd;
	}
	int getFd() const { return ev_.data.fd; }
};

class Epoll {
	int efd_;
	bool verify(const char *msg, int ret, int *err) const {
		if (ret >= 0) return true;
		if (err == 0) throw cybozu::Exception(msg) << cybozu::NetErrorNo();
		*err = errno;
		return false;
	}
public:
	Epoll() : efd_(-1) {}
	bool init(int *err = 0)
	{
		efd_ = ::epoll_create1(0);
		return verify("Epoll:init", efd_, err);
	}
	~Epoll()
	{
		if (efd_ >= 0) ::close(efd_);
	}
	/*
		throw if err == NULL
	*/
	bool ctrl(int op, int fd, EpollEvent *ev, int *err = 0) {
		int ret = ::epoll_ctl(efd_, op, fd, &ev->ev_);
		return verify("Epoll:ctrl", ret, err);
	}
	bool add(int fd, uint32_t events = EPOLLIN, int *err = 0) {
		EpollEvent ev;
		ev.set(fd, events);
		return ctrl(EPOLL_CTL_ADD, fd, &ev, err);
	}
	bool del(int fd, int *err = 0) {
		return ctrl(EPOLL_CTL_DEL, fd, NULL, err);
	}
	/*
		msec : 0 : block
	*/
	int wait(EpollEvent *ev, int maxEv, int msec = 0)
	{
		/*
		 0 : return immediately
		-1 : block indefinitely
		*/
		if (msec == 0) {
			msec = -1;
		} else if (msec == -1) {
			msec = 0;
		}
		int ret = ::epoll_wait(efd_, &ev->ev_, maxEv, msec);
		if (ret == 0) return 0; // timeout
		if (ret < 0) return -errno;
		return ret;
	}
};

struct AutoLock {
	Epoll& ep_;
	int fd_;
	AutoLock(Epoll& ep, int fd, int events = EPOLLIN)
		: ep_(ep)
		, fd_(fd)
	{
		ep_.add(fd, events);
	}
	~AutoLock()
	{
		int err;
		ep_.del(fd_, &err);
	}
};

} // cybozu::experimental
#endif

namespace ssl {
class ClientSocket;
};

namespace socket_local {

#ifdef _WIN32
	typedef SOCKET SocketHandle;
#else
	typedef int SocketHandle;
#endif

struct InitTerm {
	/** call once for init */
	InitTerm()
	{
#ifdef _WIN32
		WSADATA data;
		int err = ::WSAStartup(MAKEWORD(2, 2), &data);
		if (err) {
			fprintf(stderr, "WSAStartup failed : %d\n", err);
			exit(1);
		}
#else
		::signal(SIGPIPE, SIG_IGN);
#endif
	}
	/** call once for term */
	~InitTerm()
	{
#ifdef _WIN32
		::WSACleanup();
#endif
	}
	void dummyCall() { }
};

template<int dummy = 0>
struct InstanceIsHere { static InitTerm it_; };

template<int dummy>
InitTerm InstanceIsHere<dummy>::it_;

struct DummyCall {
	DummyCall() { InstanceIsHere<>::it_.dummyCall(); }
};

} // cybozu::socket_local

class SocketAddr {
	union {
		// http://www.coins.tsukuba.ac.jp/~syspro/2010/No6_files/sockaddr.html
		struct sockaddr sa; /* 16byte */
		struct sockaddr_in v4; /* 16byte */
		struct sockaddr_in6 v6;
	} addr_;
	socklen_t addrlen_;
	int family_;
	friend class Socket;
	void verify() // call in only Socket::accept
	{
		if (addrlen_ == sizeof(addr_.v4) && addr_.sa.sa_family == AF_INET) {
			family_ = AF_INET;
			return;
		}
		if (addrlen_ == sizeof(addr_.v6) && addr_.sa.sa_family == AF_INET6) {
			family_ = AF_INET6;
			return;
		}
		throw cybozu::Exception("cybozu:SocketAddr:verify") << addrlen_;
	}
public:
	SocketAddr()
		: addrlen_(0)
		, family_(0)
	{
	}
	SocketAddr(const std::string& address, uint16_t port, bool forceIpV6 = false)
	{
		set(address, port, forceIpV6);
	}
	void set(const std::string& address, uint16_t port, bool forceIpV6 = false)
	{
		char portStr[16];
		CYBOZU_SNPRINTF(portStr, sizeof(portStr), "%d", port);
		memset(&addr_, 0, sizeof(addr_));
		addrlen_ = 0;
		family_ = 0;

		struct addrinfo *result = 0;
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_NUMERICSERV; // AI_PASSIVE;
		const int s = getaddrinfo(address.c_str(), portStr, &hints, &result);
		// s == EAI_AGAIN
		if (s || forceIpV6) {
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_V4MAPPED;
			if (getaddrinfo(address.c_str(), portStr, &hints, &result)) {
				goto ERR_EXIT;
			}
		}
		{
			bool found = false;
			for (const struct addrinfo *p = result; p; p = p->ai_next) {
				const int family = p->ai_family;
				if (family == hints.ai_family) {
					if (p->ai_addrlen > sizeof(addr_)) {
						break;
					}
					memcpy(&addr_, p->ai_addr, p->ai_addrlen);
					addrlen_ = (socklen_t)p->ai_addrlen;
					family_ = family;
					found = true;
					break;
				}
			}
			freeaddrinfo(result);
			if (found) return;
		}
	ERR_EXIT:
		throw cybozu::Exception("SocketAddr:set") << address << port << cybozu::NetErrorNo();
	}
	socklen_t getSize() const { return addrlen_; }
	int getFamily() const { return family_; }
	const struct sockaddr *get() const { return &addr_.sa; }
	uint16_t getPort() const {
		if (family_ == AF_INET) {
			return ntohs(addr_.v4.sin_port);
		} else if (family_ == AF_INET6) {
			return ntohs(addr_.v6.sin6_port);
		}
		throw cybozu::Exception("SocketAddr:getPort:bad family") << family_;
	}
	// compare addr without port
	bool hasSameAddr(const SocketAddr& rhs) const
	{
		const uint8_t *v4 = 0;
		const uint8_t *v6 = 0;
		if (family_ == AF_INET) {
			if (rhs.family_ == AF_INET) return memcmp(&addr_.v4.sin_addr, &rhs.addr_.v4.sin_addr, sizeof(in_addr)) == 0;
			if (rhs.family_ != AF_INET6) return false;
			v4 = (const uint8_t*)&addr_.v4.sin_addr;
			v6 = (const uint8_t*)&rhs.addr_.v6.sin6_addr;
		} else if (family_ != AF_INET6) {
			return false;
		} else {
			if (rhs.family_ == AF_INET6) return memcmp(&addr_.v6.sin6_addr, &rhs.addr_.v6.sin6_addr, sizeof(in6_addr)) == 0;
			if (rhs.family_ != AF_INET) return false;
			v4 = (const uint8_t*)&rhs.addr_.v4.sin_addr;
			v6 = (const uint8_t*)&addr_.v6.sin6_addr;
		}
		// Ipv6-mapped?
		const uint8_t header[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff };
		return memcmp(v6, header, 12) == 0 && memcmp(v6 + 12, v4, 4) == 0;
	}
	std::string toStr() const
	{
		if (family_ == AF_INET || family_ == AF_INET6) {
			char buf[INET6_ADDRSTRLEN];
			assert(INET_ADDRSTRLEN <= INET6_ADDRSTRLEN);
			const bool isIPv4 = family_ == AF_INET;
			const void *pa = isIPv4 ? (const void*)&addr_.v4.sin_addr : (const void*)&addr_.v6.sin6_addr;
			// not "const void*" because of vc
			const char *p = inet_ntop(family_, const_cast<void*>(pa), buf, sizeof(buf));
			if (!p) throw cybozu::Exception("cybozu:SocketAddr:toStr") << cybozu::NetErrorNo();
			if (isIPv4) return std::string(p) + ':' + cybozu::itoa(getPort());
			return std::string("[") + p + "]:" + cybozu::itoa(getPort());
		}
		throw cybozu::Exception("cybozu:SocketAddr:toStr:bad family_") << family_;
	}
};
/*
	socket class
	@note ower is moved if copied
*/
class Socket {
	friend class cybozu::ssl::ClientSocket;
private:
	cybozu::socket_local::SocketHandle sd_;
	Socket(const Socket&);
	void operator=(const Socket&);
#ifdef WIN32
	void setTimeout(int type, int msec)
	{
		setSocketOption(type, msec);
	}
	/* return msec */
	int getTimeout(int type) const
	{
		return getSocketOption(type);
	}
#else
	void setTimeout(int type, int msec)
	{
		struct timeval t;
		t.tv_sec = msec / 1000;
		t.tv_usec = (msec % 1000) * 1000;
		setSocketOption(type, t);
	}
	/* return msec */
	int getTimeout(int type) const
	{
		struct timeval t;
		getSocketOption(type, &t);
		return t.tv_sec * 1000 + t.tv_usec / 1000; /* msec */
	}
#endif
	void setBlocking(bool isBlocking)
	{
#ifdef _WIN32
		u_long val = isBlocking ? 0 : 1;
		int ret = ::ioctlsocket(sd_, FIONBIO, &val);
#else
		int val = isBlocking ? 0 : 1;
		int ret = ::ioctl(sd_, FIONBIO, &val);
#endif
		if (ret < 0) throw cybozu::Exception("Socket:setBlocking") << cybozu::NetErrorNo() << isBlocking;
	}
public:
#ifndef _WIN32
	static const int INVALID_SOCKET = -1;
#endif
	Socket()
		: sd_(INVALID_SOCKET)
	{
	}

	bool isValid() const { return sd_ != INVALID_SOCKET; }

	// move
#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
	Socket(Socket&& rhs)
		: sd_(INVALID_SOCKET)
	{
		sd_ = cybozu::AtomicExchange(&rhs.sd_, sd_);
	}
#endif
	// close and move
	void moveFrom(Socket& rhs)
	{
		close();
		sd_ = cybozu::AtomicExchange(&rhs.sd_, INVALID_SOCKET);
	}
#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
	void operator=(Socket&& rhs)
#else
	void operator=(Socket& rhs)
#endif
	{
		moveFrom(rhs);
	}

	~Socket()
	{
		close(cybozu::DontThrow);
	}

	bool close(bool dontThrow = false)
	{
		cybozu::socket_local::SocketHandle sd = cybozu::AtomicExchange(&sd_, INVALID_SOCKET);
		if (sd == INVALID_SOCKET) return true;
#ifdef _WIN32
		// ::shutdown(sd, SD_SEND);
		// shutdown is called in closesocket
		bool isOK = ::closesocket(sd) == 0;
#else
		bool isOK = ::close(sd) == 0;
#endif
		if (!dontThrow && !isOK) throw cybozu::Exception("Socket:close") << cybozu::NetErrorNo();
		return isOK;
	}
	/*
		how 0 : SHUTRD ; disallow read
		    1 : SHUT_WR ; disallow write
		    2 : SHUT_RDWR ; disallow read/write
	*/
	bool shutdown(int how, bool dontThrow = false)
	{
		bool isOK = ::shutdown(sd_, how) == 0;
		if (!dontThrow && !isOK) throw cybozu::Exception("Socket:waitForClose:shutdown") << cybozu::NetErrorNo();
		return isOK;
	}
	/*
		send FIN and wait for remote's close().
		this function is used for the following situation.
		sock.write()
		sock.waitForClose()
		sock.close()
	*/
	void waitForClose()
	{
		if (sd_ == INVALID_SOCKET) return;
		//	send FIN and this socket can't write any data.
		shutdown(1);
		// wait for FIN from the peer.
		char buf[1];
		ssize_t readSize = readSome(buf, sizeof(buf));
		if (readSize != 0) {
			throw cybozu::Exception("Socket:waitForClose:readSome:bad size") << readSize;
		}
	}

	/*!
		receive data
		@param buf [out] receive buffer
		@param bufSize [in] receive buffer size(byte)
		@note return read size
	*/
	size_t readSome(void *buf, size_t bufSize)
	{
		int size = (int)(std::min)((size_t)0x7fffffff, bufSize);
#ifdef _WIN32
		int readSize = ::recv(sd_, (char *)buf, size, 0);
#else
	RETRY:
		ssize_t readSize = ::read(sd_, buf, size);
		if (readSize < 0 && errno == EINTR) goto RETRY;
#endif
		if (readSize < 0) throw cybozu::Exception("Socket:readSome") << cybozu::NetErrorNo() << bufSize;
		return readSize;
	}

	/*!
		receive all data unless timeout
		@param buf [out] receive buffer
		@param bufSize [in] receive buffer size(byte)
	*/
	void read(void *buf, size_t bufSize)
	{
		char *p = (char *)buf;
		while (bufSize > 0) {
			size_t readSize = readSome(p, bufSize);
			if (readSize == 0) throw cybozu::Exception("Socket:read:readSize is zero");
			p += readSize;
			bufSize -= readSize;
		}
	}
	/*!
		write all data
		@param buf [out] send buffer
		@param bufSize [in] send buffer size(byte)
	*/
	void write(bool *pb, const void *buf, size_t bufSize)
	{
		const char *p = (const char *)buf;
		while (bufSize > 0) {
			int size = (int)(std::min)(size_t(0x7fffffff), bufSize);
#ifdef _WIN32
			int writeSize = ::send(sd_, p, size, 0);
#else
			int writeSize = ::write(sd_, p, size);
			if (writeSize < 0 && errno == EINTR) continue;
#endif
			if (writeSize < 0) {
				*pb = false;
				return;
			}
			p += writeSize;
			bufSize -= writeSize;
		}
		*pb = true;
	}
	void write(const void *buf, size_t bufSize)
	{
		bool b;
		write(&b, buf, bufSize);
		if (!b) throw cybozu::Exception("Socket:write") << cybozu::NetErrorNo() << bufSize;
	}
	/**
		connect to address:port
		@param address [in] address
		@param port [in] port
		@param msec: 0 : block
	*/
	void connect(const std::string& address, uint16_t port, int msec = 0)
	{
		SocketAddr addr;
		addr.set(address, port);
		connect(addr, msec);
	}
	/**
		connect to resolved socket addr
	*/
	void connect(const cybozu::SocketAddr& addr, int msec = 0)
	{
		if (isValid()) throw cybozu::Exception("Socket:connect:already connect");
		sd_ = ::socket(addr.getFamily(), SOCK_STREAM, IPPROTO_TCP);
		if (!isValid()) {
			throw cybozu::Exception("Socket:connect:socket") << cybozu::NetErrorNo();
		}
		if (msec == 0) {
			if (::connect(sd_, addr.get(), addr.getSize()) < 0) {
				throw cybozu::Exception("Socket:connect") << cybozu::NetErrorNo() << addr.toStr();
			}
		} else {
			setBlocking(false);
			if (::connect(sd_, addr.get(), addr.getSize()) < 0) {
#ifdef _WIN32
				bool inProgress = WSAGetLastError() == WSAEWOULDBLOCK;
#else
				bool inProgress = errno == EINPROGRESS;
#endif
				if (!inProgress) throw cybozu::Exception("Socket:connect:not in progress") << cybozu::NetErrorNo() << addr.toStr();
				if (!queryAccept(msec, false)) throw cybozu::Exception("Socket:connect:timeout") << addr.toStr();
				int err = getSocketOption(SO_ERROR);
				if (err != 0) throw cybozu::Exception("Socket::connect:bad socket") << cybozu::NetErrorNo(err);
			}
			setBlocking(true);
		}
	}

	static const int allowIPv4 = 1;
	static const int allowIPv6 = 2;
	/**
		init for server
		@param port [in] port number
	*/
	void bind(uint16_t port, int mode = allowIPv4 | allowIPv6)
	{
		const int family = (mode & allowIPv6) ? AF_INET6 : AF_INET;
		sd_ = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
		if (!isValid()) {
			throw cybozu::Exception("Socket:bind:socket") << cybozu::NetErrorNo();
		}
		setSocketOption(SO_REUSEADDR, 1);
		struct sockaddr_in6 addr6;
		struct sockaddr_in addr4;
		struct sockaddr *addr;
		socklen_t addrLen;
		if (mode & allowIPv6) {
			setSocketOption(IPV6_V6ONLY, (mode & allowIPv4) ? 0 : 1, IPPROTO_IPV6);
			memset(&addr6, 0, sizeof(addr6));
			addr6.sin6_family = AF_INET6;
			addr6.sin6_port = htons(port);
			addr = (struct sockaddr*)&addr6;
			addrLen = sizeof(addr6);
		} else {
			memset(&addr4, 0, sizeof(addr4));
			addr4.sin_family = AF_INET;
			addr4.sin_port = htons(port);
			addr = (struct sockaddr*)&addr4;
			addrLen = sizeof(addr4);
		}
		if (::bind(sd_, addr, addrLen) == 0) {
			if (::listen(sd_, SOMAXCONN) == 0) {
				return;
			}
		}
		cybozu::NetErrorNo keep;
		close(cybozu::DontThrow);
		throw cybozu::Exception("Socket:bind") << keep;
	}

	/**
		return positive if accepted
		return zero if timeout
		return negative(-errno) if error
	*/
	int queryAcceptNoThrow(int msec = 1000, bool checkWrite = true)
	{
		if (sd_ == INVALID_SOCKET) return -EBADF;
#ifdef CYBOZU_SOCKET_USE_EPOLL
		int err;
		experimental::Epoll ep;
		if (!ep.init(&err)) return -err;
		uint32_t events = checkWrite ? EPOLLIN : EPOLLOUT;
		experimental::AutoLock al(ep, sd_, events);
		experimental::EpollEvent ev;
		int ret = ep.wait(&ev, 1, msec);
		if (ret != 1) return ret;
		assert(ev.getFd() == sd_);
		return ret;
#else
#ifndef _WIN32
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ms739169.aspx
		if (sd_ >= FD_SETSIZE) return -EMFILE;
#endif
		struct timeval timeout;
		timeout.tv_sec = msec / 1000;
		timeout.tv_usec = (msec % 1000) * 1000;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET((unsigned)sd_, &fds);
		int fdNum;
		if (checkWrite) {
			fdNum = ::select((int)sd_ + 1, &fds, 0, 0, &timeout);
		} else {
			fdNum = ::select((int)sd_ + 1, 0, &fds, 0, &timeout);
		}
		if (fdNum < 0) return -errno;
		return fdNum;
#endif
	}
	/**
		return true if acceptable, otherwise false
		return false if one second passed
		while (!server.queryAccept()) {
		}
		client.accept(server);
	*/
	bool queryAccept(int msec = 1000, bool checkWrite = true)
	{
		int ret = queryAcceptNoThrow(msec, checkWrite);
		if (ret < 0) throw cybozu::Exception("Socket:queryAccept") << cybozu::NetErrorNo(-ret);
		return ret > 0;
	}

	/**
		accept for server
	*/
	void accept(Socket& client, SocketAddr *paddr = 0) const
	{
		if (paddr) {
			struct sockaddr *psa = &paddr->addr_.sa;
			paddr->addrlen_ = sizeof(paddr->addr_);
			client.sd_ = ::accept(sd_, psa, &paddr->addrlen_);
			paddr->verify();
		} else {
			client.sd_ = ::accept(sd_, 0, 0);
		}
		if (!client.isValid()) throw cybozu::Exception("Socket:accept") << cybozu::NetErrorNo();
	}

	template<typename T>
	void setSocketOption(int optname, const T& value, int level = SOL_SOCKET)
	{
		bool isOK = setsockopt(sd_, level, optname, cybozu::cast<const char*>(&value), sizeof(T)) == 0;
		if (!isOK) throw cybozu::Exception("Socket:setSocketOption") << cybozu::NetErrorNo();
	}
	template<typename T>
	void getSocketOption(int optname, T* value, int level = SOL_SOCKET) const
	{
		socklen_t len = (socklen_t)sizeof(T);
		bool isOK = getsockopt(sd_, level, optname, cybozu::cast<char*>(value), &len) == 0;
		if (!isOK) throw cybozu::Exception("Socket:getSocketOption") << cybozu::NetErrorNo();
	}
	int getSocketOption(int optname) const
	{
		int ret;
		getSocketOption(optname, &ret);
		return ret;
	}
	/**
		setup linger
	*/
	void setLinger(uint16_t l_onoff, uint16_t l_linger)
	{
		struct linger linger;
		linger.l_onoff = l_onoff;
		linger.l_linger = l_linger;
		setSocketOption(SO_LINGER, &linger);
	}
	/**
		get receive buffer size
		@retval positive buffer size(byte)
		@retval -1 error
	*/
	int getReceiveBufferSize() const
	{
		return getSocketOption(SO_RCVBUF);
	}
	/**
		set receive buffer size
		@param size [in] buffer size(byte)
	*/
	void setReceiveBufferSize(int size)
	{
		setSocketOption(SO_RCVBUF, size);
	}
	/**
		get send buffer size
		@retval positive buffer size(byte)
		@retval -1 error
	*/
	int getSendBufferSize() const
	{
		return getSocketOption(SO_SNDBUF);
	}
	/**
		sed send buffer size
		@param size [in] buffer size(byte)
	*/
	void setSendBufferSize(int size)
	{
		setSocketOption(SO_SNDBUF, size);
	}
	/**
		set send timeout
		@param msec [in] msec
	*/
	void setSendTimeout(int msec)
	{
		setTimeout(SO_SNDTIMEO, msec);
	}
	/**
		set receive timeout
		@param msec [in] msec
	*/
	void setReceiveTimeout(int msec)
	{
		setTimeout(SO_RCVTIMEO, msec);
	}
	/**
		get send timeout(msec)
	*/
	int getSendTimeout() const
	{
		return getTimeout(SO_SNDTIMEO);
	}
	/**
		get receive timeout(msec)
	*/
	int getReceiveTimeout() const
	{
		return getTimeout(SO_RCVTIMEO);
	}
};

} // cybozu

#ifdef _MSC_VER
	#pragma warning(pop)
#endif
