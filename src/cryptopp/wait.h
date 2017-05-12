// wait.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_WAIT_H
#define CRYPTOPP_WAIT_H

#include "config.h"

#if !defined(NO_OS_DEPENDENCE) && (defined(SOCKETS_AVAILABLE) || defined(WINDOWS_PIPES_AVAILABLE))

#include "cryptlib.h"
#include "misc.h"
#include "stdcpp.h"

#ifdef USE_WINDOWS_STYLE_SOCKETS
#include <winsock2.h>
#else
#include <sys/types.h>
#include <sys/select.h>
#endif

// For defintions of VOID, PVOID, HANDLE, PHANDLE, etc.
#if defined(CRYPTOPP_WIN32_AVAILABLE)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "hrtimer.h"

#if defined(__has_feature)
# if __has_feature(memory_sanitizer)
#  define CRYPTOPP_MSAN 1
# endif
#endif

// http://connect.microsoft.com/VisualStudio/feedback/details/1581706
//   and http://github.com/weidai11/cryptopp/issues/214
#if CRYPTOPP_MSC_VERSION == 1900
# pragma warning(push)
# pragma warning(disable: 4589)
#endif

NAMESPACE_BEGIN(CryptoPP)

class Tracer
{
public:
	Tracer(unsigned int level) : m_level(level) {}
	virtual ~Tracer() {}

protected:
	//! Override this in your most-derived tracer to do the actual tracing.
	virtual void Trace(unsigned int n, std::string const& s) = 0;

	/*! By default, tracers will decide which trace messages to trace according to a trace level
		mechanism. If your most-derived tracer uses a different mechanism, override this to
		return false. If this method returns false, the default TraceXxxx(void) methods will all
		return 0 and must be overridden explicitly by your tracer for trace messages you want. */
	virtual bool UsingDefaults() const { return true; }

protected:
	unsigned int m_level;

	void TraceIf(unsigned int n, std::string const&s)
		{ if (n) Trace(n, s); }

	/*! Returns nr if, according to the default log settings mechanism (using log levels),
	    the message should be traced. Returns 0 if the default trace level mechanism is not
		in use, or if it is in use but the event should not be traced. Provided as a utility
		method for easier and shorter coding of default TraceXxxx(void) implementations. */
	unsigned int Tracing(unsigned int nr, unsigned int minLevel) const
		{ return (UsingDefaults() && m_level >= minLevel) ? nr : 0; }
};

// Your Tracer-derived class should inherit as virtual public from Tracer or another
// Tracer-derived class, and should pass the log level in its constructor. You can use the
// following methods to begin and end your Tracer definition.

// This constructor macro initializes Tracer directly even if not derived directly from it;
// this is intended, virtual base classes are always initialized by the most derived class.
#define CRYPTOPP_TRACER_CONSTRUCTOR(DERIVED) \
	public: DERIVED(unsigned int level = 0) : Tracer(level) {}

#define CRYPTOPP_BEGIN_TRACER_CLASS_1(DERIVED, BASE1) \
	class DERIVED : virtual public BASE1, public NotCopyable { CRYPTOPP_TRACER_CONSTRUCTOR(DERIVED)

#define CRYPTOPP_BEGIN_TRACER_CLASS_2(DERIVED, BASE1, BASE2) \
	class DERIVED : virtual public BASE1, virtual public BASE2, public NotCopyable { CRYPTOPP_TRACER_CONSTRUCTOR(DERIVED)

#define CRYPTOPP_END_TRACER_CLASS };

// In your Tracer-derived class, you should define a globally unique event number for each
// new event defined. This can be done using the following macros.

#define CRYPTOPP_BEGIN_TRACER_EVENTS(UNIQUENR)	enum { EVENTBASE = UNIQUENR,
#define CRYPTOPP_TRACER_EVENT(EVENTNAME)				EventNr_##EVENTNAME,
#define CRYPTOPP_END_TRACER_EVENTS				};

// In your own Tracer-derived class, you must define two methods per new trace event type:
// - unsigned int TraceXxxx() const
//   Your default implementation of this method should return the event number if according
//   to the default trace level system the event should be traced, or 0 if it should not.
// - void TraceXxxx(string const& s)
//   This method should call TraceIf(TraceXxxx(), s); to do the tracing.
// For your convenience, a macro to define these two types of methods are defined below.
// If you use this macro, you should also use the TRACER_EVENTS macros above to associate
// event names with numbers.

#define CRYPTOPP_TRACER_EVENT_METHODS(EVENTNAME, LOGLEVEL) \
	virtual unsigned int Trace##EVENTNAME() const { return Tracing(EventNr_##EVENTNAME, LOGLEVEL); } \
	virtual void Trace##EVENTNAME(std::string const& s) { TraceIf(Trace##EVENTNAME(), s); }


/*! A simple unidirectional linked list with m_prev == 0 to indicate the final entry.
    The aim of this implementation is to provide a very lightweight and practical
	tracing mechanism with a low performance impact. Functions and methods supporting
	this call-stack mechanism would take a parameter of the form "CallStack const& callStack",
	and would pass this parameter to subsequent functions they call using the construct:

	SubFunc(arg1, arg2, CallStack("my func at place such and such", &callStack));

	The advantage of this approach is that it is easy to use and should be very efficient,
	involving no allocation from the heap, just a linked list of stack objects containing
	pointers to static ASCIIZ strings (or possibly additional but simple data if derived). */
class CallStack
{
public:
	CallStack(char const* i, CallStack const* p) : m_info(i), m_prev(p) {}
	CallStack const* Prev() const { return m_prev; }
	virtual std::string Format() const;

protected:
	char const* m_info;
	CallStack const* m_prev;
};

/*! An extended CallStack entry type with an additional numeric parameter. */
class CallStackWithNr : public CallStack
{
public:
	CallStackWithNr(char const* i, word32 n, CallStack const* p) : CallStack(i, p), m_nr(n) {}
	std::string Format() const;

protected:
	word32 m_nr;
};

/*! An extended CallStack entry type with an additional string parameter. */
class CallStackWithStr : public CallStack
{
public:
	CallStackWithStr(char const* i, char const* z, CallStack const* p) : CallStack(i, p), m_z(z) {}
	std::string Format() const;

protected:
	char const* m_z;
};

// Thanks to Maximilian Zamorsky for help with http://connect.microsoft.com/VisualStudio/feedback/details/1570496/
CRYPTOPP_BEGIN_TRACER_CLASS_1(WaitObjectsTracer, Tracer)
	CRYPTOPP_BEGIN_TRACER_EVENTS(0x48752841)
		CRYPTOPP_TRACER_EVENT(NoWaitLoop)
	CRYPTOPP_END_TRACER_EVENTS
	CRYPTOPP_TRACER_EVENT_METHODS(NoWaitLoop, 1)
CRYPTOPP_END_TRACER_CLASS

struct WaitingThreadData;

//! container of wait objects
class WaitObjectContainer : public NotCopyable
{
public:
	//! exception thrown by WaitObjectContainer
	class Err : public Exception
	{
	public:
		Err(const std::string& s) : Exception(IO_ERROR, s) {}
	};

	static unsigned int MaxWaitObjects();

	WaitObjectContainer(WaitObjectsTracer* tracer = 0);

	void Clear();
	void SetNoWait(CallStack const& callStack);
	void ScheduleEvent(double milliseconds, CallStack const& callStack);
	// returns false if timed out
	bool Wait(unsigned long milliseconds);

#ifdef USE_WINDOWS_STYLE_SOCKETS
# ifndef CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562
	virtual ~WaitObjectContainer();
# else
	~WaitObjectContainer();
#endif
	void AddHandle(HANDLE handle, CallStack const& callStack);
#else
	void AddReadFd(int fd, CallStack const& callStack);
	void AddWriteFd(int fd, CallStack const& callStack);
#endif

private:
	WaitObjectsTracer* m_tracer;

#ifdef USE_WINDOWS_STYLE_SOCKETS
	void CreateThreads(unsigned int count);
	std::vector<HANDLE> m_handles;
	std::vector<WaitingThreadData *> m_threads;
	HANDLE m_startWaiting;
	HANDLE m_stopWaiting;
#else
	fd_set m_readfds, m_writefds;
	int m_maxFd;
#endif
	double m_firstEventTime;
	Timer m_eventTimer;
	bool m_noWait;

#ifdef USE_WINDOWS_STYLE_SOCKETS
	typedef size_t LastResultType;
#else
	typedef int LastResultType;
#endif
	enum { LASTRESULT_NOWAIT = -1, LASTRESULT_SCHEDULED = -2, LASTRESULT_TIMEOUT = -3 };
	LastResultType m_lastResult;
	unsigned int m_sameResultCount;
	Timer m_noWaitTimer;
	void SetLastResult(LastResultType result);
	void DetectNoWait(LastResultType result, CallStack const& callStack);
};

NAMESPACE_END

#if CRYPTOPP_MSC_VERSION == 1900
# pragma warning(pop)
#endif

#endif

#endif
