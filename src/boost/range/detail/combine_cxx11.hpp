//  Copyright Neil Groves 2014. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_DETAIL_COMBINE_CXX11_HPP
#define BOOST_RANGE_DETAIL_COMBINE_CXX11_HPP

#include <boost/range/iterator_range_core.hpp>
#include <boost/range/iterator.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/iterator/zip_iterator.hpp>

namespace boost
{
    namespace range
    {

template<typename... Ranges>
auto combine(Ranges&&... rngs) ->
    combined_range<decltype(boost::make_tuple(boost::begin(rngs)...))>
{
    return combined_range<decltype(boost::make_tuple(boost::begin(rngs)...))>(
                boost::make_tuple(boost::begin(rngs)...),
                boost::make_tuple(boost::end(rngs)...));
}

    } // namespace range

using range::combine;

} // namespace boost

#endif // include guard
