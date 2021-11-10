// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2003-2008 Matthias Christian Schabel
// Copyright (C) 2008 Steven Watanabe
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_CMATH_IMPL_HPP 
#define BOOST_UNITS_CMATH_IMPL_HPP

#include <boost/config.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost {
namespace units {
namespace detail {

template<class Y> 
inline bool isgreater BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1) || (boost::math::isnan)(v2)) return false;
    else return v1 > v2;
}

template<class Y> 
inline bool isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1) || (boost::math::isnan)(v2)) return false;
    else return v1 >= v2;
}

template<class Y> 
inline bool isless BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1) || (boost::math::isnan)(v2)) return false;
    else return v1 < v2;
}

template<class Y> 
inline bool islessequal BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1) || (boost::math::isnan)(v2)) return false;
    else return v1 <= v2;
}

template<class Y> 
inline bool islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1) || (boost::math::isnan)(v2)) return false;
    else return v1 < v2 || v1 > v2;
}

template<class Y> 
inline bool isunordered BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    return (boost::math::isnan)(v1) || (boost::math::isnan)(v2);
}

template<class Y>
inline Y fdim BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1)) return v1;
    else if((boost::math::isnan)(v2)) return v2;
    else if(v1 > v2) return(v1 - v2);
    else return(Y(0));
}

#if 0

template<class T>
struct fma_issue_warning {
    enum { value = false };
};

template<class Y>
inline Y fma(const Y& v1,const Y& v2,const Y& v3)
{
    //this implementation does *not* meet the
    //requirement of infinite intermediate precision
    BOOST_STATIC_WARNING((fma_issue_warning<Y>::value));

    return v1 * v2 + v3;
}

#endif

template<class Y>
inline Y fmax BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1)) return(v2);
    else if((boost::math::isnan)(v2)) return(v1);
    else if(v1 > v2) return(v1);
    else return(v2);
}

template<class Y>
inline Y fmin BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& v1,const Y& v2)
{
    if((boost::math::isnan)(v1)) return(v2);
    else if((boost::math::isnan)(v2)) return(v1);
    else if(v1 < v2) return(v1);
    else return(v2);
}

//template<class Y>
//inline long long llrint(const Y& val)
//{
//    return static_cast<long long>(rint(val));
//}
//
//template<class Y>
//inline long long llround(const Y& val)
//{
//    return static_cast<long long>(round(val));
//}

#if 0

template<class Y>
inline Y nearbyint(const Y& val)
{
    //this is not really correct.
    //the result should be according to the
    //current rounding mode.
    using boost::math::round;
    return round(val);
}

template<class Y>
inline Y rint(const Y& val)
{
    //I don't feel like trying to figure out
    //how to raise a floating pointer exception
    return nearbyint(val);
}

#endif

template<class Y>
inline Y trunc BOOST_PREVENT_MACRO_SUBSTITUTION(const Y& val)
{
    if(val > 0) return std::floor(val);
    else if(val < 0) return std::ceil(val);
    else return val;
}

}
}
}

#endif // BOOST_UNITS_CMATH_IMPL_HPP
