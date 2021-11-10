//  Boost.Varaint
//  Contains multivisitors that are implemented via variadic templates, std::tuple
//  and decltype(auto)
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013-2014.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).

#ifndef BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP14_BASED_HPP
#define BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP14_BASED_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/variant/detail/multivisitors_cpp14_based.hpp>
#include <tuple>

namespace boost {

namespace detail { namespace variant {

    // Forward declaration
    template <typename Visitor, typename Visitables, typename... Values>
    class one_by_one_visitor_and_value_referer_cpp14;

    template <typename Visitor, typename Visitables, typename... Values>
    inline one_by_one_visitor_and_value_referer_cpp14<Visitor, Visitables, Values... >
        make_one_by_one_visitor_and_value_referer_cpp14(
            Visitor& visitor, Visitables visitables, std::tuple<Values...> values
        )
    {
        return one_by_one_visitor_and_value_referer_cpp14<Visitor, Visitables, Values... > (
            visitor, visitables, values
        );
    }

    template <typename Visitor, typename Visitables, typename... Values>
    class one_by_one_visitor_and_value_referer_cpp14
    {
        Visitor&                        visitor_;
        std::tuple<Values...>           values_;
        Visitables                      visitables_;

    public: // structors
        one_by_one_visitor_and_value_referer_cpp14(
                    Visitor& visitor, Visitables visitables, std::tuple<Values...> values
                ) BOOST_NOEXCEPT
            : visitor_(visitor)
            , values_(values)
            , visitables_(visitables)
        {}

    public: // visitor interfaces
        template <typename Value>
        decltype(auto) operator()(Value&& value) const
        {
            return ::boost::apply_visitor(
                make_one_by_one_visitor_and_value_referer_cpp14(
                    visitor_,
                    tuple_tail(visitables_),
                    std::tuple_cat(values_, std::make_tuple(wrap<Value, ! ::boost::is_lvalue_reference<Value>::value>(value)))
                )
                , unwrap(std::get<0>(visitables_)) // getting Head element
            );
        }

    private:
        one_by_one_visitor_and_value_referer_cpp14& operator=(const one_by_one_visitor_and_value_referer_cpp14&);
    };

    template <typename Visitor, typename... Values>
    class one_by_one_visitor_and_value_referer_cpp14<Visitor, std::tuple<>, Values...>
    {
        Visitor&                        visitor_;
        std::tuple<Values...>           values_;

    public:
        one_by_one_visitor_and_value_referer_cpp14(
                    Visitor& visitor, std::tuple<> /*visitables*/, std::tuple<Values...> values
                ) BOOST_NOEXCEPT
            : visitor_(visitor)
            , values_(values)
        {}

        template <class Tuple, std::size_t... I>
        decltype(auto) do_call(Tuple t, index_sequence<I...>) const {
            return visitor_(unwrap(std::get<I>(t))...);
        }

        template <typename Value>
        decltype(auto) operator()(Value&& value) const
        {
            return do_call(
                std::tuple_cat(values_, std::make_tuple(wrap<Value, ! ::boost::is_lvalue_reference<Value>::value>(value))),
                make_index_sequence<sizeof...(Values) + 1>()
            );
        }
    };

}} // namespace detail::variant

    template <class Visitor, class T1, class T2, class T3, class... TN>
    inline decltype(auto) apply_visitor(const Visitor& visitor, T1&& v1, T2&& v2, T3&& v3, TN&&... vn,
        typename boost::disable_if<
            boost::detail::variant::has_result_type<Visitor>,
            bool
        >::type = true)
    {
        return boost::apply_visitor(
            ::boost::detail::variant::make_one_by_one_visitor_and_value_referer_cpp14(
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
    inline decltype(auto) apply_visitor(Visitor& visitor, T1&& v1, T2&& v2, T3&& v3, TN&&... vn,
        typename boost::disable_if<
            boost::detail::variant::has_result_type<Visitor>,
            bool
        >::type = true)
    {
        return ::boost::apply_visitor(
            ::boost::detail::variant::make_one_by_one_visitor_and_value_referer_cpp14(
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

#endif // BOOST_VARIANT_DETAIL_MULTIVISITORS_CPP14_BASED_HPP

