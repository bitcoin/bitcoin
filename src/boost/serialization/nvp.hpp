#ifndef BOOST_SERIALIZATION_NVP_HPP
#define BOOST_SERIALIZATION_NVP_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// nvp.hpp: interface for serialization system.

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/core/nvp.hpp>
#include <boost/preprocessor/stringize.hpp>

#define BOOST_SERIALIZATION_NVP(name)                       \
    boost::serialization::make_nvp(BOOST_PP_STRINGIZE(name), name)
/**/

#define BOOST_SERIALIZATION_BASE_OBJECT_NVP(name)           \
    boost::serialization::make_nvp(                         \
        BOOST_PP_STRINGIZE(name),                           \
        boost::serialization::base_object<name >(*this)     \
    )
/**/

#include <boost/serialization/level.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/wrapper.hpp>
#include <boost/serialization/base_object.hpp>

namespace boost {
namespace serialization {

template<class Archive, class T>
void save(
    Archive & ar,
    const nvp<T> & t,
    const unsigned int /* file_version */
){
    ar << t.const_value();
}
template<class Archive, class T>
void load(
    Archive & ar,
    nvp<T> & t ,
    const unsigned int /* file_version */
){
    ar >> t.value();
}

template<class Archive, class T>
inline void serialize(
    Archive & ar,
    nvp<T> & t,
    const unsigned int file_version
){
    split_free(ar, t, file_version);
}

template <class T>
struct implementation_level<nvp< T > >
{
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<object_serializable> type;
    BOOST_STATIC_CONSTANT(int, value = implementation_level::type::value);
};
template <class T>
struct implementation_level<const nvp< T > >
{
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<object_serializable> type;
    BOOST_STATIC_CONSTANT(int, value = implementation_level::type::value);
};

// nvp objects are generally created on the stack and are never tracked
template<class T>
struct tracking_level<nvp< T > >
{
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<track_never> type;
    BOOST_STATIC_CONSTANT(int, value = tracking_level::type::value);
};
template<class T>
struct tracking_level<const nvp< T > >
{
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<track_never> type;
    BOOST_STATIC_CONSTANT(int, value = tracking_level::type::value);
};

// these traits aren't used by nvp so they don't need to be defined
#if 0
template<class T>
struct version<const nvp< T > > {
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<0> type;
    BOOST_STATIC_CONSTANT(int, value = 0);
};
struct version<const nvp< T > > {
    typedef mpl::integral_c_tag tag;
    typedef mpl::int_<0> type;
    BOOST_STATIC_CONSTANT(int, value = 0);
};

template<class T>
struct extended_type_info_impl<const nvp< T > > {
    typedef extended_type_info_impl< T > type;
};
#endif

template<class T>
struct is_wrapper<const nvp<T> > {
    typedef boost::mpl::true_ type;
};
template<class T>
struct is_wrapper<nvp<T> > {
    typedef boost::mpl::true_ type;
};


} // seralization
} // boost


#endif // BOOST_SERIALIZATION_NVP_HPP
