/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_THROW_EXCEPTION

#	define BOOST_QVM_THROW_EXCEPTION ::boost::qvm::throw_exception

#	include <exception>

#	ifndef BOOST_QVM_NO_EXCEPTIONS
#   	if defined(__clang__) && !defined(__ibmxl__) // Clang C++ emulates GCC, so it has to appear early.
#       	if !__has_feature(cxx_exceptions)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__DMC__) // Digital Mars C++
#       	if !defined(_CPPUNWIND)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__GNUC__) && !defined(__ibmxl__) // GNU C++:
#       	if !defined(__EXCEPTIONS)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__KCC) // Kai C++
#       	if !defined(_EXCEPTIONS)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__CODEGEARC__) // CodeGear - must be checked for before Borland
#       	if !defined(_CPPUNWIND) && !defined(__EXCEPTIONS)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__BORLANDC__) // Borland
#       	if !defined(_CPPUNWIND) && !defined(__EXCEPTIONS)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__MWERKS__) // Metrowerks CodeWarrior
#       	if !__option(exceptions)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__IBMCPP__) && defined(__COMPILER_VER__) && defined(__MVS__) // IBM z/OS XL C/C++
#       	if !defined(_CPPUNWIND) && !defined(__EXCEPTIONS)
#           define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(__ibmxl__) // IBM XL C/C++ for Linux (Little Endian)
#       	if !__has_feature(cxx_exceptions)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	elif defined(_MSC_VER) // Microsoft Visual C++
			// Must remain the last #elif since some other vendors (Metrowerks, for
			// example) also #define _MSC_VER
#       	if !defined(_CPPUNWIND)
#           	define BOOST_QVM_NO_EXCEPTIONS
#       	endif
#   	endif
#	endif

////////////////////////////////////////

#	ifdef BOOST_NORETURN
#   	define BOOST_QVM_NORETURN BOOST_NORETURN
#	else
#   	if defined(_MSC_VER)
#       	define BOOST_QVM_NORETURN __declspec(noreturn)
#   	elif defined(__GNUC__)
#       	define BOOST_QVM_NORETURN __attribute__ ((__noreturn__))
#   	elif defined(__has_attribute) && defined(__SUNPRO_CC) && (__SUNPRO_CC > 0x5130)
#       	if __has_attribute(noreturn)
#           	define BOOST_QVM_NORETURN [[noreturn]]
#       	endif
#   	elif defined(__has_cpp_attribute)
#       	if __has_cpp_attribute(noreturn)
#           	define BOOST_QVM_NORETURN [[noreturn]]
#       	endif
#   	endif
#	endif

#	if !defined(BOOST_QVM_NORETURN)
#  		define BOOST_QVM_NORETURN
#	endif

////////////////////////////////////////

#	ifdef BOOST_QVM_NO_EXCEPTIONS

namespace boost
{
    BOOST_QVM_NORETURN void throw_exception( std::exception const & ); // user defined
}

namespace boost { namespace qvm {

    template <class T>
    BOOST_QVM_NORETURN void throw_exception( T const & e )
    {
        ::boost::throw_exception(e);
    }

} }

#	else

namespace boost { namespace qvm {

    template <class T>
    BOOST_QVM_NORETURN void throw_exception( T const & e )
    {
        throw e;
    }

} }

#	endif

#endif
