/*=============================================================================
    Copyright (c) 2012 Nathan Ridge

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ADAPTED_STRUCT_DETAIL_DEFINE_STRUCT_INLINE_HPP
#define BOOST_FUSION_ADAPTED_STRUCT_DETAIL_DEFINE_STRUCT_INLINE_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/config.hpp>
#include <boost/fusion/support/category_of.hpp>
#include <boost/fusion/sequence/sequence_facade.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>
#include <boost/fusion/algorithm/auxiliary/copy.hpp>
#include <boost/fusion/adapted/struct/detail/define_struct.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

// MSVC and GCC <= 4.4 have a bug that affects partial specializations of
// nested templates under some circumstances. This affects the implementation
// of BOOST_FUSION_DEFINE_STRUCT_INLINE, which uses such specializations for
// the iterator class's 'deref' and 'value_of' metafunctions. On these compilers
// an alternate implementation for these metafunctions is used that does not 
// require such specializations. The alternate implementation takes longer
// to compile so its use is restricted to the offending compilers.
// For MSVC, the bug was reported at https://connect.microsoft.com/VisualStudio/feedback/details/757891/c-compiler-error-involving-partial-specializations-of-nested-templates
// For GCC, 4.4 and earlier are no longer maintained so there is no need
// to report a bug.
#if defined(BOOST_MSVC) || (defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ <= 4)))
    #define BOOST_FUSION_NEED_NESTED_TEMPLATE_PARTIAL_SPEC_WKND 
#endif

#ifdef BOOST_FUSION_NEED_NESTED_TEMPLATE_PARTIAL_SPEC_WKND
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/if.hpp>
#include <boost/fusion/sequence/intrinsic/at_c.hpp>
#include <boost/fusion/container/vector.hpp>
#endif


#define BOOST_FUSION_MAKE_DEFAULT_INIT_LIST_ENTRY(R, DATA, N, ATTRIBUTE)        \
    BOOST_PP_COMMA_IF(N) BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)()

#define BOOST_FUSION_MAKE_DEFAULT_INIT_LIST(ATTRIBUTES_SEQ)                     \
            : BOOST_PP_SEQ_FOR_EACH_I(                                          \
              BOOST_FUSION_MAKE_DEFAULT_INIT_LIST_ENTRY,                        \
              ~,                                                                \
              ATTRIBUTES_SEQ)                                                   \

#define BOOST_FUSION_IGNORE_2(ARG1, ARG2)

#define BOOST_FUSION_MAKE_COPY_CONSTRUCTOR(NAME, ATTRIBUTES_SEQ)                \
    BOOST_FUSION_GPU_ENABLED                                                    \
    NAME(BOOST_PP_SEQ_FOR_EACH_I(                                               \
            BOOST_FUSION_MAKE_CONST_REF_PARAM,                                  \
            ~,                                                                  \
            ATTRIBUTES_SEQ))                                                    \
        : BOOST_PP_SEQ_FOR_EACH_I(                                              \
              BOOST_FUSION_MAKE_INIT_LIST_ENTRY,                                \
              ~,                                                                \
              ATTRIBUTES_SEQ)                                                   \
    {                                                                           \
    }                                                                           \

#define BOOST_FUSION_MAKE_CONST_REF_PARAM(R, DATA, N, ATTRIBUTE)                \
    BOOST_PP_COMMA_IF(N)                                                        \
    BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) const&                                 \
    BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)

#define BOOST_FUSION_MAKE_INIT_LIST_ENTRY_I(NAME) NAME(NAME)

#define BOOST_FUSION_MAKE_INIT_LIST_ENTRY(R, DATA, N, ATTRIBUTE)                \
    BOOST_PP_COMMA_IF(N)                                                        \
    BOOST_FUSION_MAKE_INIT_LIST_ENTRY_I(BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE))

#define BOOST_FUSION_ITERATOR_NAME(NAME)                                        \
    BOOST_PP_CAT(boost_fusion_detail_, BOOST_PP_CAT(NAME, _iterator))

// Note: all template parameter names need to be uglified, otherwise they might
//       shadow a template parameter of the struct when used with
//       BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE

#define BOOST_FUSION_MAKE_ITERATOR_VALUE_OF_SPECS(Z, N, NAME)                   \
    template <typename boost_fusion_detail_Sq>                                  \
    struct value_of<                                                            \
               BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq, N>      \
           >                                                                    \
        : boost::mpl::identity<                                                 \
              typename boost_fusion_detail_Sq::t##N##_type                      \
          >                                                                     \
    {                                                                           \
    };

#define BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                  \
    SPEC_TYPE, CALL_ARG_TYPE, TYPE_QUAL, ATTRIBUTE, N)                          \
                                                                                \
    template <typename boost_fusion_detail_Sq>                                  \
    struct deref<SPEC_TYPE, N> >                                                \
    {                                                                           \
        typedef typename boost_fusion_detail_Sq::t##N##_type TYPE_QUAL& type;   \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        static type call(CALL_ARG_TYPE, N> const& iter)                         \
        {                                                                       \
            return iter.seq_.BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);              \
        }                                                                       \
    };

#define BOOST_FUSION_MAKE_ITERATOR_DEREF_SPECS(R, NAME, N, ATTRIBUTE)           \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq,                \
        BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq,                \
        ,                                                                       \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        BOOST_FUSION_ITERATOR_NAME(NAME)<const boost_fusion_detail_Sq,          \
        BOOST_FUSION_ITERATOR_NAME(NAME)<const boost_fusion_detail_Sq,          \
        const,                                                                  \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        const BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq,          \
        BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq,                \
        ,                                                                       \
        ATTRIBUTE,                                                              \
        N)                                                                      \
    BOOST_FUSION_MAKE_ITERATOR_DEREF_SPEC(                                      \
        const BOOST_FUSION_ITERATOR_NAME(NAME)<const boost_fusion_detail_Sq,    \
        BOOST_FUSION_ITERATOR_NAME(NAME)<const boost_fusion_detail_Sq,          \
        const,                                                                  \
        ATTRIBUTE,                                                              \
        N)                                                                      \

#define BOOST_FUSION_MAKE_VALUE_AT_SPECS(Z, N, DATA)                            \
    template <typename boost_fusion_detail_Sq>                                  \
    struct value_at<boost_fusion_detail_Sq, boost::mpl::int_<N> >               \
    {                                                                           \
        typedef typename boost_fusion_detail_Sq::t##N##_type type;              \
    };

#define BOOST_FUSION_MAKE_AT_SPECS(R, DATA, N, ATTRIBUTE)                       \
    template <typename boost_fusion_detail_Sq>                                  \
    struct at<boost_fusion_detail_Sq, boost::mpl::int_<N> >                     \
    {                                                                           \
        typedef typename boost::mpl::if_<                                       \
            boost::is_const<boost_fusion_detail_Sq>,                            \
            typename boost_fusion_detail_Sq::t##N##_type const&,                \
            typename boost_fusion_detail_Sq::t##N##_type&                       \
        >::type type;                                                           \
                                                                                \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        static type call(boost_fusion_detail_Sq& sq)                            \
        {                                                                       \
            return sq. BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);                    \
        }                                                                       \
    };

#define BOOST_FUSION_MAKE_TYPEDEF(R, DATA, N, ATTRIBUTE)                        \
    typedef BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) t##N##_type;

#define BOOST_FUSION_MAKE_DATA_MEMBER(R, DATA, N, ATTRIBUTE)                    \
    BOOST_PP_TUPLE_ELEM(2, 0, ATTRIBUTE) BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE);

#ifdef BOOST_FUSION_NEED_NESTED_TEMPLATE_PARTIAL_SPEC_WKND

#define BOOST_FUSION_DEFINE_ITERATOR_VALUE_OF(NAME, ATTRIBUTE_SEQ_SIZE)         \
        template <typename boost_fusion_detail_Iterator>                        \
        struct value_of : boost::fusion::result_of::at_c<                       \
                              ref_vec_t,                                        \
                              boost_fusion_detail_Iterator::index::value        \
                          >                                                     \
        {                                                                       \
        };

#define BOOST_FUSION_DEFINE_ITERATOR_DEREF(NAME, ATTRIBUTES_SEQ)                \
        template <typename boost_fusion_detail_Iterator>                        \
        struct deref                                                            \
        {                                                                       \
            typedef typename boost::remove_const<                               \
                boost_fusion_detail_Iterator                                    \
            >::type iterator_raw_type;                                          \
                                                                                \
            static const int index = iterator_raw_type::index::value;           \
                                                                                \
            typedef typename boost::fusion::result_of::at_c<                    \
                ref_vec_t,                                                      \
                index                                                           \
            >::type result_raw_type;                                            \
                                                                                \
            typedef typename boost::mpl::if_<                                   \
                boost::is_const<typename iterator_raw_type::sequence_type>,     \
                typename boost::add_const<result_raw_type>::type,               \
                result_raw_type                                                 \
            >::type type;                                                       \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type call(iterator_raw_type const& iter)                     \
            {                                                                   \
                return boost::fusion::at_c<index>(iter.ref_vec);                \
            }                                                                   \
        };

#define BOOST_FUSION_MAKE_ITERATOR_WKND_FIELD_NAME(R, DATA, N, ATTRIBUTE)       \
        BOOST_PP_COMMA_IF(N) seq.BOOST_PP_TUPLE_ELEM(2, 1, ATTRIBUTE)

#define BOOST_FUSION_DEFINE_ITERATOR_WKND_INIT_LIST_ENTRIES(ATTRIBUTES_SEQ)     \
        , ref_vec(BOOST_PP_SEQ_FOR_EACH_I(                                      \
                          BOOST_FUSION_MAKE_ITERATOR_WKND_FIELD_NAME,           \
                          ~,                                                    \
                          BOOST_PP_SEQ_TAIL(ATTRIBUTES_SEQ)))

#define BOOST_FUSION_MAKE_ITERATOR_WKND_REF(Z, N, DATA)                         \
        BOOST_PP_COMMA_IF(N)                                                    \
        typename boost::mpl::if_<                                               \
                boost::is_const<boost_fusion_detail_Seq>,                       \
                typename boost::add_const<                                      \
                        typename boost_fusion_detail_Seq::t##N##_type           \
                >::type,                                                        \
                typename boost_fusion_detail_Seq::t##N##_type                   \
        >::type&

#define BOOST_FUSION_DEFINE_ITERATOR_WKND_MEMBERS(ATTRIBUTES_SEQ_SIZE)          \
        typedef boost::fusion::vector<                                          \
            BOOST_PP_REPEAT(                                                    \
                    ATTRIBUTES_SEQ_SIZE,                                        \
                    BOOST_FUSION_MAKE_ITERATOR_WKND_REF,                        \
                    ~)                                                          \
        > ref_vec_t;                                                            \
                                                                                \
        ref_vec_t ref_vec;

#else

#define BOOST_FUSION_DEFINE_ITERATOR_VALUE_OF(NAME, ATTRIBUTES_SEQ_SIZE)        \
        template <typename boost_fusion_detail_T> struct value_of;              \
        BOOST_PP_REPEAT(                                                        \
            ATTRIBUTES_SEQ_SIZE,                                                \
            BOOST_FUSION_MAKE_ITERATOR_VALUE_OF_SPECS,                          \
            NAME)

#define BOOST_FUSION_DEFINE_ITERATOR_DEREF(NAME, ATTRIBUTES_SEQ)                \
        template <typename boost_fusion_detail_T> struct deref;                 \
        BOOST_PP_SEQ_FOR_EACH_I(                                                \
            BOOST_FUSION_MAKE_ITERATOR_DEREF_SPECS,                             \
            NAME,                                                               \
            ATTRIBUTES_SEQ)

#define BOOST_FUSION_DEFINE_ITERATOR_WKND_INIT_LIST_ENTRIES(ATTRIBUTES_SEQ)

#define BOOST_FUSION_DEFINE_ITERATOR_WKND_MEMBERS(ATTRIBUTES_SEQ_SIZE)

#endif  // BOOST_FUSION_NEED_NESTED_TEMPLATE_PARTIAL_SPEC_WKND

// Note: We can't nest the iterator inside the struct because we run into
//       a MSVC10 bug involving partial specializations of nested templates.

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_IMPL(NAME, ATTRIBUTES)                \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_ITERATOR(NAME, ATTRIBUTES)                \
    struct NAME : boost::fusion::sequence_facade<                               \
                      NAME,                                                     \
                      boost::fusion::random_access_traversal_tag                \
                  >                                                             \
    {                                                                           \
        BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    };

#define BOOST_FUSION_DEFINE_TPL_STRUCT_INLINE_IMPL(                             \
    TEMPLATE_PARAMS_SEQ, NAME, ATTRIBUTES)                                      \
                                                                                \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_ITERATOR(NAME, ATTRIBUTES)                \
                                                                                \
    template <                                                                  \
        BOOST_FUSION_ADAPT_STRUCT_UNPACK_TEMPLATE_PARAMS_IMPL(                  \
            (0)TEMPLATE_PARAMS_SEQ)                                             \
    >                                                                           \
    struct NAME : boost::fusion::sequence_facade<                               \
                      NAME<                                                     \
                          BOOST_PP_SEQ_ENUM(TEMPLATE_PARAMS_SEQ)                \
                      >,                                                        \
                      boost::fusion::random_access_traversal_tag                \
                  >                                                             \
    {                                                                           \
        BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    };

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS(NAME, ATTRIBUTES)             \
    BOOST_FUSION_DEFINE_STRUCT_MEMBERS_IMPL(                                    \
        NAME,                                                                   \
        BOOST_PP_CAT(BOOST_FUSION_DEFINE_STRUCT_FILLER_0 ATTRIBUTES,_END))

// Note: can't compute BOOST_PP_SEQ_SIZE(ATTRIBUTES_SEQ) directly because
//       ATTRIBUTES_SEQ may be empty and calling BOOST_PP_SEQ_SIZE on an empty
//       sequence produces warnings on MSVC.
#define BOOST_FUSION_DEFINE_STRUCT_MEMBERS_IMPL(NAME, ATTRIBUTES_SEQ)           \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS_IMPL_IMPL(                        \
        NAME,                                                                   \
        ATTRIBUTES_SEQ,                                                         \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE((0)ATTRIBUTES_SEQ)))

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_ITERATOR(NAME, ATTRIBUTES)            \
    BOOST_FUSION_DEFINE_STRUCT_ITERATOR_IMPL(                                   \
        NAME,                                                                   \
        BOOST_PP_CAT(BOOST_FUSION_DEFINE_STRUCT_FILLER_0 ATTRIBUTES,_END))

#define BOOST_FUSION_DEFINE_STRUCT_ITERATOR_IMPL(NAME, ATTRIBUTES_SEQ)          \
    BOOST_FUSION_DEFINE_STRUCT_INLINE_ITERATOR_IMPL_IMPL(                       \
        NAME,                                                                   \
        ATTRIBUTES_SEQ,                                                         \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE((0)ATTRIBUTES_SEQ)))

#define BOOST_FUSION_DEFINE_STRUCT_INLINE_ITERATOR_IMPL_IMPL(                   \
    NAME, ATTRIBUTES_SEQ, ATTRIBUTES_SEQ_SIZE)                                  \
                                                                                \
    template <typename boost_fusion_detail_Seq, int N>                          \
    struct BOOST_FUSION_ITERATOR_NAME(NAME)                                     \
        : boost::fusion::iterator_facade<                                       \
              BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Seq, N>,     \
              boost::fusion::random_access_traversal_tag                        \
          >                                                                     \
    {                                                                           \
        typedef boost::mpl::int_<N> index;                                      \
        typedef boost_fusion_detail_Seq sequence_type;                          \
                                                                                \
        BOOST_FUSION_GPU_ENABLED                                                \
        BOOST_FUSION_ITERATOR_NAME(NAME)(boost_fusion_detail_Seq& seq)          \
            : seq_(seq)                                                         \
              BOOST_FUSION_DEFINE_ITERATOR_WKND_INIT_LIST_ENTRIES(              \
                      (0)ATTRIBUTES_SEQ)                                        \
        {}                                                                      \
                                                                                \
        boost_fusion_detail_Seq& seq_;                                          \
                                                                                \
        BOOST_FUSION_DEFINE_ITERATOR_WKND_MEMBERS(ATTRIBUTES_SEQ_SIZE)          \
                                                                                \
        BOOST_FUSION_DEFINE_ITERATOR_VALUE_OF(NAME, ATTRIBUTES_SEQ_SIZE)        \
                                                                                \
        BOOST_FUSION_DEFINE_ITERATOR_DEREF(NAME, ATTRIBUTES_SEQ)                \
                                                                                \
        template <typename boost_fusion_detail_It>                              \
        struct next                                                             \
        {                                                                       \
            typedef BOOST_FUSION_ITERATOR_NAME(NAME)<                           \
                typename boost_fusion_detail_It::sequence_type,                 \
                boost_fusion_detail_It::index::value + 1                        \
            > type;                                                             \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type call(boost_fusion_detail_It const& it)                  \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <typename boost_fusion_detail_It>                              \
        struct prior                                                            \
        {                                                                       \
            typedef BOOST_FUSION_ITERATOR_NAME(NAME)<                           \
                typename boost_fusion_detail_It::sequence_type,                 \
                boost_fusion_detail_It::index::value - 1                        \
            > type;                                                             \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type call(boost_fusion_detail_It const& it)                  \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <                                                              \
            typename boost_fusion_detail_It1,                                   \
            typename boost_fusion_detail_It2                                    \
        >                                                                       \
        struct distance                                                         \
        {                                                                       \
            typedef typename boost::mpl::minus<                                 \
                typename boost_fusion_detail_It2::index,                        \
                typename boost_fusion_detail_It1::index                         \
            >::type type;                                                       \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type call(boost_fusion_detail_It1 const& /* it1 */,          \
                             boost_fusion_detail_It2 const& /* it2 */)          \
            {                                                                   \
                return type();                                                  \
            }                                                                   \
        };                                                                      \
                                                                                \
        template <                                                              \
            typename boost_fusion_detail_It,                                    \
            typename boost_fusion_detail_M                                      \
        >                                                                       \
        struct advance                                                          \
        {                                                                       \
            typedef BOOST_FUSION_ITERATOR_NAME(NAME)<                           \
                typename boost_fusion_detail_It::sequence_type,                 \
                boost_fusion_detail_It::index::value                            \
                    + boost_fusion_detail_M::value                              \
            > type;                                                             \
                                                                                \
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                            \
            static type call(boost_fusion_detail_It const& it)                  \
            {                                                                   \
                return type(it.seq_);                                           \
            }                                                                   \
        };                                                                      \
    };


#define BOOST_FUSION_DEFINE_STRUCT_INLINE_MEMBERS_IMPL_IMPL(                    \
    NAME, ATTRIBUTES_SEQ, ATTRIBUTES_SEQ_SIZE)                                  \
                                                                                \
    NAME()                                                                      \
        BOOST_PP_IF(ATTRIBUTES_SEQ_SIZE,                                        \
            BOOST_FUSION_MAKE_DEFAULT_INIT_LIST,                                \
            BOOST_PP_EMPTY)(ATTRIBUTES_SEQ)                                     \
    {                                                                           \
    }                                                                           \
                                                                                \
    BOOST_PP_IF(                                                                \
        ATTRIBUTES_SEQ_SIZE,                                                    \
        BOOST_FUSION_MAKE_COPY_CONSTRUCTOR,                                     \
        BOOST_FUSION_IGNORE_2)                                                  \
            (NAME, ATTRIBUTES_SEQ)                                              \
                                                                                \
    template <typename boost_fusion_detail_Seq>                                 \
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED                              \
    NAME(const boost_fusion_detail_Seq& rhs)                                    \
    {                                                                           \
        boost::fusion::copy(rhs, *this);                                        \
    }                                                                           \
                                                                                \
    template <typename boost_fusion_detail_Seq>                                 \
    BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED                              \
    NAME& operator=(const boost_fusion_detail_Seq& rhs)                         \
    {                                                                           \
        boost::fusion::copy(rhs, *this);                                        \
        return *this;                                                           \
    }                                                                           \
                                                                                \
    template <typename boost_fusion_detail_Sq>                                  \
    struct begin                                                                \
    {                                                                           \
        typedef BOOST_FUSION_ITERATOR_NAME(NAME)<boost_fusion_detail_Sq, 0>     \
             type;                                                              \
                                                                                \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        static type call(boost_fusion_detail_Sq& sq)                            \
        {                                                                       \
            return type(sq);                                                    \
        }                                                                       \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_detail_Sq>                                  \
    struct end                                                                  \
    {                                                                           \
        typedef BOOST_FUSION_ITERATOR_NAME(NAME)<                               \
            boost_fusion_detail_Sq,                                             \
            ATTRIBUTES_SEQ_SIZE                                                 \
        > type;                                                                 \
                                                                                \
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED                                \
        static type call(boost_fusion_detail_Sq& sq)                            \
        {                                                                       \
            return type(sq);                                                    \
        }                                                                       \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_detail_Sq>                                  \
    struct size : boost::mpl::int_<ATTRIBUTES_SEQ_SIZE>                         \
    {                                                                           \
    };                                                                          \
                                                                                \
    template <typename boost_fusion_detail_Sq>                                  \
    struct empty : boost::mpl::bool_<ATTRIBUTES_SEQ_SIZE == 0>                  \
    {                                                                           \
    };                                                                          \
                                                                                \
    template <                                                                  \
        typename boost_fusion_detail_Sq,                                        \
        typename boost_fusion_detail_N                                          \
    >                                                                           \
    struct value_at : value_at<                                                 \
                          boost_fusion_detail_Sq,                               \
                          boost::mpl::int_<boost_fusion_detail_N::value>        \
                      >                                                         \
    {                                                                           \
    };                                                                          \
                                                                                \
    BOOST_PP_REPEAT(                                                            \
        ATTRIBUTES_SEQ_SIZE,                                                    \
        BOOST_FUSION_MAKE_VALUE_AT_SPECS,                                       \
        ~)                                                                      \
                                                                                \
    template <                                                                  \
        typename boost_fusion_detail_Sq,                                        \
        typename boost_fusion_detail_N                                          \
    >                                                                           \
    struct at : at<                                                             \
                    boost_fusion_detail_Sq,                                     \
                    boost::mpl::int_<boost_fusion_detail_N::value>              \
                >                                                               \
    {                                                                           \
    };                                                                          \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_FUSION_MAKE_AT_SPECS, ~, ATTRIBUTES_SEQ)      \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(BOOST_FUSION_MAKE_TYPEDEF, ~, ATTRIBUTES_SEQ)       \
                                                                                \
    BOOST_PP_SEQ_FOR_EACH_I(                                                    \
        BOOST_FUSION_MAKE_DATA_MEMBER,                                          \
        ~,                                                                      \
        ATTRIBUTES_SEQ)

#endif
