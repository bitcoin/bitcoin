///////////////////////////////////////////////////////////////////////////////
/// \file tags.hpp
/// Contains the tags for all the overloadable operators in C++
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_TAGS_HPP_EAN_04_01_2005
#define BOOST_PROTO_TAGS_HPP_EAN_04_01_2005

#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace proto { namespace tagns_ { namespace tag
{

    /// Tag type for terminals; aka, leaves in the expression tree.
    struct terminal {};

    /// Tag type for the unary + operator.
    struct unary_plus {};

    /// Tag type for the unary - operator.
    struct negate {};

    /// Tag type for the unary * operator.
    struct dereference {};

    /// Tag type for the unary ~ operator.
    struct complement {};

    /// Tag type for the unary & operator.
    struct address_of {};

    /// Tag type for the unary ! operator.
    struct logical_not {};

    /// Tag type for the unary prefix ++ operator.
    struct pre_inc {};

    /// Tag type for the unary prefix -- operator.
    struct pre_dec {};

    /// Tag type for the unary postfix ++ operator.
    struct post_inc {};

    /// Tag type for the unary postfix -- operator.
    struct post_dec {};

    /// Tag type for the binary \<\< operator.
    struct shift_left {};

    /// Tag type for the binary \>\> operator.
    struct shift_right {};

    /// Tag type for the binary * operator.
    struct multiplies {};

    /// Tag type for the binary / operator.
    struct divides {};

    /// Tag type for the binary % operator.
    struct modulus {};

    /// Tag type for the binary + operator.
    struct plus {};

    /// Tag type for the binary - operator.
    struct minus {};

    /// Tag type for the binary \< operator.
    struct less {};

    /// Tag type for the binary \> operator.
    struct greater {};

    /// Tag type for the binary \<= operator.
    struct less_equal {};

    /// Tag type for the binary \>= operator.
    struct greater_equal {};

    /// Tag type for the binary == operator.
    struct equal_to {};

    /// Tag type for the binary != operator.
    struct not_equal_to {};

    /// Tag type for the binary || operator.
    struct logical_or {};

    /// Tag type for the binary && operator.
    struct logical_and {};

    /// Tag type for the binary & operator.
    struct bitwise_and {};

    /// Tag type for the binary | operator.
    struct bitwise_or {};

    /// Tag type for the binary ^ operator.
    struct bitwise_xor {};

    /// Tag type for the binary , operator.
    struct comma {};

    /// Tag type for the binary ->* operator.
    struct mem_ptr {};

    /// Tag type for the binary = operator.
    struct assign {};

    /// Tag type for the binary \<\<= operator.
    struct shift_left_assign {};

    /// Tag type for the binary \>\>= operator.
    struct shift_right_assign {};

    /// Tag type for the binary *= operator.
    struct multiplies_assign {};

    /// Tag type for the binary /= operator.
    struct divides_assign {};

    /// Tag type for the binary %= operator.
    struct modulus_assign {};

    /// Tag type for the binary += operator.
    struct plus_assign {};

    /// Tag type for the binary -= operator.
    struct minus_assign {};

    /// Tag type for the binary &= operator.
    struct bitwise_and_assign {};

    /// Tag type for the binary |= operator.
    struct bitwise_or_assign {};

    /// Tag type for the binary ^= operator.
    struct bitwise_xor_assign {};

    /// Tag type for the binary subscript operator.
    struct subscript {};

    /// Tag type for the binary virtual data members.
    struct member {};

    /// Tag type for the ternary ?: conditional operator.
    struct if_else_ {};

    /// Tag type for the n-ary function call operator.
    struct function {};

}}}}

#endif
