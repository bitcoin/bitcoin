//
// detail/push_options.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// No header guard

#if defined(__COMO__)

// Comeau C++

#elif defined(__DMC__)

// Digital Mars C++

#elif defined(__INTEL_COMPILER) || defined(__ICL) \
  || defined(__ICC) || defined(__ECC)

// Intel C++

# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)
#  if !defined(BOOST_ASIO_DISABLE_VISIBILITY)
#   pragma GCC visibility push (default)
#  endif // !defined(BOOST_ASIO_DISABLE_VISIBILITY)
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)

#elif defined(__clang__)

// Clang

# if defined(__OBJC__)
#  if !defined(__APPLE_CC__) || (__APPLE_CC__ <= 1)
#   if !defined(BOOST_ASIO_DISABLE_OBJC_WORKAROUND)
#    if !defined(Protocol) && !defined(id)
#     define Protocol cpp_Protocol
#     define id cpp_id
#     define BOOST_ASIO_OBJC_WORKAROUND
#    endif
#   endif
#  endif
# endif

# if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32)
#  if !defined(BOOST_ASIO_DISABLE_VISIBILITY)
#   pragma GCC visibility push (default)
#  endif // !defined(BOOST_ASIO_DISABLE_VISIBILITY)
# endif // !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32)

# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# if (__clang_major__ >= 6)
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
# endif // (__clang_major__ >= 6)

# pragma push_macro ("emit")
# undef emit

# pragma push_macro ("signal")
# undef signal

# pragma push_macro ("slot")
# undef slot

#elif defined(__GNUC__)

// GNU C++

# if defined(__MINGW32__) || defined(__CYGWIN__)
#  pragma pack (push, 8)
# endif

# if defined(__OBJC__)
#  if !defined(__APPLE_CC__) || (__APPLE_CC__ <= 1)
#   if !defined(BOOST_ASIO_DISABLE_OBJC_WORKAROUND)
#    if !defined(Protocol) && !defined(id)
#     define Protocol cpp_Protocol
#     define id cpp_id
#     define BOOST_ASIO_OBJC_WORKAROUND
#    endif
#   endif
#  endif
# endif

# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)
#  if !defined(BOOST_ASIO_DISABLE_VISIBILITY)
#   pragma GCC visibility push (default)
#  endif // !defined(BOOST_ASIO_DISABLE_VISIBILITY)
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || (__GNUC__ > 4)

# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
# if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4)
#  pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
# endif // (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || (__GNUC__ > 4)
# if (__GNUC__ >= 7)
#  pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
# endif // (__GNUC__ >= 7)

# pragma push_macro ("emit")
# undef emit

# pragma push_macro ("signal")
# undef signal

# pragma push_macro ("slot")
# undef slot

#elif defined(__KCC)

// Kai C++

#elif defined(__sgi)

// SGI MIPSpro C++

#elif defined(__DECCXX)

// Compaq Tru64 Unix cxx

#elif defined(__ghs)

// Greenhills C++

#elif defined(__BORLANDC__) && !defined(__clang__)

// Borland C++

# pragma option push -a8 -b -Ve- -Vx- -w-inl -vi-
# pragma nopushoptwarn
# pragma nopackwarning
# if !defined(__MT__)
#  error Multithreaded RTL must be selected.
# endif // !defined(__MT__)

#elif defined(__MWERKS__)

// Metrowerks CodeWarrior

#elif defined(__SUNPRO_CC)

// Sun Workshop Compiler C++

#elif defined(__HP_aCC)

// HP aCC

#elif defined(__MRC__) || defined(__SC__)

// MPW MrCpp or SCpp

#elif defined(__IBMCPP__)

// IBM Visual Age

#elif defined(_MSC_VER)

// Microsoft Visual C++
//
// Must remain the last #elif since some other vendors (Metrowerks, for example)
// also #define _MSC_VER

# pragma warning (disable:4103)
# pragma warning (push)
# pragma warning (disable:4127)
# pragma warning (disable:4180)
# pragma warning (disable:4244)
# pragma warning (disable:4355)
# pragma warning (disable:4510)
# pragma warning (disable:4512)
# pragma warning (disable:4610)
# pragma warning (disable:4675)
# if (_MSC_VER < 1600)
// Visual Studio 2008 generates spurious warnings about unused parameters.
#  pragma warning (disable:4100)
# endif // (_MSC_VER < 1600)
# if defined(_M_IX86) && defined(_Wp64)
// The /Wp64 option is broken. If you want to check 64 bit portability, use a
// 64 bit compiler!
#  pragma warning (disable:4311)
#  pragma warning (disable:4312)
# endif // defined(_M_IX86) && defined(_Wp64)
# pragma pack (push, 8)
// Note that if the /Og optimisation flag is enabled with MSVC6, the compiler
// has a tendency to incorrectly optimise away some calls to member template
// functions, even though those functions contain code that should not be
// optimised away! Therefore we will always disable this optimisation option
// for the MSVC6 compiler.
# if (_MSC_VER < 1300)
#  pragma optimize ("g", off)
# endif
# if !defined(_MT)
#  error Multithreaded RTL must be selected.
# endif // !defined(_MT)

# if defined(__cplusplus_cli) || defined(__cplusplus_winrt)
#  if !defined(BOOST_ASIO_DISABLE_CLR_WORKAROUND)
#   if !defined(generic)
#    define generic cpp_generic
#    define BOOST_ASIO_CLR_WORKAROUND
#   endif
#  endif
# endif

# pragma push_macro ("emit")
# undef emit

# pragma push_macro ("signal")
# undef signal

# pragma push_macro ("slot")
# undef slot

#endif
