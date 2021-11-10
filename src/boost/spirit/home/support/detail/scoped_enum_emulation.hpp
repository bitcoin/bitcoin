//  Copyright (c) 2001-2011 Hartmut Kaiser
//  http://spirit.sourceforge.net/
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SPIRIT_SCOPED_ENUM_EMULATION_HPP
#define BOOST_SPIRIT_SCOPED_ENUM_EMULATION_HPP

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/version.hpp>
#include <boost/config.hpp>

#if BOOST_VERSION >= 105600
# include <boost/core/scoped_enum.hpp>
#elif BOOST_VERSION >= 104000
# include <boost/detail/scoped_enum_emulation.hpp>
#else
# if !defined(BOOST_NO_CXX11_SCOPED_ENUMS)
#  define BOOST_NO_CXX11_SCOPED_ENUMS
# endif 
# define BOOST_SCOPED_ENUM_START(name) struct name { enum enum_type
# define BOOST_SCOPED_ENUM_END };
# define BOOST_SCOPED_ENUM(name) name::enum_type
#endif

#endif
