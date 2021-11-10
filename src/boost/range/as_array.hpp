// Boost.Range library
//
//  Copyright Thorsten Ottosen 2006. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_AS_ARRAY_HPP
#define BOOST_RANGE_AS_ARRAY_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/range/iterator_range.hpp>
#include <boost/range/detail/str_types.hpp>

namespace boost
{

    template< class R >
    inline iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<R>::type > 
    as_array( R& r )
    {
        return boost::make_iterator_range( r );
    }

    template< class Range >
    inline boost::iterator_range< BOOST_DEDUCED_TYPENAME range_iterator<const Range>::type > 
    as_array( const Range& r )
    {
        return boost::make_iterator_range( r );
    }
    
}

#endif

