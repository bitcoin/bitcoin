//-----------------------------------------------------------------------------
// boost variant/visitor_ptr.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003
// Eric Friedman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_VISITOR_PTR_HPP
#define BOOST_VARIANT_VISITOR_PTR_HPP

#include <boost/variant/bad_visit.hpp>
#include <boost/variant/static_visitor.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/type_traits/is_void.hpp>

namespace boost {

//////////////////////////////////////////////////////////////////////////
// function template visitor_ptr
//
// Adapts a function pointer for use as visitor capable of handling
// values of a single type. Throws bad_visit if inappropriately applied.
//
template <typename T, typename R>
class visitor_ptr_t
    : public static_visitor<R>
{
private: // representation

    typedef R (*visitor_t)(T);

    visitor_t visitor_;

public: // typedefs

    typedef R result_type;

private: // private typedefs

    typedef typename mpl::eval_if<
          is_reference<T>
        , mpl::identity<T>
        , add_reference<const T>
        >::type argument_fwd_type;

public: // structors

    explicit visitor_ptr_t(visitor_t visitor) BOOST_NOEXCEPT
      : visitor_(visitor)
    {
    }

public: // static visitor interfaces

    template <typename U>
    result_type operator()(const U&) const
    {
        boost::throw_exception(bad_visit());
    }

public: // static visitor interfaces, cont.

    result_type operator()(argument_fwd_type operand) const
    {
        return visitor_(operand);
    }

};

template <typename R, typename T>
inline visitor_ptr_t<T,R> visitor_ptr(R (*visitor)(T))
{
    return visitor_ptr_t<T,R>(visitor);
}

} // namespace boost

#endif// BOOST_VISITOR_VISITOR_PTR_HPP
