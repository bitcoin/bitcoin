#ifndef BOOST_BIND_STD_PLACEHOLDERS_HPP_INCLUDED
#define BOOST_BIND_STD_PLACEHOLDERS_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// Copyright 2021 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/is_placeholder.hpp>
#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL) && !defined(BOOST_NO_CXX11_DECLTYPE) && !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

#include <functional>
#include <type_traits>

namespace boost
{

template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_1)>::type > { enum _vt { value = 1 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_2)>::type > { enum _vt { value = 2 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_3)>::type > { enum _vt { value = 3 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_4)>::type > { enum _vt { value = 4 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_5)>::type > { enum _vt { value = 5 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_6)>::type > { enum _vt { value = 6 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_7)>::type > { enum _vt { value = 7 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_8)>::type > { enum _vt { value = 8 }; };
template<> struct is_placeholder< typename std::decay<decltype(std::placeholders::_9)>::type > { enum _vt { value = 9 }; };

} // namespace boost

#endif

#endif // #ifndef BOOST_BIND_STD_PLACEHOLDERS_HPP_INCLUDED
