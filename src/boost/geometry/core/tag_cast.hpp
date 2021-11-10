// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_CORE_TAG_CAST_HPP
#define BOOST_GEOMETRY_CORE_TAG_CAST_HPP


#include <type_traits>


namespace boost { namespace geometry
{

/*!
\brief Metafunction defining a type being either the specified tag, or one
    of the specified basetags if the type inherits from them.
\details Tags can inherit each other. A multi_point inherits, for example,
    both the multi_tag and the pointlike_tag. Often behaviour can be shared
    between different geometry types. A tag, found by the metafunction tag,
    can be casted to a more basic tag, and then dispatched by that tag.
\ingroup core
\tparam Tag The tag to be casted to one of the base tags
\tparam BaseTags Base tags

\qbk{[include reference/core/tag_cast.qbk]}
*/
template <typename Tag, typename ...BaseTags>
struct tag_cast
{
    typedef Tag type;
};

#ifndef DOXYGEN_NO_SPECIALIZATIONS

template <typename Tag, typename BaseTag, typename ...BaseTags>
struct tag_cast<Tag, BaseTag, BaseTags...>
{
    typedef std::conditional_t
        <
          std::is_base_of<BaseTag, Tag>::value,
          BaseTag,
          typename tag_cast
            <
                Tag, BaseTags...
            >::type
        > type;
};

#endif // DOXYGEN_NO_SPECIALIZATIONS


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_CORE_TAG_CAST_HPP
