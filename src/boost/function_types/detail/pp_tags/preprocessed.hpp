
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

// no include guards, this file is guarded externally

// this file has been generated from the master.hpp file in the same directory
namespace boost { namespace function_types {
typedef detail::property_tag<0x00000200,0x00000300> non_variadic;
typedef detail::property_tag<0x00000100,0x00000300> variadic;
typedef detail::property_tag<0,0x00000400> non_const;
typedef detail::property_tag<0x00000400,0x00000400> const_qualified;
typedef detail::property_tag<0,0x00000800> non_volatile;
typedef detail::property_tag<0x00000800,0x00000800> volatile_qualified; 
typedef detail::property_tag<0x00008000,0x00ff8000> default_cc;
typedef detail::property_tag<0 , 3072> non_cv;
typedef detail::property_tag<0x00000400 , 3072> const_non_volatile;
typedef detail::property_tag<0x00000800, 3072> volatile_non_const;
typedef detail::property_tag<3072 , 3072> cv_qualified;
namespace detail {
typedef constant<0x00ff0fff> full_mask;
template <bits_t Flags, bits_t CCID> struct encode_bits_impl
{
BOOST_STATIC_CONSTANT( bits_t, value = 
Flags | (0x00008000 * CCID) << 1 );
};
template <bits_t Flags, bits_t CCID, std::size_t Arity> 
struct encode_charr_impl
{
BOOST_STATIC_CONSTANT(std::size_t, value = (std::size_t)(1+
Flags | (0x00008000 * CCID) << 1 | Arity << 24
));
};
template <bits_t Bits> struct decode_bits
{
BOOST_STATIC_CONSTANT(bits_t, flags = Bits & 0x00000fff);
BOOST_STATIC_CONSTANT(bits_t, cc_id = 
( (Bits & 0x00ff0fff) / 0x00008000) >> 1 
);
BOOST_STATIC_CONSTANT(bits_t, tag_bits = (Bits & 0x00ff0fff));
BOOST_STATIC_CONSTANT(std::size_t, arity = (std::size_t)
(Bits >> 24) 
);
};
template <bits_t LHS_bits, bits_t LHS_mask, bits_t RHS_bits, bits_t RHS_mask>
struct tag_ice
{
BOOST_STATIC_CONSTANT(bool, match =
RHS_bits == (LHS_bits & RHS_mask & (RHS_bits | ~0x000000ff))
);
BOOST_STATIC_CONSTANT(bits_t, combined_bits = 
(LHS_bits & ~RHS_mask) | RHS_bits
);
BOOST_STATIC_CONSTANT(bits_t, combined_mask =
LHS_mask | RHS_mask
);
BOOST_STATIC_CONSTANT(bits_t, extracted_bits =
LHS_bits & RHS_mask
);
};
typedef property_tag<0x00000001,0x000000ff> callable_builtin_tag;
typedef property_tag<0x00000003,0x000000ff> nonmember_callable_builtin_tag;
typedef property_tag<0x00000007,0x000000ff> function_tag;
typedef property_tag<0x00000013,0x000000ff> reference_tag;
typedef property_tag<0x0000000b,0x000000ff> pointer_tag;
typedef property_tag<0x00000061,0x000000ff> member_function_pointer_tag;
typedef property_tag<0x000000a3,0x000000ff> member_object_pointer_tag;
typedef property_tag<0x000002a3,0x00ff0fff> member_object_pointer_base;
typedef property_tag<0x00000020,0x000000ff> member_pointer_tag;
typedef property_tag< 33287 , 16745471 > nv_dcc_func;
typedef property_tag< 33377 , 16745471 > nv_dcc_mfp;
} 
} } 
