//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2012,2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURES_FUTURE_ERROR_CODE_HPP
#define BOOST_THREAD_FUTURES_FUTURE_ERROR_CODE_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/system/error_code.hpp>
#include <boost/type_traits/integral_constant.hpp>

namespace boost
{

  //enum class future_errc
  BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_errc)
  {
      broken_promise = 1,
      future_already_retrieved,
      promise_already_satisfied,
      no_state
  }
  BOOST_SCOPED_ENUM_DECLARE_END(future_errc)

  namespace system
  {
    template <>
    struct BOOST_SYMBOL_VISIBLE is_error_code_enum< ::boost::future_errc> : public true_type {};

    #ifdef BOOST_NO_CXX11_SCOPED_ENUMS
    template <>
    struct BOOST_SYMBOL_VISIBLE is_error_code_enum< ::boost::future_errc::enum_type> : public true_type { };
    #endif
  } // system

  BOOST_THREAD_DECL
  const system::error_category& future_category() BOOST_NOEXCEPT;

  namespace system
  {
    inline
    error_code
    make_error_code(future_errc e) BOOST_NOEXCEPT
    {
        return error_code(underlying_cast<int>(e), boost::future_category());
    }

    inline
    error_condition
    make_error_condition(future_errc e) BOOST_NOEXCEPT
    {
        return error_condition(underlying_cast<int>(e), boost::future_category());
    }
  } // system
} // boost

#endif // header
