///////////////////////////////////////////////////////////////////////////////
// foreach.hpp header file
//
// Copyright 2010 Eric Niebler.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//
// Credits:
// Kazutoshi Satoda: for suggesting the need for a _fwd header for foreach's
//                      customization points.

#ifndef BOOST_FOREACH_FWD_HPP
#define BOOST_FOREACH_FWD_HPP

// This must be at global scope, hence the uglified name
enum boost_foreach_argument_dependent_lookup_hack
{
    boost_foreach_argument_dependent_lookup_hack_value
};

namespace boost
{

namespace foreach
{
    ///////////////////////////////////////////////////////////////////////////////
    // boost::foreach::tag
    //
    typedef boost_foreach_argument_dependent_lookup_hack tag;

    ///////////////////////////////////////////////////////////////////////////////
    // boost::foreach::is_lightweight_proxy
    //   Specialize this for user-defined collection types if they are inexpensive to copy.
    //   This tells BOOST_FOREACH it can avoid the rvalue/lvalue detection stuff.
    template<typename T>
    struct is_lightweight_proxy;

    ///////////////////////////////////////////////////////////////////////////////
    // boost::foreach::is_noncopyable
    //   Specialize this for user-defined collection types if they cannot be copied.
    //   This also tells BOOST_FOREACH to avoid the rvalue/lvalue detection stuff.
    template<typename T>
    struct is_noncopyable;

} // namespace foreach

} // namespace boost

#endif
