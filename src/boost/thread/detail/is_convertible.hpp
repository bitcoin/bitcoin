//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011-2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_DETAIL_IS_CONVERTIBLE_HPP
#define BOOST_THREAD_DETAIL_IS_CONVERTIBLE_HPP

#include <boost/type_traits/is_convertible.hpp>
#include <boost/thread/detail/move.hpp>

namespace boost
{
  namespace thread_detail
  {
    template <typename T1, typename T2>
    struct is_convertible : boost::is_convertible<T1,T2> {};

#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES

#if defined(BOOST_INTEL_CXX_VERSION) && (BOOST_INTEL_CXX_VERSION <= 1300)

#if defined BOOST_THREAD_USES_MOVE
    template <typename T1, typename T2>
    struct is_convertible<
      rv<T1> &,
      rv<rv<T2> > &
    > : false_type {};
#endif

#elif defined __GNUC__ && (__GNUC__ < 4 || ( __GNUC__ == 4 && __GNUC_MINOR__ <= 4 ))

    template <typename T1, typename T2>
    struct is_convertible<T1&, T2&> : boost::is_convertible<T1, T2> {};
#endif

#endif
  }

} // namespace boost


#endif //  BOOST_THREAD_DETAIL_MEMORY_HPP
