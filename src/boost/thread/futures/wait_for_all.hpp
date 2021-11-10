//  (C) Copyright 2008-10 Anthony Williams
//  (C) Copyright 2011-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURES_WAIT_FOR_ALL_HPP
#define BOOST_THREAD_FUTURES_WAIT_FOR_ALL_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/futures/is_future_type.hpp>

#include <boost/core/enable_if.hpp>

namespace boost
{
  template<typename Iterator>
  typename boost::disable_if<is_future_type<Iterator>,void>::type wait_for_all(Iterator begin,Iterator end)
  {
      for(Iterator current=begin;current!=end;++current)
      {
          current->wait();
      }
  }

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename F1,typename F2>
    typename boost::enable_if<is_future_type<F1>,void>::type wait_for_all(F1& f1,F2& f2)
    {
        f1.wait();
        f2.wait();
    }

    template<typename F1,typename F2,typename F3>
    void wait_for_all(F1& f1,F2& f2,F3& f3)
    {
        f1.wait();
        f2.wait();
        f3.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4>
    void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4)
    {
        f1.wait();
        f2.wait();
        f3.wait();
        f4.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4,typename F5>
    void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
    {
        f1.wait();
        f2.wait();
        f3.wait();
        f4.wait();
        f5.wait();
    }
#else
    template<typename F1, typename... Fs>
    typename boost::enable_if<is_future_type<F1>,void>::type wait_for_all(F1& f1, Fs&... fs)
    {
        bool dummy[] = { (f1.wait(), true), (fs.wait(), true)... };

        // prevent unused parameter warning
        (void) dummy;
    }
#endif // !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)}

}

#endif // header
