//  Boost.Varaint
//  Contains multivisitors that are implemented via variadic templates and std::tuple
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013-2014.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#ifndef BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP11_BASED_HPP
#define BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP11_BASED_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/variant/detail/apply_visitor_unary.hpp>
#include <boost/variant/variant_fwd.hpp> // for BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES
#include <boost/move/utility.hpp>
#include <boost/type_traits/is_lvalue_reference.hpp>
#include <boost/core/enable_if.hpp>

#if defined(BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES) || defined(BOOST_NO_CXX11_HDR_TUPLE)
#   error "This file requires <tuple> and variadic templates support"
#endif

#include <tuple>

namespace boost { 

namespace detail { namespace variant {

    // Implementing some of the C++14 features in C++11
    template <std::size_t... I> class index_sequence {};

    template <std::size_t N, std::size_t... I> 
    struct make_index_sequence 
        : make_index_sequence<N-1, N-1, I...> 
    {};
    template <std::size_t... I> 
    struct make_index_sequence<0, I...> 
        : index_sequence<I...> 
    {};

    template <typename T_, bool MoveSemantics_>
    struct MoveableWrapper //Just a reference with some metadata
    {
        typedef T_ T;
        static constexpr bool MoveSemantics = MoveSemantics_;

        T& v;
    };

    template <typename Tp, bool MoveSemantics>
    MoveableWrapper<Tp, MoveSemantics>
        wrap(Tp& t)
    {
        return MoveableWrapper<Tp, MoveSemantics>{t};
    }

    template <typename Wrapper>
    typename enable_if_c<Wrapper::MoveSemantics, typename Wrapper::T>::type
        unwrap(Wrapper& w)
    {
        return ::boost::move(w.v);
    }

    template <typename Wrapper>
    typename disable_if_c<Wrapper::MoveSemantics, typename Wrapper::T>::type &
        unwrap(Wrapper& w)
    {
        return w.v;
    }

    // Implementing some of the helper tuple methods
    template <std::size_t... I, typename Tuple>
    std::tuple<typename std::tuple_element<I + 1, Tuple>::type...> 
        tuple_tail_impl(const Tuple& tpl, index_sequence<I...>)
    {
        return std::tuple<
            typename std::tuple_element<I + 1, Tuple>::type...
        > (std::get<I + 1>(tpl)...);
    }

    template <typename Head, typename... Tail>
    std::tuple<Tail...> tuple_tail(const std::tuple<Head, Tail...>& tpl)
    {
        return tuple_tail_impl(tpl, make_index_sequence<sizeof...(Tail)>());
    }


    // Forward declaration
    template <typename Visitor, typename Visitables, typename... Values>
    class one_by_one_visitor_and_value_referer;

    template <typename Visitor, typename Visitables, typename... Values>
    inline one_by_one_visitor_and_value_referer<Visitor, Visitables, Values... >
        make_one_by_one_visitor_and_value_referer(
            Visitor& visitor, Visitables visitables, std::tuple<Values...> values
        )
    {
        return one_by_one_visitor_and_value_referer<Visitor, Visitables, Values... > (
            visitor, visitables, values
        );
    }

    template <typename Visitor, typename Visitables, typename... Values>
    class one_by_one_visitor_and_value_referer
    {
        Visitor&                        visitor_;
        std::tuple<Values...>           values_;
        Visitables                      visitables_;

    public: // structors
        one_by_one_visitor_and_value_referer(
                    Visitor& visitor, Visitables visitables, std::tuple<Values...> values
                ) BOOST_NOEXCEPT
            : visitor_(visitor)
            , values_(values)
            , visitables_(visitables)
        {}

    public: // visitor interfaces
        typedef typename Visitor::result_type result_type;

        template <typename Value>
        result_type operator()(Value&& value) const
        {
            return ::boost::apply_visitor(
                make_one_by_one_visitor_and_value_referer(
                    visitor_,
                    tuple_tail(visitables_),
                    std::tuple_cat(values_, std::make_tuple(wrap<Value, ! ::boost::is_lvalue_reference<Value>::value>(value)))
                )
                , unwrap(std::get<0>(visitables_)) // getting Head element
            );
        }

    private:
        one_by_one_visitor_and_value_referer& operator=(const one_by_one_visitor_and_value_referer&);
    };

    template <typename Visitor, typename... Values>
    class one_by_one_visitor_and_value_referer<Visitor, std::tuple<>, Values...>
    {
        Visitor&                        visitor_;
        std::tuple<Values...>           values_;

    public:
        one_by_one_visitor_and_value_referer(
                    Visitor& visitor, std::tuple<> /*visitables*/, std::tuple<Values...> values
                ) BOOST_NOEXCEPT
            : visitor_(visitor)
            , values_(values)
        {}

        typedef typename Visitor::result_type result_type;

        template <class Tuple, std::size_t... I>
        result_type do_call(Tuple t, index_sequence<I...>) const {
            return visitor_(unwrap(std::get<I>(t))...);
        }

        template <typename Value>
        result_type operator()(Value&& value) const
        {
            return do_call(
                std::tuple_cat(values_, std::make_tuple(wrap<Value, ! ::boost::is_lvalue_reference<Value>::value>(value))),
                make_index_sequence<sizeof...(Values) + 1>()
            );
        }
    };

}} // namespace detail::variant

    template <class Visitor, class T1, class T2, class T3, class... TN>
    inline typename Visitor::result_type
        apply_visitor(const Visitor& visitor, T1&& v1, T2&& v2, T3&& v3, TN&&... vn)
    {
        return ::boost::apply_visitor(
            ::boost::detail::variant::make_one_by_one_visitor_and_value_referer(
                visitor,
                std::make_tuple(
                    ::boost::detail::variant::wrap<T2, ! ::boost::is_lvalue_reference<T2>::value>(v2),
                    ::boost::detail::variant::wrap<T3, ! ::boost::is_lvalue_reference<T3>::value>(v3),
                    ::boost::detail::variant::wrap<TN, ! ::boost::is_lvalue_reference<TN>::value>(vn)...
                    ),
                std::tuple<>()
                ),
                ::boost::forward<T1>(v1)
            );
    }
    
    template <class Visitor, class T1, class T2, class T3, class... TN>
    inline typename Visitor::result_type
        apply_visitor(Visitor& visitor, T1&& v1, T2&& v2, T3&& v3, TN&&... vn)
    {
        return ::boost::apply_visitor(
            ::boost::detail::variant::make_one_by_one_visitor_and_value_referer(
                visitor,
                std::make_tuple(
                    ::boost::detail::variant::wrap<T2, ! ::boost::is_lvalue_reference<T2>::value>(v2),
                    ::boost::detail::variant::wrap<T3, ! ::boost::is_lvalue_reference<T3>::value>(v3),
                    ::boost::detail::variant::wrap<TN, ! ::boost::is_lvalue_reference<TN>::value>(vn)...
                    ),
                std::tuple<>()
                ),
                ::boost::forward<T1>(v1)
            );
    }

} // namespace boost

#endif // BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP11_BASED_HPP

