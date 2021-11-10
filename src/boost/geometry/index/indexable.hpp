// Boost.Geometry Index
//
// Copyright (c) 2011-2019 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_INDEXABLE_HPP
#define BOOST_GEOMETRY_INDEX_INDEXABLE_HPP

#include <boost/tuple/tuple.hpp>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/is_indexable.hpp>

#include <boost/geometry/util/type_traits.hpp>

namespace boost { namespace geometry { namespace index { namespace detail
{

template <typename From, typename To>
struct is_referencable
    : std::is_same
        <
            typename util::remove_cref<From>::type,
            typename util::remove_cref<To>::type
        >
{};

template <typename Indexable, typename V>
inline Indexable const& indexable_prevent_any_type(V const& )
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE("Unexpected type.", V);
    return Indexable();
}

/*!
\brief The function object extracting Indexable from Value.

It translates Value object to Indexable object. The default version handles Values which are Indexables.
This template is also specialized for std::pair<Indexable, T2>, boost::tuple<Indexable, ...>
and std::tuple<Indexable, ...>.

\tparam Value       The Value type which may be translated directly to the Indexable.
\tparam IsIndexable If true, the const reference to Value is returned.
*/
template <typename Value, bool IsIndexable = is_indexable<Value>::value>
struct indexable
{
    BOOST_GEOMETRY_STATIC_ASSERT(
        (detail::is_indexable<Value>::value),
        "Value has to be an Indexable.",
        Value);

    /*! \brief The type of result returned by function object. */
    typedef Value const& result_type;

    /*!
    \brief Return indexable extracted from the value.
    
    \param v The value.
    \return The indexable.
    */
    inline result_type operator()(Value const& v) const
    {
        return v;
    }

    /*!
    \brief Prevent reference to temporary for types convertible to Value.
    */
    template <typename V>
    inline result_type operator()(V const& v) const
    {
        return indexable_prevent_any_type<Value>(v);
    }
};

/*!
\brief The function object extracting Indexable from Value.

This specialization translates from std::pair<Indexable, T2>.

\tparam Indexable       The Indexable type.
\tparam Second          The second type.
*/
template <typename Indexable, typename Second>
struct indexable<std::pair<Indexable, Second>, false>
{
    typedef std::pair<Indexable, Second> value_type;

    BOOST_GEOMETRY_STATIC_ASSERT(
        (detail::is_indexable<Indexable>::value),
        "The first type of std::pair has to be an Indexable.",
        Indexable);

    /*! \brief The type of result returned by function object. */
    typedef Indexable const& result_type;

    /*!
    \brief Return indexable extracted from the value.
    
    \param v The value.
    \return The indexable.
    */
    inline result_type operator()(value_type const& v) const
    {
        return v.first;
    }

    /*!
    \brief Return indexable extracted from compatible type different than value_type.

    \param v The value.
    \return The indexable.
    */
    template <typename I, typename S>
    inline result_type operator()(std::pair<I, S> const& v) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (is_referencable<I, result_type>::value),
            "Unexpected type.",
            std::pair<I, S>);
        return v.first;
    }

    /*!
    \brief Prevent reference to temporary for types convertible to Value.
    */
    template <typename V>
    inline result_type operator()(V const& v) const
    {
        return indexable_prevent_any_type<Indexable>(v);
    }
};

/*!
\brief The function object extracting Indexable from Value.

This specialization translates from boost::tuple<Indexable, ...>
  or boost::tuples::cons<Indexable, ...>.

\tparam Value       The Value type.
\tparam Indexable   The Indexable type.
*/
template <typename Value, typename Indexable>
struct indexable_boost_tuple
{
    typedef Value value_type;

    BOOST_GEOMETRY_STATIC_ASSERT(
        (detail::is_indexable<Indexable>::value),
        "The first type of boost::tuple has to be an Indexable.",
        Indexable);

    /*! \brief The type of result returned by function object. */
    typedef Indexable const& result_type;

    /*!
    \brief Return indexable extracted from the value.
    
    \param v The value.
    \return The indexable.
    */
    inline result_type operator()(value_type const& v) const
    {
        return boost::get<0>(v);
    }

    /*!
    \brief Return indexable extracted from compatible type different than value_type.

    \param v The value.
    \return The indexable.
    */
    template <typename I, typename U1, typename U2, typename U3, typename U4,
              typename U5, typename U6, typename U7, typename U8, typename U9>
    inline result_type operator()(boost::tuple<I, U1, U2, U3, U4, U5, U6, U7, U8, U9> const& v) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (is_referencable<I, result_type>::value),
            "Unexpected type.",
            boost::tuple<I, U1, U2, U3, U4, U5, U6, U7, U8, U9>);
        return boost::get<0>(v);
    }

    /*!
    \brief Return indexable extracted from compatible type different than value_type.

    \param v The value.
    \return The indexable.
    */
    template <typename I, typename T>
    inline result_type operator()(boost::tuples::cons<I, T> const& v) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (is_referencable<I, result_type>::value),
            "Unexpected type.",
            boost::tuples::cons<I, T>);
        return boost::get<0>(v);
    }

    /*!
    \brief Prevent reference to temporary for types convertible to Value.
    */
    template <typename V>
    inline result_type operator()(V const& v) const
    {
        return indexable_prevent_any_type<Indexable>(v);
    }
};

/*!
\brief The function object extracting Indexable from Value.

This specialization translates from boost::tuple<Indexable, ...>.

\tparam Indexable   The Indexable type.
*/
template <typename Indexable, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
struct indexable<boost::tuple<Indexable, T1, T2, T3, T4, T5, T6, T7, T8, T9>, false>
    : indexable_boost_tuple
        <
            boost::tuple<Indexable, T1, T2, T3, T4, T5, T6, T7, T8, T9>,
            Indexable
        >
{};

/*!
\brief The function object extracting Indexable from Value.

This specialization translates from boost::tuples::cons<Indexable, ...>.

\tparam Indexable   The Indexable type.
*/
template <typename Indexable, typename Tail>
struct indexable<boost::tuples::cons<Indexable, Tail>, false>
    : indexable_boost_tuple
        <
            boost::tuples::cons<Indexable, Tail>,
            Indexable
        >
{};

}}}} // namespace boost::geometry::index::detail

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#include <tuple>

namespace boost { namespace geometry { namespace index { namespace detail {

/*!
\brief The function object extracting Indexable from Value.

This specialization translates from std::tuple<Indexable, Args...>.
It's defined if the compiler supports tuples and variadic templates.

\tparam Indexable   The Indexable type.
*/
template <typename Indexable, typename ...Args>
struct indexable<std::tuple<Indexable, Args...>, false>
{
    typedef std::tuple<Indexable, Args...> value_type;

    BOOST_GEOMETRY_STATIC_ASSERT(
        (detail::is_indexable<Indexable>::value),
        "The first type of std::tuple has to be an Indexable.",
        Indexable);

    /*! \brief The type of result returned by function object. */
    typedef Indexable const& result_type;

    /*!
    \brief Return indexable extracted from the value.
    
    \param v The value.
    \return The indexable.
    */
    result_type operator()(value_type const& v) const
    {
        return std::get<0>(v);
    }

    /*!
    \brief Return indexable extracted from compatible type different than value_type.

    \param v The value.
    \return The indexable.
    */
    template <typename I, typename ...A>
    inline result_type operator()(std::tuple<I, A...> const& v) const
    {
        BOOST_GEOMETRY_STATIC_ASSERT(
            (is_referencable<I, result_type>::value),
            "Unexpected type.",
            std::tuple<I, A...>);
        return std::get<0>(v);
    }

    /*!
    \brief Prevent reference to temporary for types convertible to Value.
    */
    template <typename V>
    inline result_type operator()(V const& v) const
    {
        return indexable_prevent_any_type<Indexable>(v);
    }
};

}}}} // namespace boost::geometry::index::detail

#endif // !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

namespace boost { namespace geometry { namespace index {

/*!
\brief The function object extracting Indexable from Value.

It translates Value object to Indexable object. By default, it can handle Values which are Indexables,
std::pair<Indexable, T2>, boost::tuple<Indexable, ...> and std::tuple<Indexable, ...> if STD tuples
and variadic templates are supported.

\tparam Value       The Value type which may be translated directly to the Indexable.
*/
template <typename Value>
struct indexable
    : detail::indexable<Value>
{
    /*! \brief The type of result returned by function object. It should be const Indexable reference. */
    typedef typename detail::indexable<Value>::result_type result_type;

    /*!
    \brief Return indexable extracted from the value.
    
    \param v The value.
    \return The indexable.
    */
    inline result_type operator()(Value const& v) const
    {
        return detail::indexable<Value>::operator()(v);
    }

    /*!
    \brief Return indexable extracted from the value. Overload for types
           compatible with Value but different yet holding referencable
           Indexable, e.g. tuple containing a reference.

    \param v The value.
    \return The indexable.
    */
    template <typename V>
    inline result_type operator()(V const& v) const
    {
        return detail::indexable<Value>::operator()(v);
    }
};

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_INDEXABLE_HPP
