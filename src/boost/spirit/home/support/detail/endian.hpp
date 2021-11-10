//  Copyright (c) 2001-2011 Hartmut Kaiser
//  http://spirit.sourceforge.net/
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_SUPPORT_DETAIL_ENDIAN_HPP
#define BOOST_SPIRIT_SUPPORT_DETAIL_ENDIAN_HPP

#if defined(_MSC_VER)
#pragma once
#endif

// We need to treat the endian number types as PODs
#if !defined(BOOST_ENDIAN_FORCE_PODNESS)
#define BOOST_ENDIAN_FORCE_PODNESS 1
#endif

#include <boost/endian/arithmetic.hpp>

#endif
