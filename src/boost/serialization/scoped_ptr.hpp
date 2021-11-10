#ifndef BOOST_SERIALIZATION_SCOPED_PTR_HPP_VP_2003_10_30
#define BOOST_SERIALIZATION_SCOPED_PTR_HPP_VP_2003_10_30

#if defined(_MSC_VER)
# pragma once
#endif

//  Copyright (c) 2003 Vladimir Prus.
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Provides non-intrusive serialization for boost::scoped_ptr
// Does not allow to serialize scoped_ptr's to builtin types.

#include <boost/config.hpp>

#include <boost/scoped_ptr.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

    template<class Archive, class T>
    void save(
        Archive & ar,
        const boost::scoped_ptr< T > & t,
        const unsigned int /* version */
    ){
        T* r = t.get();
        ar << boost::serialization::make_nvp("scoped_ptr", r);
    }

    template<class Archive, class T>
    void load(
        Archive & ar,
        boost::scoped_ptr< T > & t,
        const unsigned int /* version */
    ){
        T* r;
        ar >> boost::serialization::make_nvp("scoped_ptr", r);
        t.reset(r);
    }

    template<class Archive, class T>
    void serialize(
        Archive& ar,
        boost::scoped_ptr< T >& t,
        const unsigned int version
    ){
        boost::serialization::split_free(ar, t, version);
    }

} // namespace serialization
} // namespace boost

#endif // BOOST_SERIALIZATION_SCOPED_PTR_HPP_VP_2003_10_30
