#ifndef BOOST_DESCRIBE_ENUMERATORS_HPP_INCLUDED
#define BOOST_DESCRIBE_ENUMERATORS_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/describe/detail/config.hpp>

#if defined(BOOST_DESCRIBE_CXX11)

namespace boost
{
namespace describe
{

template<class E> using describe_enumerators = decltype( boost_enum_descriptor_fn( static_cast<E*>(0) ) );

} // namespace describe
} // namespace boost

#endif // defined(BOOST_DESCRIBE_CXX11)

#endif // #ifndef BOOST_DESCRIBE_ENUMERATORS_HPP_INCLUDED
