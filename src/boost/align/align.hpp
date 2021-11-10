/*
Copyright 2014-2015 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_ALIGN_ALIGN_HPP
#define BOOST_ALIGN_ALIGN_HPP

#include <boost/config.hpp>

#if !defined(BOOST_NO_CXX11_STD_ALIGN) && !defined(BOOST_LIBSTDCXX_VERSION)
#include <boost/align/detail/align_cxx11.hpp>
#else
#include <boost/align/detail/align.hpp>
#endif

#endif
