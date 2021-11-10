//  sign.hpp
//
//  (C) Copyright 2011 Vicente J. Botet Escriba
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//

#ifndef BOOST_RATIO_MPL_SIGN_HPP
#define BOOST_RATIO_MPL_SIGN_HPP

#include <boost/ratio/ratio.hpp>
#include <boost/ratio/mpl/numeric_cast.hpp>
#include <boost/ratio/detail/mpl/sign.hpp>

namespace boost { 
namespace mpl {

template<>
struct sign_impl< rational_c_tag >
{
    template< typename R > struct apply
        : ratio_sign<R>
    {
    };
};    
}
}

#endif  // BOOST_RATIO_MPL_SIGN_HPP
