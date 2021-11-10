///////////////////////////////////////////////////////////////////////////////
//  Copyright 2020 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MP_TRANS_RED_TYPE_HPP
#define BOOST_MP_TRANS_RED_TYPE_HPP

namespace boost { namespace multiprecision { namespace detail {

template <class T>
struct transcendental_reduction_type
{
   using type = T;
};

}
}
} // namespace boost::multiprecision::detail

#endif // BOOST_MP_TRANS_RED_TYPE_HPP
