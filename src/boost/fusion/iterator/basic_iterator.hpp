/*=============================================================================
    Copyright (c) 2009 Christopher Schmidt

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#ifndef BOOST_FUSION_ITERATOR_BASIC_ITERATOR_HPP
#define BOOST_FUSION_ITERATOR_BASIC_ITERATOR_HPP

#include <boost/fusion/support/config.hpp>
#include <boost/fusion/iterator/iterator_facade.hpp>

#include <boost/mpl/and.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/minus.hpp>
#include <boost/mpl/int.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace fusion
{
    namespace extension
    {
        template <typename>
        struct value_of_impl;

        template <typename>
        struct deref_impl;

        template <typename>
        struct value_of_data_impl;

        template <typename>
        struct key_of_impl;

        template <typename>
        struct deref_data_impl;
    }

    template<typename Tag, typename Category, typename Seq, int Index>
    struct basic_iterator
      : iterator_facade<basic_iterator<Tag, Category, Seq, Index>, Category>
    {
        typedef mpl::int_<Index> index;
        typedef Seq seq_type;

        template <typename It>
        struct value_of
          : extension::value_of_impl<Tag>::template apply<It>
        {};

        template <typename It>
        struct deref
          : extension::deref_impl<Tag>::template apply<It>
        {};

        template <typename It>
        struct value_of_data
          : extension::value_of_data_impl<Tag>::template apply<It>
        {};

        template <typename It>
        struct key_of
          : extension::key_of_impl<Tag>::template apply<It>
        {};

        template <typename It>
        struct deref_data
          : extension::deref_data_impl<Tag>::template apply<It>
        {};

        template <typename It, typename N>
        struct advance
        {
            typedef
                basic_iterator<Tag, Category, Seq, Index + N::value>
            type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static type
            call(It const& it)
            {
                return type(*it.seq,0);
            }
        };

        template <typename It>
        struct next
          : advance<It, mpl::int_<1> >
        {};

        template <typename It>
        struct prior
          : advance<It, mpl::int_<-1> >
        {};

        template <typename It1, typename It2>
        struct distance
        {
            typedef mpl::minus<typename It2::index, typename It1::index> type;

            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            static
            type
            call(It1 const&, It2 const&)
            {
                return type();
            }
        };

        template <typename It1, typename It2>
        struct equal_to
          : mpl::and_<
                is_same<
                    typename remove_const<typename It1::seq_type>::type
                  , typename remove_const<typename It2::seq_type>::type
                >
              , mpl::equal_to<typename It1::index,typename It2::index>
            >
        {};

        template<typename OtherSeq>
        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        basic_iterator(basic_iterator<Tag,Category,OtherSeq,Index> const& it)
          : seq(it.seq)
        {}

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        basic_iterator(Seq& in_seq, int)
          : seq(&in_seq)
        {}

        template<typename OtherSeq>
        BOOST_CXX14_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        basic_iterator&
        operator=(basic_iterator<Tag,Category,OtherSeq,Index> const& it)
        {
            seq=it.seq;
            return *this;
        }

        Seq* seq;
    };
}}

#ifdef BOOST_FUSION_WORKAROUND_FOR_LWG_2408
namespace std
{
    template <typename Tag, typename Category, typename Seq, int Index>
    struct iterator_traits< ::boost::fusion::basic_iterator<Tag, Category, Seq, Index> >
    { };
}
#endif

#endif
