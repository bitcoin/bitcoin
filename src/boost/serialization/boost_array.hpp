#ifndef BOOST_SERIALIZATION_BOOST_ARRAY_HPP
#define BOOST_SERIALIZATION_BOOST_ARRAY_HPP

// (C) Copyright 2005 Matthias Troyer and Dave Abrahams
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp> // msvc 6.0 needs this for warning suppression

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/serialization/nvp.hpp>
#include <boost/array.hpp>

namespace boost { namespace serialization {
// implement serialization for boost::array
template <class Archive, class T, std::size_t N>
void serialize(Archive& ar, boost::array<T,N>& a, const unsigned int /* version */)
{
    ar & boost::serialization::make_nvp("elems", a.elems);
}

} } // end namespace boost::serialization


#endif //BOOST_SERIALIZATION_BOOST_ARRAY_HPP
