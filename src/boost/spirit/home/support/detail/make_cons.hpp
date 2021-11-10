/*=============================================================================
  Copyright (c) 2001-2011 Joel de Guzman
  http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_MAKE_CONS_OCTOBER_16_2008_1252PM
#define BOOST_SPIRIT_MAKE_CONS_OCTOBER_16_2008_1252PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/eval_if.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost { namespace spirit { namespace detail
{
    template <typename T>
    struct as_meta_element
      : mpl::eval_if_c<is_abstract<T>::value || is_function<T>::value
          , add_reference<T>, remove_const<T> >
    {};

    template <typename T>
    struct as_meta_element<T&> : as_meta_element<T>   // always store by value
    {};

    template <typename T, int N>
    struct as_meta_element<T[N]>
    {
        typedef const T(&type)[N];
    };

    namespace result_of
    {
        template <typename Car, typename Cdr = fusion::nil_>
        struct make_cons
        {
            typedef typename as_meta_element<Car>::type car_type;            typedef typename fusion::cons<car_type, Cdr> type;
        };
    }

    template <typename Car, typename Cdr>
    fusion::cons<typename as_meta_element<Car>::type, Cdr>
    make_cons(Car const& car, Cdr const& cdr)
    {
        typedef typename as_meta_element<Car>::type car_type;
        typedef typename fusion::cons<car_type, Cdr> result;
        return result(car, cdr);
    }

    template <typename Car>
    fusion::cons<typename as_meta_element<Car>::type>
    make_cons(Car const& car)
    {
        typedef typename as_meta_element<Car>::type car_type;
        typedef typename fusion::cons<car_type> result;
        return result(car);
    }

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 0)
    // workaround for gcc-4.0 bug where illegal function types
    // can be formed (const is added to function type)
    // description: http://lists.boost.org/Archives/boost/2009/04/150743.php
    template <typename Car>
    fusion::cons<typename as_meta_element<Car>::type>
    make_cons(Car& car, typename enable_if<is_function<Car> >::type* = 0)
    {
        typedef typename as_meta_element<Car>::type car_type;
        typedef typename fusion::cons<car_type> result;
        return result(car);
    }
#endif
}}}

#endif
