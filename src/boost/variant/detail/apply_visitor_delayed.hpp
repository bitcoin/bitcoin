//-----------------------------------------------------------------------------
// boost variant/detail/apply_visitor_delayed.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003
// Eric Friedman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_APPLY_VISITOR_DELAYED_HPP
#define BOOST_VARIANT_DETAIL_APPLY_VISITOR_DELAYED_HPP

#include <boost/variant/detail/apply_visitor_unary.hpp>
#include <boost/variant/detail/apply_visitor_binary.hpp>
#include <boost/variant/variant_fwd.hpp> // for BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES


#include <boost/variant/detail/has_result_type.hpp>
#include <boost/core/enable_if.hpp>

namespace boost {

//////////////////////////////////////////////////////////////////////////
// function template apply_visitor(visitor)
//
// Returns a function object, overloaded for unary and binary usage, that
// visits its arguments using visitor (or a copy of visitor) via
//  * apply_visitor( visitor, [argument] )
// under unary invocation, or
//  * apply_visitor( visitor, [argument1], [argument2] )
// under binary invocation.
//
// NOTE: Unlike other apply_visitor forms, the visitor object must be
//   non-const; this prevents user from giving temporary, to disastrous
//   effect (i.e., returned function object would have dead reference).
//

template <typename Visitor>
class apply_visitor_delayed_t
{
public: // visitor typedefs

    typedef typename Visitor::result_type
        result_type;

private: // representation

    Visitor& visitor_;

public: // structors

    explicit apply_visitor_delayed_t(Visitor& visitor) BOOST_NOEXCEPT
      : visitor_(visitor)
    {
    }

#if !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

public: // N-ary visitor interface
    template <typename... Visitables>
    result_type operator()(Visitables&... visitables) const
    {
        return apply_visitor(visitor_, visitables...);
    }

#else // !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

public: // unary visitor interface

    template <typename Visitable>
    result_type operator()(Visitable& visitable) const
    {
        return apply_visitor(visitor_, visitable);
    }

public: // binary visitor interface

    template <typename Visitable1, typename Visitable2>
    result_type operator()(Visitable1& visitable1, Visitable2& visitable2) const
    {
        return apply_visitor(visitor_, visitable1, visitable2);
    }

#endif // !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

private:
    apply_visitor_delayed_t& operator=(const apply_visitor_delayed_t&);

};

template <typename Visitor>
inline typename boost::enable_if<
        boost::detail::variant::has_result_type<Visitor>,
        apply_visitor_delayed_t<Visitor>
    >::type apply_visitor(Visitor& visitor)
{
    return apply_visitor_delayed_t<Visitor>(visitor);
}

#if !defined(BOOST_NO_CXX14_DECLTYPE_AUTO) && !defined(BOOST_NO_CXX11_DECLTYPE_N3276) \
    && !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)

template <typename Visitor>
class apply_visitor_delayed_cpp14_t
{
private: // representation
    Visitor& visitor_;

public: // structors

    explicit apply_visitor_delayed_cpp14_t(Visitor& visitor) BOOST_NOEXCEPT
      : visitor_(visitor)
    {
    }

public: // N-ary visitor interface
    template <typename... Visitables>
    decltype(auto) operator()(Visitables&... visitables) const
    {
        return apply_visitor(visitor_, visitables...);
    }

private:
    apply_visitor_delayed_cpp14_t& operator=(const apply_visitor_delayed_cpp14_t&);

};

template <typename Visitor>
inline  typename boost::disable_if<
        boost::detail::variant::has_result_type<Visitor>,
        apply_visitor_delayed_cpp14_t<Visitor>
    >::type apply_visitor(Visitor& visitor)
{
    return apply_visitor_delayed_cpp14_t<Visitor>(visitor);
}

#endif // !defined(BOOST_NO_CXX14_DECLTYPE_AUTO) && !defined(BOOST_NO_CXX11_DECLTYPE_N3276)
            // && !defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES)


} // namespace boost

#endif // BOOST_VARIANT_DETAIL_APPLY_VISITOR_DELAYED_HPP
