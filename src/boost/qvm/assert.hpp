/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_ASSERT
#	ifdef BOOST_ASSERT
#		define BOOST_QVM_ASSERT BOOST_ASSERT
#	else
#		include <cassert>
#		define BOOST_QVM_ASSERT assert
#	endif
#endif
