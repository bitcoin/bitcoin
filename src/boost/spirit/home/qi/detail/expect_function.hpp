/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_QI_DETAIL_EXPECT_FUNCTION_HPP
#define BOOST_SPIRIT_QI_DETAIL_EXPECT_FUNCTION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/multi_pass_wrapper.hpp>
#include <boost/throw_exception.hpp>

namespace boost { namespace spirit { namespace qi { namespace detail
{
    template <
        typename Iterator, typename Context
      , typename Skipper, typename Exception>
    struct expect_function
    {
        typedef Iterator iterator_type;
        typedef Context context_type;

        expect_function(
            Iterator& first_, Iterator const& last_
          , Context& context_, Skipper const& skipper_)
          : first(first_)
          , last(last_)
          , context(context_)
          , skipper(skipper_)
          , is_first(true)
        {
        }

        template <typename Component, typename Attribute>
        bool operator()(Component const& component, Attribute& attr) const
        {
            // if this is not the first component in the expect chain we 
            // need to flush any multi_pass iterator we might be acting on
            if (!is_first)
                spirit::traits::clear_queue(first);

            // if we are testing the first component in the sequence,
            // return true if the parser fails, if this is not the first
            // component, throw exception if the parser fails
            if (!component.parse(first, last, context, skipper, attr))
            {
                if (is_first)
                {
                    is_first = false;
                    return true;        // true means the match failed
                }
                boost::throw_exception(Exception(first, last, component.what(context)));
#if defined(BOOST_NO_EXCEPTIONS)
                return true;            // for systems not supporting exceptions
#endif
            }
            is_first = false;
            return false;
        }

        template <typename Component>
        bool operator()(Component const& component) const
        {
            // if this is not the first component in the expect chain we 
            // need to flush any multi_pass iterator we might be acting on
            if (!is_first)
                spirit::traits::clear_queue(first);

            // if we are testing the first component in the sequence,
            // return true if the parser fails, if this not the first
            // component, throw exception if the parser fails
            if (!component.parse(first, last, context, skipper, unused))
            {
                if (is_first)
                {
                    is_first = false;
                    return true;
                }
                boost::throw_exception(Exception(first, last, component.what(context)));
#if defined(BOOST_NO_EXCEPTIONS)
                return false;   // for systems not supporting exceptions
#endif
            }
            is_first = false;
            return false;
        }

        Iterator& first;
        Iterator const& last;
        Context& context;
        Skipper const& skipper;
        mutable bool is_first;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(expect_function& operator= (expect_function const&))
    };
}}}}

#endif
