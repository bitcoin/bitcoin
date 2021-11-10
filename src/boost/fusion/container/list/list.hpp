/*=============================================================================
    Copyright (c) 2014-2015 Kohei Takahashi

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_LIST_10262014_0537
#define FUSION_LIST_10262014_0537

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/container/list/list_fwd.hpp>

///////////////////////////////////////////////////////////////////////////////
// Without variadics, we will use the PP version
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_FUSION_HAS_VARIADIC_LIST)
# include <boost/fusion/container/list/detail/cpp03/list.hpp>
#else

///////////////////////////////////////////////////////////////////////////////
// C++11 interface
///////////////////////////////////////////////////////////////////////////////
#include <utility>
#include <boost/fusion/container/list/detail/list_to_cons.hpp>

namespace boost { namespace fusion
{
    struct nil_;

    template <>
    struct list<>
        : detail::list_to_cons<>::type
    {
    private:
        typedef detail::list_to_cons<> list_to_cons;
        typedef list_to_cons::type inherited_type;

    public:
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list()
            : inherited_type() {}

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence const& rhs)
            : inherited_type(rhs) {}

        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(Sequence const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
#else
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence&& rhs)
            : inherited_type(std::forward<Sequence>(rhs)) {}

        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(Sequence&& rhs)
        {
            inherited_type::operator=(std::forward<Sequence>(rhs));
            return *this;
        }
#endif
    };

    template <typename ...T>
    struct list
        : detail::list_to_cons<T...>::type
    {
    private:
        typedef detail::list_to_cons<T...> list_to_cons;
        typedef typename list_to_cons::type inherited_type;

    public:
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list()
            : inherited_type() {}

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence const& rhs)
            : inherited_type(rhs) {}
#else
        template <typename Sequence>
        BOOST_FUSION_GPU_ENABLED
        list(Sequence&& rhs)
            : inherited_type(std::forward<Sequence>(rhs)) {}
#endif

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        explicit
        list(typename detail::call_param<T>::type ...args)
            : inherited_type(list_to_cons::call(args...)) {}

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(Sequence const& rhs)
        {
            inherited_type::operator=(rhs);
            return *this;
        }
#else
        template <typename Sequence>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        list&
        operator=(Sequence&& rhs)
        {
            inherited_type::operator=(std::forward<Sequence>(rhs));
            return *this;
        }
#endif
    };
}}

#endif
#endif
