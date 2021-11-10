//
//  boost/signals2/detail/lwm_nop.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//  Copyright (c) 2008 Frank Mori Hess
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_SIGNALS2_LWM_NOP_HPP
#define BOOST_SIGNALS2_LWM_NOP_HPP

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif


#include <boost/signals2/dummy_mutex.hpp>

namespace boost
{

namespace signals2
{

class mutex: public dummy_mutex
{
};

} // namespace signals2

} // namespace boost

#endif // #ifndef BOOST_SIGNALS2_LWM_NOP_HPP
