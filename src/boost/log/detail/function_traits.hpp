/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   function_traits.hpp
 * \author Andrey Semashev
 * \date   30.08.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_FUNCTION_TRAITS_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_FUNCTION_TRAITS_HPP_INCLUDED_

#include <boost/mpl/has_xxx.hpp>
#include <boost/log/detail/config.hpp>

#if defined(BOOST_NO_SFINAE) || defined(BOOST_MPL_CFG_NO_HAS_XXX)
#   if !defined(BOOST_LOG_NO_FUNCTION_TRAITS)
#      define BOOST_LOG_NO_FUNCTION_TRAITS
#   endif
#else

#include <boost/mpl/int.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/is_nonmember_callable_builtin.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

    //  A number of traits to deal with functors
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_argument_type, argument_type, false)
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_first_argument_type, first_argument_type, false)
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_second_argument_type, second_argument_type, false)
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_arg1_type, arg1_type, false)
    BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(has_arg2_type, arg2_type, false)

    namespace has_arity_no_adl {

        typedef char yes_type;
        struct no_type
        {
            char dummy[2];
        };

        template< typename FunT, int ArityV = FunT::arity >
        struct checker
        {
        };

        template< typename FunT >
        yes_type has_arity_impl(FunT const&, checker< FunT >*);
        template< typename FunT >
        no_type has_arity_impl(FunT const&, ...);

    } // namespace has_arity_no_adl

    //! The metafunction detects if the type has an arity static constant member
    template< typename FunT >
    struct has_arity
    {
        static FunT const& get_FunT();

        enum value_t { value = (sizeof(has_arity_no_adl::has_arity_impl(get_FunT(), 0)) == sizeof(has_arity_no_adl::yes_type)) };
        typedef mpl::bool_< value > type;
    };

    //! The metafunction results in an unqualified type with removed reference
    template< typename T >
    struct root_type :
        public remove_cv<
            typename remove_reference<
                T
            >::type
        >
    {
    };

    template<
        typename FunT,
        bool = function_types::is_nonmember_callable_builtin< FunT >::value,
        bool = has_argument_type< FunT >::value,
        bool = has_first_argument_type< FunT >::value,
        bool = has_arg1_type< FunT >::value
    >
    struct first_argument_type_of_impl
    {
    };
    template< typename FunT >
    struct first_argument_type_of_impl< FunT, true, false, false, false >
    {
        typedef typename root_type<
            typename mpl::front<
                typename function_types::parameter_types< FunT >::type
            >::type
        >::type type;
    };
    template< typename FunT, bool HasFirstArgumentV, bool HasArg1V >
    struct first_argument_type_of_impl< FunT, false, true, HasFirstArgumentV, HasArg1V >
    {
        typedef typename root_type<
            typename FunT::argument_type
        >::type type;
    };
    template< typename FunT, bool HasArg1V >
    struct first_argument_type_of_impl< FunT, false, false, true, HasArg1V >
    {
        typedef typename root_type<
            typename FunT::first_argument_type
        >::type type;
    };
    template< typename FunT >
    struct first_argument_type_of_impl< FunT, false, false, false, true >
    {
        typedef typename root_type<
            typename FunT::arg1_type
        >::type type;
    };

    //! The metafunction returns the first argument type of a function
    template< typename FunT >
    struct first_argument_type_of :
        public first_argument_type_of_impl< FunT >
    {
    };


    template<
        typename FunT,
        bool = function_types::is_nonmember_callable_builtin< FunT >::value,
        bool = has_second_argument_type< FunT >::value,
        bool = has_arg2_type< FunT >::value
    >
    struct second_argument_type_of_impl
    {
    };
    template< typename FunT >
    struct second_argument_type_of_impl< FunT, true, false, false >
    {
        typedef typename root_type<
            typename mpl::front<
                typename mpl::pop_front<
                    typename function_types::parameter_types< FunT >::type
                >::type
            >::type
        >::type type;
    };
    template< typename FunT, bool HasArg2V >
    struct second_argument_type_of_impl< FunT, false, true, HasArg2V >
    {
        typedef typename root_type<
            typename FunT::second_argument_type
        >::type type;
    };
    template< typename FunT >
    struct second_argument_type_of_impl< FunT, false, false, true >
    {
        typedef typename root_type<
            typename FunT::arg2_type
        >::type type;
    };

    //! The metafunction returns the second argument type of a function
    template< typename FunT >
    struct second_argument_type_of :
        public second_argument_type_of_impl< FunT >
    {
    };


    template<
        typename FunT,
        bool = function_types::is_nonmember_callable_builtin< FunT >::value,
        bool = has_arity< FunT >::value,
        bool = has_argument_type< FunT >::value,
        bool = has_second_argument_type< FunT >::value
    >
    struct arity_of_impl
    {
    };
    template< typename FunT >
    struct arity_of_impl< FunT, true, false, false, false > :
        public function_types::function_arity< FunT >
    {
    };
    template< typename FunT, bool HasArgumentTypeV, bool HasSecondArgumentTypeV >
    struct arity_of_impl< FunT, false, true, HasArgumentTypeV, HasSecondArgumentTypeV > :
        public mpl::int_< FunT::arity >
    {
    };
    template< typename FunT, bool HasArgumentTypeV >
    struct arity_of_impl< FunT, false, false, HasArgumentTypeV, true > :
        public mpl::int_< 2 >
    {
    };
    template< typename FunT >
    struct arity_of_impl< FunT, false, false, true, false > :
        public mpl::int_< 1 >
    {
    };

    //! The metafunction returns the arity of a function
    template< typename FunT >
    struct arity_of :
        public arity_of_impl< FunT >
    {
    };

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // defined(BOOST_NO_SFINAE) || defined(BOOST_MPL_CFG_NO_HAS_XXX)

#endif // BOOST_LOG_DETAIL_FUNCTION_TRAITS_HPP_INCLUDED_
