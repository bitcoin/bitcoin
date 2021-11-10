// Boost.Signals2 library

// Copyright Frank Mori Hess 2009.
//
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_VARIADIC_SLOT_HPP
#define BOOST_SIGNALS2_VARIADIC_SLOT_HPP

#include <boost/signals2/detail/variadic_arg_type.hpp>

#define BOOST_SIGNALS2_SLOT_N_BINDING_CONSTRUCTORS \
  template<typename A1, typename A2, typename ... BindArgs> \
    slot(const A1 &arg1, const A2 &arg2, const BindArgs & ... args) \
  { \
    init_slot_function(boost::bind(arg1, arg2, args...)); \
  }


#include <boost/signals2/detail/slot_template.hpp>
#endif // BOOST_SIGNALS2_VARIADIC_SLOT_HPP
