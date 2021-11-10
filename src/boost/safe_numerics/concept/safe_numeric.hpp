#ifndef BOOST_NUMERIC_CONCEPT_SAFE_NUMERIC_HPP
#define BOOST_NUMERIC_CONCEPT_SAFE_NUMERIC_HPP

//  Copyright (c) 2015 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include <typetraits>
#include <boost/concept/usage.hpp>
#include "boost/safe_numerics/concept/safe_numeric.hpp"

namespace boost {
namespace safe_numerics {

template<class T>
struct SafeNumeric : public Numeric<T> {
    BOOST_CONCEPT_USAGE(SafeNumeric<T>){
        using t1 = get_exception_policy<T>;
        using t2 = get_promotion_policy<T>;
        using t3 = base_type<T>;
    }
    constexpr static bool value = is_safe<T>::value && Numeric<T>::value ;
    constexpr operator bool (){
        return value;
    }
};

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_CONCEPT_SAFE_NUMERIC_HPP
