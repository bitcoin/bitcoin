// Copyright (C) 2004 Arkadiy Vertleyb
// Copyright (C) 2005 Peder Holt
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_TEMPLATE_ENCODING_HPP_INCLUDED
#define BOOST_TYPEOF_TEMPLATE_ENCODING_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/detail/is_unary.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/cat.hpp>

#include <boost/typeof/encode_decode.hpp>
#include <boost/typeof/int_encoding.hpp>

#include <boost/typeof/type_template_param.hpp>
#include <boost/typeof/integral_template_param.hpp>
#include <boost/typeof/template_template_param.hpp>

#ifdef BOOST_BORLANDC
#define BOOST_TYPEOF_QUALIFY(P) self_t::P
#else
#define BOOST_TYPEOF_QUALIFY(P) P
#endif
// The template parameter description, entered by the user,
// is converted into a polymorphic "object"
// that is used to generate the code responsible for
// encoding/decoding the parameter, etc.

// make sure to cat the sequence first, and only then add the prefix.
#define BOOST_TYPEOF_MAKE_OBJ(elem) BOOST_PP_CAT(\
    BOOST_TYPEOF_MAKE_OBJ,\
    BOOST_PP_SEQ_CAT((_) BOOST_TYPEOF_TO_SEQ(elem))\
    )

#define BOOST_TYPEOF_TO_SEQ(tokens) BOOST_TYPEOF_ ## tokens ## _BOOST_TYPEOF

// BOOST_TYPEOF_REGISTER_TEMPLATE

#define BOOST_TYPEOF_REGISTER_TEMPLATE_EXPLICIT_ID(Name, Params, Id)\
    BOOST_TYPEOF_REGISTER_TEMPLATE_IMPL(\
        Name,\
        BOOST_TYPEOF_MAKE_OBJS(BOOST_TYPEOF_TOSEQ(Params)),\
        BOOST_PP_SEQ_SIZE(BOOST_TYPEOF_TOSEQ(Params)),\
        Id)

#define BOOST_TYPEOF_REGISTER_TEMPLATE(Name, Params)\
    BOOST_TYPEOF_REGISTER_TEMPLATE_EXPLICIT_ID(Name, Params, BOOST_TYPEOF_UNIQUE_ID())

#define BOOST_TYPEOF_OBJECT_MAKER(s, data, elem)\
    BOOST_TYPEOF_MAKE_OBJ(elem)

#define BOOST_TYPEOF_MAKE_OBJS(Params)\
    BOOST_PP_SEQ_TRANSFORM(BOOST_TYPEOF_OBJECT_MAKER, ~, Params)

// As suggested by Paul Mensonides:

#define BOOST_TYPEOF_TOSEQ(x)\
    BOOST_PP_IIF(\
        BOOST_PP_IS_UNARY(x),\
        x BOOST_PP_TUPLE_EAT(3), BOOST_PP_REPEAT\
    )(x, BOOST_TYPEOF_TOSEQ_2, ~)

#define BOOST_TYPEOF_TOSEQ_2(z, n, _) (class)

// BOOST_TYPEOF_VIRTUAL

#define BOOST_TYPEOF_CAT_4(a, b, c, d) BOOST_TYPEOF_CAT_4_I(a, b, c, d)
#define BOOST_TYPEOF_CAT_4_I(a, b, c, d) a ## b ## c ## d

#define BOOST_TYPEOF_VIRTUAL(Fun, Obj)\
    BOOST_TYPEOF_CAT_4(BOOST_TYPEOF_, BOOST_PP_SEQ_HEAD(Obj), _, Fun)

// BOOST_TYPEOF_SEQ_ENUM[_TRAILING][_1]
// Two versions provided due to reentrancy issue

#define BOOST_TYPEOF_SEQ_EXPAND_ELEMENT(z,n,seq)\
   BOOST_PP_SEQ_ELEM(0,seq) (z,n,BOOST_PP_SEQ_ELEM(n,BOOST_PP_SEQ_ELEM(1,seq)))

#define BOOST_TYPEOF_SEQ_ENUM(seq,macro)\
    BOOST_PP_ENUM(BOOST_PP_SEQ_SIZE(seq),BOOST_TYPEOF_SEQ_EXPAND_ELEMENT,(macro)(seq))

#define BOOST_TYPEOF_SEQ_ENUM_TRAILING(seq,macro)\
    BOOST_PP_ENUM_TRAILING(BOOST_PP_SEQ_SIZE(seq),BOOST_TYPEOF_SEQ_EXPAND_ELEMENT,(macro)(seq))

#define BOOST_TYPEOF_SEQ_EXPAND_ELEMENT_1(z,n,seq)\
    BOOST_PP_SEQ_ELEM(0,seq) (z,n,BOOST_PP_SEQ_ELEM(n,BOOST_PP_SEQ_ELEM(1,seq)))

#define BOOST_TYPEOF_SEQ_ENUM_1(seq,macro)\
    BOOST_PP_ENUM(BOOST_PP_SEQ_SIZE(seq),BOOST_TYPEOF_SEQ_EXPAND_ELEMENT_1,(macro)(seq))

#define BOOST_TYPEOF_SEQ_ENUM_TRAILING_1(seq,macro)\
    BOOST_PP_ENUM_TRAILING(BOOST_PP_SEQ_SIZE(seq),BOOST_TYPEOF_SEQ_EXPAND_ELEMENT_1,(macro)(seq))

//

#define BOOST_TYPEOF_PLACEHOLDER(z, n, elem)\
    BOOST_TYPEOF_VIRTUAL(PLACEHOLDER, elem)(elem)

#define BOOST_TYPEOF_PLACEHOLDER_TYPES(z, n, elem)\
    BOOST_TYPEOF_VIRTUAL(PLACEHOLDER_TYPES, elem)(elem, n)

#define BOOST_TYPEOF_REGISTER_TEMPLATE_ENCODE_PARAM(r, data, n, elem)\
    BOOST_TYPEOF_VIRTUAL(ENCODE, elem)(elem, n)

#define BOOST_TYPEOF_REGISTER_TEMPLATE_DECODE_PARAM(r, data, n, elem)\
    BOOST_TYPEOF_VIRTUAL(DECODE, elem)(elem, n)

#define BOOST_TYPEOF_REGISTER_TEMPLATE_PARAM_PAIR(z, n, elem) \
    BOOST_TYPEOF_VIRTUAL(EXPANDTYPE, elem)(elem) BOOST_PP_CAT(P, n)

#define BOOST_TYPEOF_REGISTER_DEFAULT_TEMPLATE_TYPE(Name,Params,ID)\
    Name< BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params), P) >

//Since we are creating an internal decode struct, we need to use different template names, T instead of P.
#define BOOST_TYPEOF_REGISTER_DECODER_TYPE_PARAM_PAIR(z,n,elem) \
    BOOST_TYPEOF_VIRTUAL(EXPANDTYPE, elem)(elem) BOOST_PP_CAT(T, n)

//Default template param decoding

#define BOOST_TYPEOF_TYPEDEF_DECODED_TEMPLATE_TYPE(Name,Params)\
    typedef Name<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params),BOOST_TYPEOF_QUALIFY(P))> type;

//Branch the decoding
#define BOOST_TYPEOF_TYPEDEF_DECODED_TYPE(Name,Params)\
    BOOST_PP_IF(BOOST_TYPEOF_HAS_TEMPLATES(Params),\
        BOOST_TYPEOF_TYPEDEF_DECODED_TEMPLATE_TEMPLATE_TYPE,\
        BOOST_TYPEOF_TYPEDEF_DECODED_TEMPLATE_TYPE)(Name,Params)

#define BOOST_TYPEOF_REGISTER_TEMPLATE_IMPL(Name, Params, Size, ID)\
    BOOST_TYPEOF_BEGIN_ENCODE_NS\
    BOOST_TYPEOF_REGISTER_TEMPLATE_TEMPLATE_IMPL(Name, Params, ID)\
    template<class V\
        BOOST_TYPEOF_SEQ_ENUM_TRAILING(Params, BOOST_TYPEOF_REGISTER_TEMPLATE_PARAM_PAIR)\
    >\
    struct encode_type_impl<V, Name<BOOST_PP_ENUM_PARAMS(Size, P)> >\
    {\
        typedef typename boost::type_of::push_back<V, boost::type_of::constant<std::size_t,ID> >::type V0;\
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_TYPEOF_REGISTER_TEMPLATE_ENCODE_PARAM, ~, Params)\
        typedef BOOST_PP_CAT(V, Size) type;\
    };\
    template<class Iter>\
    struct decode_type_impl<boost::type_of::constant<std::size_t,ID>, Iter>\
    {\
        typedef decode_type_impl<boost::type_of::constant<std::size_t,ID>, Iter> self_t;\
        typedef boost::type_of::constant<std::size_t,ID> self_id;\
        typedef Iter iter0;\
        BOOST_PP_SEQ_FOR_EACH_I(BOOST_TYPEOF_REGISTER_TEMPLATE_DECODE_PARAM, ~, Params)\
        BOOST_TYPEOF_TYPEDEF_DECODED_TYPE(Name, Params)\
        typedef BOOST_PP_CAT(iter, Size) iter;\
    };\
    BOOST_TYPEOF_END_ENCODE_NS

#endif//BOOST_TYPEOF_TEMPLATE_ENCODING_HPP_INCLUDED
