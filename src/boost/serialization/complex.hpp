#ifndef  BOOST_SERIALIZATION_COMPLEX_HPP
#define BOOST_SERIALIZATION_COMPLEX_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// serialization/utility.hpp:
// serialization for stl utility templates

// (C) Copyright 2007 Matthias Troyer .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <complex>
#include <boost/config.hpp>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/is_bitwise_serializable.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

template<class Archive, class T>
inline void serialize(
    Archive & ar,
    std::complex< T > & t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

template<class Archive, class T>
inline void save(
    Archive & ar,
    std::complex< T > const & t,
    const unsigned int /* file_version */
){
    const T re = t.real();
    const T im = t.imag();
    ar << boost::serialization::make_nvp("real", re);
    ar << boost::serialization::make_nvp("imag", im);
}

template<class Archive, class T>
inline void load(
    Archive & ar,
    std::complex< T >& t,
    const unsigned int /* file_version */
){
    T re;
    T im;
    ar >> boost::serialization::make_nvp("real", re);
    ar >> boost::serialization::make_nvp("imag", im);
    t = std::complex< T >(re,im);
}

// specialization of serialization traits for complex
template <class T>
struct is_bitwise_serializable<std::complex< T > >
    : public is_bitwise_serializable< T > {};

template <class T>
struct implementation_level<std::complex< T > >
    : mpl::int_<object_serializable> {} ;

// treat complex just like builtin arithmetic types for tracking
template <class T>
struct tracking_level<std::complex< T > >
    : mpl::int_<track_never> {} ;

} // serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_COMPLEX_HPP
