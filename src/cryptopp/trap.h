// trap.h - written and placed in public domain by Jeffrey Walton.
//          Copyright assigned to Crypto++ project

//! \file trap.h
//! \brief Debugging and diagnostic assertions
//! \details <tt>CRYPTOPP_ASSERT</tt> is the library's debugging and diagnostic assertion. <tt>CRYPTOPP_ASSERT</tt>
//!   is enabled by <tt>CRYPTOPP_DEBUG</tt>, <tt>DEBUG</tt> or <tt>_DEBUG</tt>.
//! \details <tt>CRYPTOPP_ASSERT</tt> raises a <tt>SIGTRAP</tt> (Unix) or calls <tt>__debugbreak()</tt> (Windows).
//!   <tt>CRYPTOPP_ASSERT</tt> is only in effect when the user requests a debug configuration. Unlike Posix assert,
//!   <tt>NDEBUG</tt> (or failure to define it) does not affect the library.
//! The traditional Posix define <tt>NDEBUG</tt> has no effect on <tt>CRYPTOPP_DEBUG</tt> or DebugTrapHandler.
//! \since Crypto++ 5.6.5
//! \sa DebugTrapHandler, <A HREF="http://github.com/weidai11/cryptopp/issues/277">Issue 277</A>,
//!   <A HREF="http://seclists.org/oss-sec/2016/q3/520">CVE-2016-7420</A>

#ifndef CRYPTOPP_TRAP_H
#define CRYPTOPP_TRAP_H

#include "config.h"

#if CRYPTOPP_DEBUG
#  include <iostream>
#  include <sstream>
#  if defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE)
#    include "ossig.h"
#  elif defined(CRYPTOPP_WIN32_AVAILABLE)
#    if (_MSC_VER >= 1400)
#      include <intrin.h>
#    endif
#  endif
#endif // CRYPTOPP_DEBUG

// ************** run-time assertion ***************

#if defined(CRYPTOPP_DOXYGEN_PROCESSING)
//! \brief Debugging and diagnostic assertion
//! \details <tt>CRYPTOPP_ASSERT</tt> is the library's debugging and diagnostic assertion. <tt>CRYPTOPP_ASSERT</tt>
//!   is enabled by the preprocessor macros <tt>CRYPTOPP_DEBUG</tt>, <tt>DEBUG</tt> or <tt>_DEBUG</tt>.
//! \details <tt>CRYPTOPP_ASSERT</tt> raises a <tt>SIGTRAP</tt> (Unix) or calls <tt>DebugBreak()</tt> (Windows).
//!   <tt>CRYPTOPP_ASSERT</tt> is only in effect when the user explictly requests a debug configuration.
//! \details If you want to ensure <tt>CRYPTOPP_ASSERT</tt> is inert, then <em>do not</em> define
//!   <tt>CRYPTOPP_DEBUG</tt>, <tt>DEBUG</tt> or <tt>_DEBUG</tt>. Avoiding the defines means <tt>CRYPTOPP_ASSERT</tt>
//!   is processed into <tt>((void)(exp))</tt>.
//! \details The traditional Posix define <tt>NDEBUG</tt> has no effect on <tt>CRYPTOPP_DEBUG</tt>, <tt>CRYPTOPP_ASSERT</tt>
//!   or DebugTrapHandler.
//! \details An example of using \ref CRYPTOPP_ASSERT "CRYPTOPP_ASSERT" and DebugTrapHandler is shown below. The library's
//!   test program, <tt>cryptest.exe</tt> (from test.cpp), exercises the structure:
//!  <pre>
//!    #if CRYPTOPP_DEBUG && (defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))
//!    static const DebugTrapHandler g_dummyHandler;
//!    #endif
//!
//!    int main(int argc, char* argv[])
//!    {
//!       CRYPTOPP_ASSERT(argv != nullptr);
//!       ...
//!    }
//!  </pre>
//! \since Crypto++ 5.6.5
//! \sa DebugTrapHandler, SignalHandler, <A HREF="http://github.com/weidai11/cryptopp/issues/277">Issue 277</A>,
//!   <A HREF="http://seclists.org/oss-sec/2016/q3/520">CVE-2016-7420</A>
#  define CRYPTOPP_ASSERT(exp) { ... }
#endif

#if CRYPTOPP_DEBUG && (defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))
#  define CRYPTOPP_ASSERT(exp) {                                  \
    if (!(exp)) {                                                 \
      std::ostringstream oss;                                     \
      oss << "Assertion failed: " << (char*)(__FILE__) << "("     \
          << (int)(__LINE__) << "): " << (char*)(__func__)        \
          << std::endl;                                           \
      std::cerr << oss.str();                                     \
      raise(SIGTRAP);                                             \
    }                                                             \
  }
#elif CRYPTOPP_DEBUG && defined(CRYPTOPP_WIN32_AVAILABLE)
#  define CRYPTOPP_ASSERT(exp) {                                  \
    if (!(exp)) {                                                 \
      std::ostringstream oss;                                     \
      oss << "Assertion failed: " << (char*)(__FILE__) << "("     \
          << (int)(__LINE__) << "): " << (char*)(__FUNCTION__)    \
          << std::endl;                                           \
      std::cerr << oss.str();                                     \
      __debugbreak();                                             \
    }                                                             \
  }
#endif // DEBUG and Unix or Windows

// Remove CRYPTOPP_ASSERT in non-debug builds.
//  Can't use CRYPTOPP_UNUSED due to circular dependency
#ifndef CRYPTOPP_ASSERT
#  define CRYPTOPP_ASSERT(exp) ((void)(exp))
#endif

NAMESPACE_BEGIN(CryptoPP)

// ************** SIGTRAP handler ***************

#if (CRYPTOPP_DEBUG && (defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))) || defined(CRYPTOPP_DOXYGEN_PROCESSING)
//! \brief Default SIGTRAP handler
//! \details DebugTrapHandler() can be used by a program to install an empty SIGTRAP handler. If present,
//!   the handler ensures there is a signal handler in place for <tt>SIGTRAP</tt> raised by
//!   <tt>CRYPTOPP_ASSERT</tt>. If <tt>CRYPTOPP_ASSERT</tt> raises <tt>SIGTRAP</tt> <em>without</em>
//!   a handler, then one of two things can occur. First, the OS might allow the program
//!   to continue. Second, the OS might terminate the program. OS X allows the program to continue, while
//!   some Linuxes terminate the program.
//! \details If DebugTrapHandler detects another handler in place, then it will not install a handler. This
//!   ensures a debugger can gain control of the <tt>SIGTRAP</tt> signal without contention. It also allows multiple
//!   DebugTrapHandler to be created without contentious or unusual behavior. Though muliple DebugTrapHandler can be
//!   created, a program should only create one, if needed.
//! \details A DebugTrapHandler is subject to C++ static initialization [dis]order. If you need to install a handler
//!   and it must be installed early, then reference the code associated with <tt>CRYPTOPP_INIT_PRIORITY</tt> in
//!   cryptlib.cpp and cpu.cpp.
//! \details If you want to ensure <tt>CRYPTOPP_ASSERT</tt> is inert, then <em>do not</em> define
//!   <tt>CRYPTOPP_DEBUG</tt>, <tt>DEBUG</tt> or <tt>_DEBUG</tt>. Avoiding the defines means <tt>CRYPTOPP_ASSERT</tt>
//!   is processed into <tt>((void)(exp))</tt>.
//! \details The traditional Posix define <tt>NDEBUG</tt> has no effect on <tt>CRYPTOPP_DEBUG</tt>, <tt>CRYPTOPP_ASSERT</tt>
//!   or DebugTrapHandler.
//! \details An example of using \ref CRYPTOPP_ASSERT "CRYPTOPP_ASSERT" and DebugTrapHandler is shown below. The library's
//!   test program, <tt>cryptest.exe</tt> (from test.cpp), exercises the structure:
//!  <pre>
//!    #if CRYPTOPP_DEBUG && (defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE))
//!    static const DebugTrapHandler g_dummyHandler;
//!    #endif
//!
//!    int main(int argc, char* argv[])
//!    {
//!       CRYPTOPP_ASSERT(argv != nullptr);
//!       ...
//!    }
//!  </pre>
//! \since Crypto++ 5.6.5
//! \sa \ref CRYPTOPP_ASSERT "CRYPTOPP_ASSERT", SignalHandler, <A HREF="http://github.com/weidai11/cryptopp/issues/277">Issue 277</A>,
//!   <A HREF="http://seclists.org/oss-sec/2016/q3/520">CVE-2016-7420</A>

#if defined(CRYPTOPP_DOXYGEN_PROCESSING)
class DebugTrapHandler : public SignalHandler<SIGILL, false> { };
#else
typedef SignalHandler<SIGILL, false> DebugTrapHandler;
#endif

#endif  // Linux, Unix and Documentation

NAMESPACE_END

#endif // CRYPTOPP_TRAP_H
