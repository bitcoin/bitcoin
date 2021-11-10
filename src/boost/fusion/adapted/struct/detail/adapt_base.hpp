/*=============================================================================
    Copyright (c) 2001-2009 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden
    Copyright (c) 2009-2011 Christopher Schmidt
    Copyright (c) 2013-2014 Damien Buhl

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_BASE_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_ADAPT_BASE_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/config.hpp>
#include <boost/fusion/support/tag_of_fwd.hpp>
#include <boost/fusion/adapted/struct/detail/adapt_auto.hpp>
#include <boost/fusion/adapted/struct/detail/adapt_is_tpl.hpp>

#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/comparison/less.hpp>
#include <boost/preprocessor/logical/not.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/tag.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/add_reference.hpp>

#include <boost/typeof/typeof.hpp>


#define BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME_TEMPLATE_PARAMS(SEQ)              \
    BOOST_PP_SEQ_HEAD(SEQ)<BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TAIL(SEQ))>           \
    BOOST_PP_EMPTY()

#define BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(SEQ)                              \
    BOOST_PP_IF(                                                                \
        BOOST_PP_SEQ_HEAD(SEQ),                                                 \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME_TEMPLATE_PARAMS,                  \
        BOOST_PP_SEQ_HEAD)(BOOST_PP_SEQ_TAIL(SEQ))

#define BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL_C(R, _, ELEM)     \
    (typename ELEM)
#define BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL(SEQ)              \
    BOOST_PP_SEQ_ENUM(                                                          \
        BOOST_PP_SEQ_FOR_EACH(                                                  \
            BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL_C,            \
            _,                                                                  \
            BOOST_PP_SEQ_TAIL(SEQ)))
#define BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(SEQ)                   \
    BOOST_PP_IF(                                                                \
        BOOST_PP_SEQ_HEAD(SEQ),                                                 \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL,                  \
        BOOST_PP_TUPLE_EAT(1))(SEQ)

#ifdef BOOST_MSVC
#   define BOOST_FUSION_ATTRIBUTE_TYPEOF(                                       \
        NAME_SEQ, ATTRIBUTE, ATTRIBUTE_TUPLE_SIZE, PREFIX, TEMPLATE_PARAMS_SEQ) \
                                                                                \
    BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS(                    \
        TEMPLATE_PARAMS_SEQ)                                                    \
                                                                                \
    struct deduced_attr_type {                                                  \
      static const BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)& obj;        \
      typedef                                                                   \
      BOOST_PP_IF(BOOST_FUSION_ADAPT_IS_TPL(TEMPLATE_PARAMS_SEQ), typename, )   \
      BOOST_TYPEOF( PREFIX() obj.BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPLE_SIZE,      \
            0, ATTRIBUTE))                                                      \
      type;                                                                     \
    };                                                                          \
                                                                                \
    typedef                                                                     \
        BOOST_PP_IF(BOOST_FUSION_ADAPT_IS_TPL(TEMPLATE_PARAMS_SEQ), typename, ) \
        deduced_attr_type::type attribute_type;

#else 
#   define BOOST_FUSION_ATTRIBUTE_TYPEOF(                                       \
        NAME_SEQ, ATTRIBUTE, ATTRIBUTE_TUPLE_SIZE, PREFIX, TEMPLATE_PARAMS_SEQ) \
                                                                                \
    struct deduced_attr_type {                                                  \
      static const BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)& obj;        \
      typedef BOOST_TYPEOF(                                                     \
          PREFIX() obj.BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPLE_SIZE, 0, ATTRIBUTE)) \
      type;                                                                     \
    };                                                                          \
                                                                                \
    typedef                                                                     \
        BOOST_PP_IF(BOOST_FUSION_ADAPT_IS_TPL(TEMPLATE_PARAMS_SEQ), typename, ) \
        deduced_attr_type::type attribute_type;

#endif

#define BOOST_FUSION_ATTRIBUTE_GIVENTYPE(                                       \
    NAME_SEQ, ATTRIBUTE, ATTRIBUTE_TUPLE_SIZE, PREFIX, TEMPLATE_PARAMS_SEQ)     \
    typedef                                                                     \
        BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPLE_SIZE, 0, ATTRIBUTE) attribute_type;
   

#ifdef BOOST_NO_PARTIAL_SPECIALIZATION_IMPLICIT_DEFAULT_ARGS
#   define BOOST_FUSION_ADAPT_STRUCT_TAG_OF_SPECIALIZATION(                     \
        MODIFIER, TEMPLATE_PARAMS_SEQ, NAME_SEQ, TAG)                           \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct tag_of<                                                              \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ) MODIFIER                \
      , void                                                                    \
    >                                                                           \
    {                                                                           \
        typedef TAG type;                                                       \
    };
#else
#   define BOOST_FUSION_ADAPT_STRUCT_TAG_OF_SPECIALIZATION(                     \
        MODIFIER, TEMPLATE_PARAMS_SEQ, NAME_SEQ, TAG)                           \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct tag_of<BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ) MODIFIER>     \
    {                                                                           \
        typedef TAG type;                                                       \
    };
#endif

#define BOOST_FUSION_ADAPT_STRUCT_BASE_UNPACK_AND_CALL(R,DATA,I,ATTRIBUTE)      \
    BOOST_PP_TUPLE_ELEM(4,0,DATA)(                                              \
        BOOST_PP_TUPLE_ELEM(4,1,DATA),                                          \
        BOOST_PP_TUPLE_ELEM(4,2,DATA),                                          \
        BOOST_PP_TUPLE_ELEM(4,3,DATA),                                          \
        I,                                                                      \
        ATTRIBUTE)

#ifdef BOOST_MSVC
#   define BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAM(R,_,ELEM)     \
        typedef ELEM ELEM;
#   define BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS_IMPL(SEQ)    \
        BOOST_PP_SEQ_FOR_EACH(                                                  \
            BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAM,             \
            _,                                                                  \
            BOOST_PP_SEQ_TAIL(SEQ))
#   define BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS(SEQ)         \
        BOOST_PP_IF(                                                            \
            BOOST_PP_SEQ_HEAD(SEQ),                                             \
            BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS_IMPL,       \
            BOOST_PP_TUPLE_EAT(1))(SEQ)
#else
#   define BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS(SEQ)
#endif

#define BOOST_FUSION_ADAPT_STRUCT_C_BASE(                                       \
    TEMPLATE_PARAMS_SEQ,NAME_SEQ,IS_VIEW,                                       \
    I,PREFIX,ATTRIBUTE,ATTRIBUTE_TUPLE_SIZE,                                    \
    DEDUCE_TYPE)                                                                \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct access::struct_member<                                               \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)                         \
      , I                                                                       \
    >                                                                           \
    {                                                                           \
        BOOST_PP_IF(DEDUCE_TYPE,                                                \
            BOOST_FUSION_ATTRIBUTE_TYPEOF, BOOST_FUSION_ATTRIBUTE_GIVENTYPE)(   \
                NAME_SEQ,                                                       \
                ATTRIBUTE,                                                      \
                ATTRIBUTE_TUPLE_SIZE,                                           \
                PREFIX,                                                         \
                TEMPLATE_PARAMS_SEQ)                                            \
                                                                                \
        BOOST_FUSION_ADAPT_STRUCT_MSVC_REDEFINE_TEMPLATE_PARAMS(                \
            TEMPLATE_PARAMS_SEQ)                                                \
                                                                                \
        typedef attribute_type type;                                            \
                                                                                \
        template<typename Seq>                                                  \
        struct apply                                                            \
        {                                                                       \
            typedef typename                                                    \
                add_reference<                                                  \
                    typename mpl::eval_if<                                      \
                        is_const<Seq>                                           \
                      , add_const<attribute_type>                               \
                      , mpl::identity<attribute_type>                           \
                    >::type                                                     \
                >::type                                                         \
            type;                                                               \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type                                                         \
            call(Seq& seq)                                                      \
            {                                                                   \
                return seq.PREFIX()                                             \
                    BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPLE_SIZE,                   \
                        BOOST_PP_NOT(DEDUCE_TYPE), ATTRIBUTE);                  \
            }                                                                   \
        };                                                                      \
    };                                                                          \
                                                                                \
    template<                                                                   \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(TEMPLATE_PARAMS_SEQ)   \
    >                                                                           \
    struct struct_member_name<                                                  \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)                         \
      , I                                                                       \
    >                                                                           \
    {                                                                           \
        typedef char const* type;                                               \
                                                                                \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        static type                                                             \
        call()                                                                  \
        {                                                                       \
            return BOOST_PP_STRINGIZE(                                          \
               BOOST_PP_TUPLE_ELEM(ATTRIBUTE_TUPLE_SIZE,                        \
                   BOOST_PP_NOT(DEDUCE_TYPE), ATTRIBUTE));                      \
        }                                                                       \
    };

#define BOOST_FUSION_ADAPT_STRUCT_BASE(                                         \
    TEMPLATE_PARAMS_SEQ,                                                        \
    NAME_SEQ,                                                                   \
    TAG,                                                                        \
    IS_VIEW,                                                                    \
    ATTRIBUTES_SEQ,                                                             \
    ATTRIBUTES_CALLBACK)                                                        \
                                                                                \
namespace boost                                                                 \
{                                                                               \
    namespace fusion                                                            \
    {                                                                           \
        namespace traits                                                        \
        {                                                                       \
            BOOST_FUSION_ADAPT_STRUCT_TAG_OF_SPECIALIZATION(                    \
                BOOST_PP_EMPTY(), TEMPLATE_PARAMS_SEQ, NAME_SEQ, TAG)           \
            BOOST_FUSION_ADAPT_STRUCT_TAG_OF_SPECIALIZATION(                    \
                const, TEMPLATE_PARAMS_SEQ, NAME_SEQ, TAG)                      \
        }                                                                       \
                                                                                \
        namespace extension                                                     \
        {                                                                       \
            BOOST_PP_IF(                                                        \
                BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ)),                \
                BOOST_PP_SEQ_FOR_EACH_I_R,                                      \
                BOOST_PP_TUPLE_EAT(4))(                                         \
                    1,                                                          \
                    BOOST_FUSION_ADAPT_STRUCT_BASE_UNPACK_AND_CALL,             \
                    (ATTRIBUTES_CALLBACK,TEMPLATE_PARAMS_SEQ,NAME_SEQ, IS_VIEW),\
                    BOOST_PP_SEQ_TAIL(ATTRIBUTES_SEQ))                          \
                                                                                \
            template<                                                           \
                BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(               \
                    TEMPLATE_PARAMS_SEQ)                                        \
            >                                                                   \
            struct struct_size<BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)> \
              : mpl::int_<BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ))>      \
            {};                                                                 \
                                                                                \
            template<                                                           \
                BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(               \
                    TEMPLATE_PARAMS_SEQ)                                        \
            >                                                                   \
            struct struct_is_view<                                              \
                BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)                 \
            >                                                                   \
              : mpl::BOOST_PP_IIF(IS_VIEW,true_,false_)                         \
            {};                                                                 \
        }                                                                       \
    }                                                                           \
                                                                                \
    namespace mpl                                                               \
    {                                                                           \
        template<typename>                                                      \
        struct sequence_tag;                                                    \
                                                                                \
        template<                                                               \
            BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(                   \
                TEMPLATE_PARAMS_SEQ)                                            \
        >                                                                       \
        struct sequence_tag<BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ)>    \
        {                                                                       \
            typedef fusion::fusion_sequence_tag type;                           \
        };                                                                      \
                                                                                \
        template<                                                               \
            BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS(                   \
                TEMPLATE_PARAMS_SEQ)                                            \
        >                                                                       \
        struct sequence_tag<                                                    \
            BOOST_FUSION_ADAPT_STRUCT_UNPACK_NAME(NAME_SEQ) const               \
        >                                                                       \
        {                                                                       \
            typedef fusion::fusion_sequence_tag type;                           \
        };                                                                      \
    }                                                                           \
}

#endif
