//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_WRAPPER_JUL_12_2009_0914PM)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_WRAPPER_JUL_12_2009_0914PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/support/detail/scoped_enum_emulation.hpp>

namespace boost { namespace spirit { namespace traits
{
    // declare special functions allowing to integrate any multi_pass iterator
    // with expectation points

    // normal iterators require no special handling
    BOOST_SCOPED_ENUM_START(clear_mode) 
    {
        clear_if_enabled,
        clear_always
    };
    BOOST_SCOPED_ENUM_END

    template <typename Iterator>
    void clear_queue(Iterator&
      , BOOST_SCOPED_ENUM(clear_mode) /*mode*/ = clear_mode::clear_if_enabled) 
    {}

    template <typename Iterator>
    void inhibit_clear_queue(Iterator&, bool) 
    {}

    template <typename Iterator>
    bool inhibit_clear_queue(Iterator&) 
    { 
        return false; 
    }

    // Helper template to recognize a multi_pass iterator. This specialization
    // will be instantiated for any non-multi_pass iterator.
    template <typename Iterator>
    struct is_multi_pass : mpl::false_ {};

}}}

#endif

