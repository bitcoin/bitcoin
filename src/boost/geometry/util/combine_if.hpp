// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP
#define BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP

#include <boost/config/pragma_message.hpp>
#if !defined(BOOST_ALLOW_DEPRECATED_HEADERS)
BOOST_PRAGMA_MESSAGE("This header is deprecated.")
#endif

#include <boost/mpl/bind.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/set.hpp>

namespace boost { namespace geometry
{

namespace util
{


/*!
    \brief Meta-function to generate all the combination of pairs of types
        from a given sequence Sequence except those that does not satisfy the
        predicate Pred
    \ingroup utility
    \par Example
    \code
        typedef boost::mpl::vector<boost::mpl::int_<0>, boost::mpl::int_<1> > types;
        typedef combine_if<types, types, always<true_> >::type combinations;
        typedef boost::mpl::vector<
            pair<boost::mpl::int_<1>, boost::mpl::int_<1> >,
            pair<boost::mpl::int_<1>, boost::mpl::int_<0> >,
            pair<boost::mpl::int_<0>, boost::mpl::int_<1> >,
            pair<boost::mpl::int_<0>, boost::mpl::int_<0> >        
        > result_types;
        
        BOOST_MPL_ASSERT(( boost::mpl::equal<combinations, result_types> ));
    \endcode
*/
template <typename Sequence1, typename Sequence2, typename Pred>
struct combine_if
{
    struct combine
    {
        template <typename Result, typename T>
        struct apply
        {
            typedef typename boost::mpl::fold<Sequence2, Result,
                boost::mpl::if_
                <
                    boost::mpl::bind
                        <
                            typename boost::mpl::lambda<Pred>::type,
                            T,
                            boost::mpl::_2
                        >,
                    boost::mpl::insert
                        <
                            boost::mpl::_1, boost::mpl::pair<T, boost::mpl::_2>
                        >,
                    boost::mpl::_1
                >
            >::type type;
        };
    };

    typedef typename boost::mpl::fold
        <
            Sequence1, boost::mpl::set0<>, combine
        >::type type;
};


} // namespace util

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_COMBINE_IF_HPP
