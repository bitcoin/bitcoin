// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2018-2021.
// Modifications copyright (c) 2018-2021, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT_HPP
#define BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT_HPP


#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>
//#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/core/geometry_types.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/visit.hpp>
#include <boost/geometry/util/sequence.hpp>


namespace boost { namespace geometry
{

namespace detail
{

template <typename ...>
struct parameter_pack_first_type {};

template <typename T, typename ... Ts>
struct parameter_pack_first_type<T, Ts...>
{
    typedef T type;
};


template <typename Seq, typename ResultSeq = util::type_sequence<>>
struct boost_variant_types;

template <typename T, typename ...Ts, typename ...Rs>
struct boost_variant_types<util::type_sequence<T, Ts...>, util::type_sequence<Rs...>>
{
    using type = typename boost_variant_types<util::type_sequence<Ts...>, util::type_sequence<Rs..., T>>::type;
};

template <typename ...Ts, typename ...Rs>
struct boost_variant_types<util::type_sequence<boost::detail::variant::void_, Ts...>, util::type_sequence<Rs...>>
{
    using type = util::type_sequence<Rs...>;
};

template <typename ...Rs>
struct boost_variant_types<util::type_sequence<>, util::type_sequence<Rs...>>
{
    using type = util::type_sequence<Rs...>;
};


} // namespace detail


// TODO: This is not used anywhere in the header files. Only in tests.
template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct point_type<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
    : point_type
        <
            typename detail::parameter_pack_first_type
                <
                    BOOST_VARIANT_ENUM_PARAMS(T)
                >::type
        >
{};


namespace traits
{

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct tag<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
    using type = dynamic_geometry_tag;
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct visit<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
    template <typename Function>
    struct visitor
        : boost::static_visitor<>
    {
        visitor(Function function)
            : m_function(function)
        {}

        template <typename Geometry>
        void operator()(Geometry && geometry)
        {
            m_function(std::forward<Geometry>(geometry));
        }

        Function m_function;
    };

    template <typename Function, typename Variant>
    static void apply(Function function, Variant && variant)
    {
        visitor<Function> visitor(function);
        boost::apply_visitor(visitor, std::forward<Variant>(variant));
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T), BOOST_VARIANT_ENUM_PARAMS(typename U)>
struct visit<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>, boost::variant<BOOST_VARIANT_ENUM_PARAMS(U)>>
{
    template <typename Function>
    struct visitor
        : boost::static_visitor<>
    {
        visitor(Function function)
            : m_function(function)
        {}

        template <typename Geometry1, typename Geometry2>
        void operator()(Geometry1 && geometry1, Geometry2 && geometry2)
        {
            m_function(std::forward<Geometry1>(geometry1),
                       std::forward<Geometry2>(geometry2));
        }

        Function m_function;
    };

    template <typename Function, typename Variant1, typename Variant2>
    static void apply(Function function, Variant1 && variant1, Variant2 && variant2)
    {
        visitor<Function> visitor(function);
        boost::apply_visitor(visitor,
                             std::forward<Variant1>(variant1),
                             std::forward<Variant2>(variant2));
    }
};


#ifdef BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct geometry_types<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
    using type = typename geometry::detail::boost_variant_types
        <
            util::type_sequence<BOOST_VARIANT_ENUM_PARAMS(T)>
        >::type;
};

#else // BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct geometry_types<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
    using type = util::type_sequence<BOOST_VARIANT_ENUM_PARAMS(T)>;
};

#endif // BOOST_VARIANT_DO_NOT_USE_VARIADIC_TEMPLATES

} // namespace traits


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_GEOMETRIES_ADAPTED_BOOST_VARIANT_HPP
