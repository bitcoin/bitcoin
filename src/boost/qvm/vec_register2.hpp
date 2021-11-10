/// Copyright (c) 2008-2021 Emil Dotchevski and Reverge Studios, Inc.
/// Copyright (c) 2018 agate-pris

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_QVM_VEC_REGISTER2_HPP
#define BOOST_QVM_VEC_REGISTER2_HPP

#include <boost/qvm/detail/vec_register_impl.hpp>

#define BOOST_QVM_REGISTER_VEC_2_READ(VecType, ScalarType, Read0, Read1)                    \
BOOST_QVM_DETAIL_REGISTER_VEC_SPECIALIZE_VEC_TRAITS_READ(VecType, ScalarType, 2)            \
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ(VecType, ScalarType, 2, 0, Read0)  \
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ(VecType, ScalarType, 2, 1, Read1)

#define BOOST_QVM_REGISTER_VEC_2_READ_WRITE(VecType, ScalarType, Read0, Read1, Write0, Write1)          \
BOOST_QVM_DETAIL_REGISTER_VEC_SPECIALIZE_VEC_TRAITS_READ_WRITE(VecType, ScalarType, 2)                  \
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ_WRITE(VecType, ScalarType, 2, 0, Read0, Write0)\
BOOST_QVM_DETAIL_SPECIALIZE_QVM_DETAIL_VEC_REGISTER_READ_WRITE(VecType, ScalarType, 2, 1, Read1, Write1)

#define BOOST_QVM_REGISTER_VEC_2(VecType, ScalarType, Element0, Element1)   \
BOOST_QVM_REGISTER_VEC_2_READ_WRITE(VecType, ScalarType, Element0, Element1, Element0, Element1)

#endif
