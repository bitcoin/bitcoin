//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//
// For more information, see http://www.boost.org/libs/range/
//
#ifndef BOOST_RANGE_ISTREAM_RANGE_HPP_INCLUDED
#define BOOST_RANGE_ISTREAM_RANGE_HPP_INCLUDED

/*!
 * \file istream_range.hpp
 */

#include <iterator>
#include <iosfwd>
#include <boost/config.hpp>
#include <boost/range/iterator_range.hpp>

namespace boost
{
    namespace range
    {
        template<class Type, class Elem, class Traits> inline
            iterator_range<std::istream_iterator<Type, Elem, Traits> >
        istream_range(std::basic_istream<Elem, Traits>& in)
        {
            return iterator_range<std::istream_iterator<Type, Elem, Traits> >(
                std::istream_iterator<Type, Elem, Traits>(in),
                std::istream_iterator<Type, Elem, Traits>());
        }
    } // namespace range
    using range::istream_range;
} // namespace boost

#endif // include guard
