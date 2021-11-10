// Copyright (C) 2005 Peder Holt
// Copyright (C) 2005 Arkadiy Vertleyb
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_TYPEOF_TEMPLATE_TEMPLATE_PARAM_HPP_INCLUDED
#define BOOST_TYPEOF_TEMPLATE_TEMPLATE_PARAM_HPP_INCLUDED

#include <boost/preprocessor/logical/or.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/seq/enum.hpp>

#define BOOST_TYPEOF_MAKE_OBJ_template(x)   BOOST_TYPEOF_TEMPLATE_PARAM(x)
#define BOOST_TYPEOF_TEMPLATE(X) template(X) BOOST_TYPEOF_EAT
#define BOOST_TYPEOF_template(X) (template(X))

#define BOOST_TYPEOF_TEMPLATE_PARAM(Params)\
    (TEMPLATE_PARAM)\
    (Params)

#define BOOST_TYPEOF_TEMPLATE_PARAM_GETPARAMS(This)\
    BOOST_TYPEOF_TOSEQ(BOOST_PP_SEQ_ELEM(1, This))

//Encode / decode this
#define BOOST_TYPEOF_TEMPLATE_PARAM_ENCODE(This, n)\
   typedef typename boost::type_of::encode_template<BOOST_PP_CAT(V, n),\
       BOOST_PP_CAT(P, n)<BOOST_TYPEOF_SEQ_ENUM(BOOST_TYPEOF_MAKE_OBJS(BOOST_TYPEOF_TEMPLATE_PARAM_GETPARAMS(This)),BOOST_TYPEOF_PLACEHOLDER) >\
   >::type BOOST_PP_CAT(V, BOOST_PP_INC(n));

#define BOOST_TYPEOF_TEMPLATE_PARAM_DECODE(This, n)\
   typedef boost::type_of::decode_template< BOOST_PP_CAT(iter, n) > BOOST_PP_CAT(d, n);\
   typedef typename BOOST_PP_CAT(d, n)::type BOOST_PP_CAT(P, n);\
   typedef typename BOOST_PP_CAT(d, n)::iter BOOST_PP_CAT(iter,BOOST_PP_INC(n));

// template<class, unsigned int, ...> class
#define BOOST_TYPEOF_TEMPLATE_PARAM_EXPANDTYPE(This) \
    template <BOOST_PP_SEQ_ENUM(BOOST_TYPEOF_TEMPLATE_PARAM_GETPARAMS(This)) > class

#define BOOST_TYPEOF_TEMPLATE_PARAM_PLACEHOLDER(Param)\
    Nested_Template_Template_Arguments_Not_Supported

//'template<class,int> class' is reduced to 'class'
#define BOOST_TYPEOF_TEMPLATE_PARAM_DECLARATION_TYPE(Param) class

// T3<int, (unsigned int)0, ...>
#define BOOST_TYPEOF_TEMPLATE_PARAM_PLACEHOLDER_TYPES(Param, n)\
    BOOST_PP_CAT(T,n)<BOOST_TYPEOF_SEQ_ENUM_1(BOOST_TYPEOF_MAKE_OBJS(BOOST_TYPEOF_TEMPLATE_PARAM_GETPARAMS(Param)),BOOST_TYPEOF_PLACEHOLDER) >

#define BOOST_TYPEOF_TEMPLATE_PARAM_ISTEMPLATE 1

////////////////////////////
// move to encode_decode?

BOOST_TYPEOF_BEGIN_ENCODE_NS

template<class V, class Type_Not_Registered_With_Typeof_System> struct encode_template_impl;
template<class T, class Iter> struct decode_template_impl;

BOOST_TYPEOF_END_ENCODE_NS

namespace boost { namespace type_of {

    template<class V, class T> struct encode_template
        : BOOST_TYPEOF_ENCODE_NS_QUALIFIER::encode_template_impl<V, T>
    {};

    template<class Iter> struct decode_template
        : BOOST_TYPEOF_ENCODE_NS_QUALIFIER::decode_template_impl<typename Iter::type, typename Iter::next>
    {};
}}

////////////////////////////
// move to template_encoding.hpp?

//Template template registration
#define BOOST_TYPEOF_REGISTER_TYPE_FOR_TEMPLATE_TEMPLATE(Name,Params,ID)\
    template<class V\
        BOOST_TYPEOF_SEQ_ENUM_TRAILING(Params,BOOST_TYPEOF_REGISTER_TEMPLATE_PARAM_PAIR)\
    >\
    struct encode_template_impl<V,Name<\
        BOOST_PP_ENUM_PARAMS(\
        BOOST_PP_SEQ_SIZE(Params),\
        P)> >\
        : boost::type_of::push_back<V, boost::type_of::constant<std::size_t,ID> >\
    {\
    };\
    template<class Iter> struct decode_template_impl<boost::type_of::constant<std::size_t,ID>, Iter>\
    {\
        BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(Params),BOOST_TYPEOF_TYPEDEF_INT_PN,_)\
        typedef Name<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_PLACEHOLDER) > type;\
        typedef Iter iter;\
    };

#define BOOST_TYPEOF_TYPEDEF_INT_PN(z,n,Params) typedef int BOOST_PP_CAT(P,n);

#if defined(__BORLANDC__) && !defined(__clang__)
#define BOOST_TYPEOF_DECODE_NESTED_TEMPLATE_HELPER_NAME BOOST_PP_CAT(\
        BOOST_PP_CAT(\
            BOOST_PP_CAT(\
                decode_nested_template_helper,\
                BOOST_TYPEOF_REGISTRATION_GROUP\
            ),0x10000\
        ),__LINE__\
    )
#define BOOST_TYPEOF_REGISTER_DECODE_NESTED_TEMPLATE_HELPER_IMPL(Name,Params,ID)\
    struct BOOST_TYPEOF_DECODE_NESTED_TEMPLATE_HELPER_NAME {\
        template<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_REGISTER_DECLARE_DECODER_TYPE_PARAM_PAIR) >\
        struct decode_params;\
        template<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_REGISTER_DECODER_TYPE_PARAM_PAIR) >\
        struct decode_params<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_PLACEHOLDER_TYPES) >\
        {\
            typedef Name<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params),T)> type;\
        };\
    };
//Template template param decoding
#define BOOST_TYPEOF_TYPEDEF_DECODED_TEMPLATE_TEMPLATE_TYPE(Name,Params)\
    typedef typename BOOST_TYPEOF_DECODE_NESTED_TEMPLATE_HELPER_NAME::decode_params<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params),P)>::type type;

#else
#define BOOST_TYPEOF_REGISTER_DECODE_NESTED_TEMPLATE_HELPER_IMPL(Name,Params,ID)

//Template template param decoding
#define BOOST_TYPEOF_TYPEDEF_DECODED_TEMPLATE_TEMPLATE_TYPE(Name,Params)\
    template<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_REGISTER_DECLARE_DECODER_TYPE_PARAM_PAIR) >\
    struct decode_params;\
    template<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_REGISTER_DECODER_TYPE_PARAM_PAIR) >\
    struct decode_params<BOOST_TYPEOF_SEQ_ENUM(Params,BOOST_TYPEOF_PLACEHOLDER_TYPES) >\
    {\
        typedef Name<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params),T)> type;\
    };\
    typedef typename decode_params<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(Params),P)>::type type;
#endif
#define BOOST_TYPEOF_REGISTER_DECLARE_DECODER_TYPE_PARAM_PAIR(z,n,elem) \
    BOOST_TYPEOF_VIRTUAL(DECLARATION_TYPE, elem)(elem) BOOST_PP_CAT(T, n)

// BOOST_TYPEOF_HAS_TEMPLATES
#define BOOST_TYPEOF_HAS_TEMPLATES(Params)\
    BOOST_PP_SEQ_FOLD_LEFT(BOOST_TYPEOF_HAS_TEMPLATES_OP, 0, Params)

#define BOOST_TYPEOF_HAS_TEMPLATES_OP(s, state, elem)\
    BOOST_PP_OR(state, BOOST_TYPEOF_VIRTUAL(ISTEMPLATE, elem))

//Define template template arguments
#define BOOST_TYPEOF_REGISTER_TEMPLATE_TEMPLATE_IMPL(Name,Params,ID)\
    BOOST_PP_IF(BOOST_TYPEOF_HAS_TEMPLATES(Params),\
        BOOST_TYPEOF_REGISTER_DECODE_NESTED_TEMPLATE_HELPER_IMPL,\
        BOOST_TYPEOF_REGISTER_TYPE_FOR_TEMPLATE_TEMPLATE)(Name,Params,ID)

#endif //BOOST_TYPEOF_TEMPLATE_TEMPLATE_PARAM_HPP_INCLUDED
