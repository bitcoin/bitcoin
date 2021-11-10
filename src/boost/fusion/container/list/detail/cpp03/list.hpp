/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_LIST_07172005_1153)
#define FUSION_LIST_07172005_1153

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/void.hpp>
#include <boost/fusion/support/detail/enabler.hpp>
#include <boost/fusion/container/list/detail/cpp03/list_fwd.hpp>
#include <boost/fusion/container/list/detail/cpp03/list_to_cons.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/core/enable_if.hpp>

#if !defined(BOOST_FUSION_DONT_USE_PREPROCESSED_FILES)
#include <boost/fusion/container/list/detail/cpp03/preprocessed/list.hpp>
#else
#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 2, line: 0, output: "preprocessed/list" FUSION_MAX_LIST_SIZE_STR ".hpp")
#endif

/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    This is an auto-generated file. Do not edit!
==============================================================================*/

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(preserve: 1)
#endif

namespace boost { namespace fusion
{
    struct nil_;
    struct void_;

    template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename T)>
    struct list
        : detail::list_to_cons<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, T)>::type
    {
    private:
        typedef
            detail::list_to_cons<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, T)>
        list_to_cons;
        typedef typename list_to_cons::type inherited_type;

    public:
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list()
            : inherited_type() {}

        template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename U)>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list(list<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, U)> const& rhs)
            : inherited_type(rhs) {}

        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence const& rhs
            , typename enable_if<traits::is_sequence<Sequence>, detail::enabler_>::type = detail::enabler)
            : inherited_type(rhs) {}

        //  Expand a couple of forwarding constructors for arguments
        //  of type (T0), (T0, T1), (T0, T1, T2) etc. Exanple:
        //
        //  list(
        //      typename detail::call_param<T0>::type arg0
        //    , typename detail::call_param<T1>::type arg1)
        //    : inherited_type(list_to_cons::call(arg0, arg1)) {}
        #include <boost/fusion/container/list/detail/cpp03/list_forward_ctor.hpp>

        template <BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, typename U)>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(list<BOOST_PP_ENUM_PARAMS(FUSION_MAX_LIST_SIZE, U)> const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }

        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename boost::enable_if<traits::is_sequence<Sequence>, list&>::type
        operator=(Sequence const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
    };
}}

#if defined(__WAVE__) && defined(BOOST_FUSION_CREATE_PREPROCESSED_FILES)
#pragma wave option(output: null)
#endif

#endif // BOOST_FUSION_DONT_USE_PREPROCESSED_FILES

#endif
