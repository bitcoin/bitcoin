//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_REPOSITORY_QI_FLUSH_MULTI_PASS_JUL_10_2009_0535PM)
#define BOOST_SPIRIT_REPOSITORY_QI_FLUSH_MULTI_PASS_JUL_10_2009_0535PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/attributes.hpp>
#include <boost/spirit/home/support/multi_pass.hpp>

#include <boost/spirit/repository/home/support/flush_multi_pass.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit 
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables flush_multi_pass
    template <>
    struct use_terminal<qi::domain, repository::tag::flush_multi_pass>
      : mpl::true_ {};

}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace repository { namespace qi
{
#ifndef BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
    using repository::flush_multi_pass;
#endif
    using repository::flush_multi_pass_type;

    ///////////////////////////////////////////////////////////////////////////
    // for a flush_multi_pass_parser generated parser
    struct flush_multi_pass_parser
      : spirit::qi::primitive_parser<flush_multi_pass_parser>
    {
        template <typename Context, typename Unused>
        struct attribute
        {
            typedef unused_type type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            spirit::traits::clear_queue(first, traits::clear_mode::clear_always);
            return true;
        }

        template <typename Context>
        info what(Context const& ctx) const
        {
            return info("flush_multi_pass");
        }
    };

}}}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace qi
{
    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<repository::tag::flush_multi_pass, Modifiers>
    {
        typedef repository::qi::flush_multi_pass_parser result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

}}}

#endif

