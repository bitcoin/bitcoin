/*=============================================================================
    Copyright (c) 2005-2013 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_FUSION_MAP_IMPL_02032013_2233)
#define BOOST_FUSION_MAP_IMPL_02032013_2233

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/iterator/deref.hpp>
#include <boost/fusion/iterator/next.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace fusion
{
    struct fusion_sequence_tag;
}}

namespace boost { namespace fusion { namespace detail
{
    struct map_impl_from_iterator {};

    template <int index, typename ...T>
    struct map_impl;

    template <int index_>
    struct map_impl<index_>
    {
        typedef fusion_sequence_tag tag;
        static int const index = index_;
        static int const size = 0;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl() BOOST_NOEXCEPT {}

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(Iterator const&, map_impl_from_iterator) BOOST_NOEXCEPT
        {}

        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        void assign(Iterator const&, map_impl_from_iterator) BOOST_NOEXCEPT
        {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        void get();
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        void get_val();
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        void get_key();
    };

    template <int index_, typename Pair, typename ...T>
    struct map_impl<index_, Pair, T...> : map_impl<index_ + 1, T...>
    {
        typedef fusion_sequence_tag tag;
        typedef map_impl<index_+1, T...> rest_type;

        using rest_type::get;
        using rest_type::get_val;
        using rest_type::get_key;

        static int const index = index_;
        static int const size = rest_type::size + 1;

        typedef Pair pair_type;
        typedef typename Pair::first_type key_type;
        typedef typename Pair::second_type value_type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl()
          : rest_type(), element()
        {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(map_impl const& rhs)
          : rest_type(rhs.get_base()), element(rhs.element)
        {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(map_impl&& rhs)
          : rest_type(BOOST_FUSION_FWD_ELEM(rest_type, *static_cast<rest_type*>(&rhs)))
          , element(BOOST_FUSION_FWD_ELEM(Pair, rhs.element))
        {}

        template <typename ...U>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(map_impl<index, U...> const& rhs)
          : rest_type(rhs.get_base()), element(rhs.element)
        {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(typename detail::call_param<Pair>::type element_
          , typename detail::call_param<T>::type... rest)
          : rest_type(rest...), element(element_)
        {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(Pair&& element_, T&&... rest)
          : rest_type(BOOST_FUSION_FWD_ELEM(T, rest)...)
          , element(BOOST_FUSION_FWD_ELEM(Pair, element_))
        {}

        template <typename Iterator>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl(Iterator const& iter, map_impl_from_iterator fi)
          : rest_type(fusion::next(iter), fi)
          , element(*iter)
        {}

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        rest_type& get_base()
        {
            return *this;
        }

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        rest_type const& get_base() const
        {
            return *this;
        }

        BOOST_FUSION_GPU_ENABLED
        mpl::identity<value_type> get_val(mpl::identity<key_type>) const;
        BOOST_FUSION_GPU_ENABLED
        pair_type get_val(mpl::int_<index>) const;

        BOOST_FUSION_GPU_ENABLED
        mpl::identity<key_type> get_key(mpl::int_<index>);
        BOOST_FUSION_GPU_ENABLED
        mpl::identity<key_type> get_key(mpl::int_<index>) const;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename cref_result<value_type>::type
        get(mpl::identity<key_type>) const
        {
            return element.second;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename ref_result<value_type>::type
        get(mpl::identity<key_type>)
        {
            return element.second;
        }

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename cref_result<pair_type>::type
        get(mpl::int_<index>) const
        {
            return element;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        typename ref_result<pair_type>::type
        get(mpl::int_<index>)
        {
            return element;
        }

        template <typename ...U>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl& operator=(map_impl<index, U...> const& rhs)
        {
            rest_type::operator=(rhs);
            element = rhs.element;
            return *this;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl& operator=(map_impl const& rhs)
        {
            rest_type::operator=(rhs);
            element = rhs.element;
            return *this;
        }

        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        map_impl& operator=(map_impl&& rhs)
        {
            rest_type::operator=(std::forward<map_impl>(rhs));
            element = BOOST_FUSION_FWD_ELEM(Pair, rhs.element);
            return *this;
        }

        template <typename Iterator>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        void assign(Iterator const& iter, map_impl_from_iterator fi)
        {
            rest_type::assign(fusion::next(iter), fi);
            element = *iter;
        }

        Pair element;
    };
}}}

#endif
