#ifndef BOOST_SERIALIZATION_WEAK_PTR_HPP
#define BOOST_SERIALIZATION_WEAK_PTR_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// weak_ptr.hpp: serialization for boost weak pointer

// (C) Copyright 2004 Robert Ramey and Martin Ecker
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/weak_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

namespace boost {
namespace serialization{

template<class Archive, class T>
inline void save(
    Archive & ar,
    const boost::weak_ptr< T > &t,
    const unsigned int /* file_version */
){
    const boost::shared_ptr< T > sp = t.lock();
    ar << boost::serialization::make_nvp("weak_ptr", sp);
}

template<class Archive, class T>
inline void load(
    Archive & ar,
    boost::weak_ptr< T > &t,
    const unsigned int /* file_version */
){
    boost::shared_ptr< T > sp;
    ar >> boost::serialization::make_nvp("weak_ptr", sp);
    t = sp;
}

template<class Archive, class T>
inline void serialize(
    Archive & ar,
    boost::weak_ptr< T > &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#ifndef BOOST_NO_CXX11_SMART_PTR
#include <memory>

namespace boost {
namespace serialization{

template<class Archive, class T>
inline void save(
    Archive & ar,
    const std::weak_ptr< T > &t,
    const unsigned int /* file_version */
){
    const std::shared_ptr< T > sp = t.lock();
    ar << boost::serialization::make_nvp("weak_ptr", sp);
}

template<class Archive, class T>
inline void load(
    Archive & ar,
    std::weak_ptr< T > &t,
    const unsigned int /* file_version */
){
    std::shared_ptr< T > sp;
    ar >> boost::serialization::make_nvp("weak_ptr", sp);
    t = sp;
}

template<class Archive, class T>
inline void serialize(
    Archive & ar,
    std::weak_ptr< T > &t,
    const unsigned int file_version
){
    boost::serialization::split_free(ar, t, file_version);
}

} // namespace serialization
} // namespace boost

#endif // BOOST_NO_CXX11_SMART_PTR

#endif // BOOST_SERIALIZATION_WEAK_PTR_HPP
