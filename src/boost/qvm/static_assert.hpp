/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_STATIC_ASSERT

#	if __cplusplus >= 201103L

#		include <utility>
#		define BOOST_QVM_STATIC_ASSERT(condition) static_assert(condition, "Boost QVM static assertion failure")

#	else

#		ifdef __GNUC__
#			define BOOST_QVM_ATTRIBUTE_UNUSED __attribute__((unused))
#		else
#			define BOOST_QVM_ATTRIBUTE_UNUSED
#		endif

#		define BOOST_QVM_TOKEN_PASTE(x, y) x ## y
#		define BOOST_QVM_TOKEN_PASTE2(x, y) BOOST_QVM_TOKEN_PASTE(x, y)
#		define BOOST_QVM_STATIC_ASSERT(condition) typedef char BOOST_QVM_TOKEN_PASTE2(boost_qvm_static_assert_failure_,__LINE__)[(condition)?1:-1] BOOST_QVM_ATTRIBUTE_UNUSED

#	endif

#endif
