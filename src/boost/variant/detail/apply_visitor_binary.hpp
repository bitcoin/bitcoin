//-----------------------------------------------------------------------------
// boost variant/detail/apply_visitor_binary.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003 Eric Friedman
// Copyright (c) 2014-2021 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_APPLY_VISITOR_BINARY_HPP
#define BOOST_VARIANT_DETAIL_APPLY_VISITOR_BINARY_HPP

#include <boost/config.hpp>

#include <boost/variant/detail/apply_visitor_unary.hpp>

#if !defined(BOOST_NO_CXX14_DECLTYPE_AUTO) && !defined(BOOST_NO_CXX11_DECLTYPE_N3276)
#   include <boost/variant/detail/has_result_type.hpp>
#endif

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#   include <boost/core/enable_if.hpp>
#   include <boost/type_traits/is_lvalue_reference.hpp>
#   include <boost/type_traits/is_same.hpp>
#   include <boost/move/utility_core.hpp> // for boost::move, boost::forward
#endif

namespace boost {

//////////////////////////////////////////////////////////////////////////
// function template apply_visitor(visitor, visitable1, visitable2)
//
// Visits visitable1 and visitable2 such that their values (which we
// shall call x and y, respectively) are used as arguments in the
// expression visitor(x, y).
//

namespace detail { namespace variant {

template <typename Visitor, typename Value1, bool MoveSemantics>
class apply_visitor_binary_invoke
{
public: // visitor typedefs

    typedef typename Visitor::result_type
        result_type;

private: // representation

    Visitor& visitor_;
    Value1& value1_;

public: // structors

    apply_visitor_binary_invoke(Visitor& visitor, Value1& value1) BOOST_NOEXCEPT
        : visitor_(visitor)
        , value1_(value1)
    {
    }

public: // visitor interfaces

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

    template <typename Value2>
        typename enable_if_c<MoveSemantics && is_same<Value2, Value2>::value, result_type>::type
    operator()(Value2&& value2)
    {
        return visitor_(::boost::move(value1_), ::boost::forward<Value2>(value2));
    }

    template <typename Value2>
        typename disable_if_c<MoveSemantics && is_same<Value2, Value2>::value, result_type>::type
    operator()(Value2&& value2)
    {
        return visitor_(value1_, ::boost::forward<Value2>(value2));
    }

#else

    template <typename Value2>
        result_type
    operator()(Value2& value2)
    {
        return visitor_(value1_, value2);
    }

#endif

private:
    apply_visitor_binary_invoke& operator=(const apply_visitor_binary_invoke&);
};

template <typename Visitor, typename Visitable2, bool MoveSemantics>
class apply_visitor_binary_unwrap
{
public: // visitor typedefs

    typedef typename Visitor::result_type
        result_type;

private: // representation

    Visitor& visitor_;
    Visitable2& visitable2_;

public: // structors

    apply_visitor_binary_unwrap(Visitor& visitor, Visitable2& visitable2) BOOST_NOEXCEPT
        : visitor_(visitor)
        , visitable2_(visitable2)
    {
    }

public: // visitor interfaces

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

    template <typename Value1>
        typename enable_if_c<MoveSemantics && is_same<Value1, Value1>::value, result_type>::type
    operator()(Value1&& value1)
    {
        apply_visitor_binary_invoke<
              Visitor
            , Value1
            , ! ::boost::is_lvalue_reference<Value1>::value
            > invoker(visitor_, value1);

        return boost::apply_visitor(invoker, ::boost::move(visitable2_));
    }

    template <typename Value1>
        typename disable_if_c<MoveSemantics && is_same<Value1, Value1>::value, result_type>::type
    operator()(Value1&& value1)
    {
        apply_visitor_binary_invoke<
              Visitor
            , Value1
            , ! ::boost::is_lvalue_reference<Value1>::value
            > invoker(visitor_, value1);

        return boost::apply_visitor(invoker, visitable2_);
    }

#else

    template <typename Value1>
        result_type
    operator()(Value1& value1)
    {
        apply_visitor_binary_invoke<
              Visitor
            , Value1
            , false
            > invoker(visitor_, value1);

        return boost::apply_visitor(invoker, visitable2_);
    }

#endif

private:
    apply_visitor_binary_unwrap& operator=(const apply_visitor_binary_unwrap&);

};

}} // namespace detail::variant

//
// nonconst-visitor version:
//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename Visitor, typename Visitable1, typename Visitable2>
inline typename Visitor::result_type
apply_visitor( Visitor& visitor, Visitable1&& visitable1, Visitable2&& visitable2)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap<
          Visitor, Visitable2, ! ::boost::is_lvalue_reference<Visitable2>::value
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, ::boost::forward<Visitable1>(visitable1));
}

#else

template <typename Visitor, typename Visitable1, typename Visitable2>
inline typename Visitor::result_type
apply_visitor( Visitor& visitor, Visitable1& visitable1, Visitable2& visitable2)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap<
          Visitor, Visitable2, false
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, visitable1);
}

#endif

//
// const-visitor version:
//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename Visitor, typename Visitable1, typename Visitable2>
inline typename Visitor::result_type
apply_visitor( const Visitor& visitor , Visitable1&& visitable1 , Visitable2&& visitable2)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap<
          const Visitor, Visitable2, ! ::boost::is_lvalue_reference<Visitable2>::value
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, ::boost::forward<Visitable1>(visitable1));
}

#else

template <typename Visitor, typename Visitable1, typename Visitable2>
inline typename Visitor::result_type
apply_visitor( const Visitor& visitor , Visitable1& visitable1 , Visitable2& visitable2)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap<
          const Visitor, Visitable2, false
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, visitable1);
}

#endif


#if !defined(BOOST_NO_CXX14_DECLTYPE_AUTO) && !defined(BOOST_NO_CXX11_DECLTYPE_N3276)

//////////////////////////////////////////////////////////////////////////
// function template apply_visitor(visitor, visitable1, visitable2)
//
// C++14 part.
//

namespace detail { namespace variant {

template <typename Visitor, typename Value1, bool MoveSemantics>
class apply_visitor_binary_invoke_cpp14
{
    Visitor& visitor_;
    Value1& value1_;

public: // structors

    apply_visitor_binary_invoke_cpp14(Visitor& visitor, Value1& value1) BOOST_NOEXCEPT
        : visitor_(visitor)
        , value1_(value1)
    {
    }

public: // visitor interfaces

    template <typename Value2>
    decltype(auto) operator()(Value2&& value2, typename enable_if_c<MoveSemantics && is_same<Value2, Value2>::value, bool>::type = true)
    {
        return visitor_(::boost::move(value1_), ::boost::forward<Value2>(value2));
    }

    template <typename Value2>
    decltype(auto) operator()(Value2&& value2, typename disable_if_c<MoveSemantics && is_same<Value2, Value2>::value, bool>::type = true)
    {
        return visitor_(value1_, ::boost::forward<Value2>(value2));
    }

private:
    apply_visitor_binary_invoke_cpp14& operator=(const apply_visitor_binary_invoke_cpp14&);
};

template <typename Visitor, typename Visitable2, bool MoveSemantics>
class apply_visitor_binary_unwrap_cpp14
{
    Visitor& visitor_;
    Visitable2& visitable2_;

public: // structors

    apply_visitor_binary_unwrap_cpp14(Visitor& visitor, Visitable2& visitable2) BOOST_NOEXCEPT
        : visitor_(visitor)
        , visitable2_(visitable2)
    {
    }

public: // visitor interfaces

    template <typename Value1>
    decltype(auto) operator()(Value1&& value1, typename enable_if_c<MoveSemantics && is_same<Value1, Value1>::value, bool>::type = true)
    {
        apply_visitor_binary_invoke_cpp14<
              Visitor
            , Value1
            , ! ::boost::is_lvalue_reference<Value1>::value
            > invoker(visitor_, value1);

        return boost::apply_visitor(invoker, ::boost::move(visitable2_));
    }

    template <typename Value1>
    decltype(auto) operator()(Value1&& value1, typename disable_if_c<MoveSemantics && is_same<Value1, Value1>::value, bool>::type = true)
    {
        apply_visitor_binary_invoke_cpp14<
              Visitor
            , Value1
            , ! ::boost::is_lvalue_reference<Value1>::value
            > invoker(visitor_, value1);

        return boost::apply_visitor(invoker, visitable2_);
    }

private:
    apply_visitor_binary_unwrap_cpp14& operator=(const apply_visitor_binary_unwrap_cpp14&);
};

}} // namespace detail::variant

template <typename Visitor, typename Visitable1, typename Visitable2>
inline decltype(auto) apply_visitor(Visitor& visitor, Visitable1&& visitable1, Visitable2&& visitable2,
    typename boost::disable_if<
        boost::detail::variant::has_result_type<Visitor>,
        bool
    >::type = true)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap_cpp14<
          Visitor, Visitable2, ! ::boost::is_lvalue_reference<Visitable2>::value
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, ::boost::forward<Visitable1>(visitable1));
}

template <typename Visitor, typename Visitable1, typename Visitable2>
inline decltype(auto) apply_visitor(const Visitor& visitor, Visitable1&& visitable1, Visitable2&& visitable2,
    typename boost::disable_if<
        boost::detail::variant::has_result_type<Visitor>,
        bool
    >::type = true)
{
    ::boost::detail::variant::apply_visitor_binary_unwrap_cpp14<
          const Visitor, Visitable2, ! ::boost::is_lvalue_reference<Visitable2>::value
        > unwrapper(visitor, visitable2);

    return boost::apply_visitor(unwrapper, ::boost::forward<Visitable1>(visitable1));
}


#endif // !defined(BOOST_NO_CXX14_DECLTYPE_AUTO) && !defined(BOOST_NO_CXX11_DECLTYPE_N3276)

} // namespace boost

#endif // BOOST_VARIANT_DETAIL_APPLY_VISITOR_BINARY_HPP
