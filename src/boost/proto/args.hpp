///////////////////////////////////////////////////////////////////////////////
/// \file args.hpp
/// Contains definition of \c term\<\>, \c list1\<\>, \c list2\<\>, ...
/// class templates.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_ARGS_HPP_EAN_04_01_2005
#define BOOST_PROTO_ARGS_HPP_EAN_04_01_2005

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/void.hpp>
#include <boost/proto/proto_fwd.hpp>
#include <boost/proto/detail/is_noncopyable.hpp>

#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/is_abstract.hpp>

namespace boost { namespace proto
{
    namespace detail
    {
        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits
        {
            typedef Expr value_type;
            typedef Expr &reference;
            typedef Expr const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits<Expr &>
        {
            typedef Expr value_type;
            typedef Expr &reference;
            typedef Expr &const_reference;
        };

        /// INTERNAL ONLY
        template<typename Expr>
        struct expr_traits<Expr const &>
        {
            typedef Expr value_type;
            typedef Expr const &reference;
            typedef Expr const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits
        {
            typedef T value_type;
            typedef T &reference;
            typedef T const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits<T &>
        {
            typedef typename mpl::if_c<is_noncopyable<T>::value, T &, T>::type value_type;
            typedef T &reference;
            typedef T &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T>
        struct term_traits<T const &>
        {
            typedef T value_type;
            typedef T const &reference;
            typedef T const &const_reference;
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T (&)[N]>
        {
            typedef T value_type[N];
            typedef T (&reference)[N];
            typedef T (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T const (&)[N]>
        {
            typedef T value_type[N];
            typedef T const (&reference)[N];
            typedef T const (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T[N]>
        {
            typedef T value_type[N];
            typedef T (&reference)[N];
            typedef T const (&const_reference)[N];
        };

        /// INTERNAL ONLY
        template<typename T, std::size_t N>
        struct term_traits<T const[N]>
        {
            typedef T value_type[N];
            typedef T const (&reference)[N];
            typedef T const (&const_reference)[N];
        };
    }

    namespace argsns_
    {
        // This is where term and all the different listN templates are defined
        #include <boost/proto/detail/args.hpp>
    }

}}

#endif

