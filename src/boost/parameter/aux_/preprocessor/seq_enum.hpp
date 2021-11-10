// Copyright David Abrahams 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_PREPROCESSOR_SEQ_ENUM_HPP
#define BOOST_PARAMETER_AUX_PREPROCESSOR_SEQ_ENUM_HPP

#include <boost/preprocessor/seq/enum.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>

#if BOOST_WORKAROUND(__MWERKS__, <= 0x3003)
#include <boost/preprocessor/seq/size.hpp>
// Temporary version of BOOST_PP_SEQ_ENUM
// until Paul M. integrates the workaround.
#define BOOST_PARAMETER_SEQ_ENUM_I(size, seq) \
    BOOST_PP_CAT(BOOST_PP_SEQ_ENUM_, size) seq
#define BOOST_PARAMETER_SEQ_ENUM(seq) \
    BOOST_PARAMETER_SEQ_ENUM_I(BOOST_PP_SEQ_SIZE(seq), seq)
#else
#define BOOST_PARAMETER_SEQ_ENUM(seq) BOOST_PP_SEQ_ENUM(seq)
#endif

#endif  // include guard

