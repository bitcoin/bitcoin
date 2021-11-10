/*=============================================================================
    Copyright (c) 2001-2014 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_X3_OPTIONAL_TRAITS_FEBRUARY_06_2007_1001AM)
#define BOOST_SPIRIT_X3_OPTIONAL_TRAITS_FEBRUARY_06_2007_1001AM

#include <boost/spirit/home/x3/support/unused.hpp>
#include <boost/optional/optional.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace spirit { namespace x3 { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct is_optional
      : mpl::false_
    {};

    template <typename T>
    struct is_optional<boost::optional<T>>
      : mpl::true_
    {};

    ///////////////////////////////////////////////////////////////////////////
    // build_optional
    //
    // Build a boost::optional from T. Return unused_type if T is unused_type.
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct build_optional
    {
        typedef boost::optional<T> type;
    };

    template <typename T>
    struct build_optional<boost::optional<T> >
    {
        typedef boost::optional<T> type;
    };

    template <>
    struct build_optional<unused_type>
    {
        typedef unused_type type;
    };

    ///////////////////////////////////////////////////////////////////////////
    // optional_value
    //
    // Get the optional's value_type. Handles unused_type as well.
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct optional_value : mpl::identity<T> {};

    template <typename T>
    struct optional_value<boost::optional<T> >
      : mpl::identity<T> {};

    template <>
    struct optional_value<unused_type>
      : mpl::identity<unused_type> {};

    template <>
    struct optional_value<unused_type const>
      : mpl::identity<unused_type> {};

}}}}

#endif
