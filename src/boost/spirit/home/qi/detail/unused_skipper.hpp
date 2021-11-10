//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_QI_UNUSED_SKIPPER_JUL_25_2009_0921AM)
#define BOOST_SPIRIT_QI_UNUSED_SKIPPER_JUL_25_2009_0921AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/unused.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace spirit { namespace qi { namespace detail
{
    template <typename Skipper>
    struct unused_skipper : unused_type
    {
        unused_skipper(Skipper const& skipper_)
          : skipper(skipper_) {}
        Skipper const& skipper;

        // silence MSVC warning C4512: assignment operator could not be generated
        BOOST_DELETED_FUNCTION(unused_skipper& operator= (unused_skipper const&))
    };

    template <typename Skipper>
    struct is_unused_skipper
      : mpl::false_ {};

    template <typename Skipper>
    struct is_unused_skipper<unused_skipper<Skipper> >
      : mpl::true_ {};

    template <>
    struct is_unused_skipper<unused_type>
      : mpl::true_ {};

    // If a surrounding lexeme[] directive was specified, the current
    // skipper is of the type unused_skipper. In this case we
    // re-activate the skipper which was active before the skip[]
    // directive.
    template <typename Skipper>
    inline Skipper const&
    get_skipper(unused_skipper<Skipper> const& u)
    {
        return u.skipper;
    }

    // If no surrounding lexeme[] directive was specified we keep what we got.
    template <typename Skipper>
    inline Skipper const&
    get_skipper(Skipper const& u)
    {
        return u;
    }

}}}}

#endif
