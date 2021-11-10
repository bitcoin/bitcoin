//-----------------------------------------------------------------------------
// boost variant/detail/initializer.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
// Copyright (c) 2002-2003
// Eric Friedman, Itay Maman
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_VARIANT_DETAIL_INITIALIZER_HPP
#define BOOST_VARIANT_DETAIL_INITIALIZER_HPP

#include <new> // for placement new

#include <boost/config.hpp>

#include <boost/call_traits.hpp>
#include <boost/detail/reference_content.hpp>
#include <boost/variant/recursive_wrapper_fwd.hpp>
#include <boost/variant/detail/move.hpp>

#if !defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)
#   include <boost/mpl/aux_/value_wknd.hpp>
#   include <boost/mpl/int.hpp>
#   include <boost/mpl/iter_fold.hpp>
#   include <boost/mpl/next.hpp>
#   include <boost/mpl/deref.hpp>
#   include <boost/mpl/pair.hpp>
#   include <boost/mpl/protect.hpp>
#else
#   include <boost/variant/variant_fwd.hpp>
#   include <boost/preprocessor/cat.hpp>
#   include <boost/preprocessor/enum.hpp>
#   include <boost/preprocessor/repeat.hpp>
#endif

namespace boost {
namespace detail { namespace variant {

///////////////////////////////////////////////////////////////////////////////
// (detail) support to simulate standard overload resolution rules
//
// The below initializers allows variant to follow standard overload
// resolution rules over the specified set of bounded types.
//
// On compilers where using declarations in class templates can correctly
// avoid name hiding, use an optimal solution based on the variant's typelist.
//
// Otherwise, use a preprocessor workaround based on knowledge of the fixed
// size of the variant's psuedo-variadic template parameter list.
//

#if !defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

// (detail) quoted metafunction make_initializer_node
//
// Exposes a pair whose first type is a node in the initializer hierarchy.
//
struct make_initializer_node
{
    template <typename BaseIndexPair, typename Iterator>
    struct apply
    {
    private: // helpers, for metafunction result (below)

        typedef typename BaseIndexPair::first
            base;
        typedef typename BaseIndexPair::second
            index;

        class initializer_node
            : public base
        {
        private: // helpers, for static functions (below)

            typedef typename mpl::deref<Iterator>::type
                recursive_enabled_T;
            typedef typename unwrap_recursive<recursive_enabled_T>::type
                public_T;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            typedef boost::is_reference<public_T> 
                is_reference_content_t;

            typedef typename boost::mpl::if_<is_reference_content_t, public_T, const public_T& >::type 
                param_T;

            template <class T> struct disable_overload{};

            typedef typename boost::mpl::if_<is_reference_content_t, disable_overload<public_T>, public_T&& >::type 
                param2_T;
#else
            typedef typename call_traits<public_T>::param_type
                param_T;
#endif

        public: // static functions

            using base::initialize;

            static int initialize(void* dest, param_T operand)
            {
                typedef typename boost::detail::make_reference_content<
                      recursive_enabled_T
                    >::type internal_T;

                new(dest) internal_T(operand);
                return BOOST_MPL_AUX_VALUE_WKND(index)::value; // which
            }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
            static int initialize(void* dest, param2_T operand)
            {
                // This assert must newer trigger, because all the reference contents are
                // handled by the initilize(void* dest, param_T operand) function above
                BOOST_ASSERT(!is_reference_content_t::value);

                typedef typename boost::mpl::if_<is_reference_content_t, param2_T, recursive_enabled_T>::type value_T;
                new(dest) value_T( boost::detail::variant::move(operand) );
                return BOOST_MPL_AUX_VALUE_WKND(index)::value; // which
            }
#endif
        };

        friend class initializer_node;

    public: // metafunction result

        typedef mpl::pair<
              initializer_node
            , typename mpl::next< index >::type
            > type;

    };
};

// (detail) class initializer_root
//
// Every level of the initializer hierarchy must expose the name
// "initialize," so initializer_root provides a dummy function:
//
class initializer_root
{
public: // static functions

    static void initialize();

};

#else // defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

    // Obsolete. Remove.
    #define BOOST_VARIANT_AUX_PP_INITIALIZER_TEMPLATE_PARAMS \
          BOOST_VARIANT_ENUM_PARAMS(typename recursive_enabled_T) \
    /**/

    // Obsolete. Remove.
    #define BOOST_VARIANT_AUX_PP_INITIALIZER_DEFINE_PARAM_T(N) \
        typedef typename unwrap_recursive< \
              BOOST_PP_CAT(recursive_enabled_T,N) \
            >::type BOOST_PP_CAT(public_T,N); \
        typedef typename call_traits< \
              BOOST_PP_CAT(public_T,N) \
            >::param_type BOOST_PP_CAT(param_T,N); \
    /**/

template < BOOST_VARIANT_ENUM_PARAMS(typename recursive_enabled_T) >
struct preprocessor_list_initializer
{
public: // static functions

    #define BOOST_VARIANT_AUX_PP_INITIALIZE_FUNCTION(z,N,_) \
        typedef typename unwrap_recursive< \
              BOOST_PP_CAT(recursive_enabled_T,N) \
            >::type BOOST_PP_CAT(public_T,N); \
        typedef typename call_traits< \
              BOOST_PP_CAT(public_T,N) \
            >::param_type BOOST_PP_CAT(param_T,N); \
        static int initialize( \
              void* dest \
            , BOOST_PP_CAT(param_T,N) operand \
            ) \
        { \
            typedef typename boost::detail::make_reference_content< \
                  BOOST_PP_CAT(recursive_enabled_T,N) \
                >::type internal_T; \
            \
            new(dest) internal_T(operand); \
            return (N); /*which*/ \
        } \
        /**/

    BOOST_PP_REPEAT(
          BOOST_VARIANT_LIMIT_TYPES
        , BOOST_VARIANT_AUX_PP_INITIALIZE_FUNCTION
        , _
        )

    #undef BOOST_VARIANT_AUX_PP_INITIALIZE_FUNCTION

};

#endif // BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE workaround

}} // namespace detail::variant
} // namespace boost

///////////////////////////////////////////////////////////////////////////////
// macro BOOST_VARIANT_AUX_INITIALIZER_T
//
// Given both the variant's typelist and a basename for forming the list of
// bounded types (i.e., T becomes T1, T2, etc.), exposes the initializer
// most appropriate to the current compiler.
//

#if !defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

#define BOOST_VARIANT_AUX_INITIALIZER_T( mpl_seq, typename_base ) \
    ::boost::mpl::iter_fold< \
          mpl_seq \
        , ::boost::mpl::pair< \
              ::boost::detail::variant::initializer_root \
            , ::boost::mpl::int_<0> \
            > \
        , ::boost::mpl::protect< \
              ::boost::detail::variant::make_initializer_node \
            > \
        >::type::first \
    /**/

#else // defined(BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE)

    // Obsolete. Remove.
    #define BOOST_VARIANT_AUX_PP_INITIALIZER_TEMPLATE_ARGS(typename_base) \
          BOOST_VARIANT_ENUM_PARAMS(typename_base) \
        /**/

#define BOOST_VARIANT_AUX_INITIALIZER_T( mpl_seq, typename_base ) \
    ::boost::detail::variant::preprocessor_list_initializer< \
          BOOST_VARIANT_ENUM_PARAMS(typename_base) \
        > \
    /**/

#endif // BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE workaround

#endif // BOOST_VARIANT_DETAIL_INITIALIZER_HPP
