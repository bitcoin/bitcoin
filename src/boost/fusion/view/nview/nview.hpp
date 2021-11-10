/*=============================================================================
    Copyright (c) 2009 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#if !defined(BOOST_FUSION_NVIEW_SEP_23_2009_0948PM)
#define BOOST_FUSION_NVIEW_SEP_23_2009_0948PM

#include <boost/fusion/support/config.hpp>
#include <boost/mpl/if.hpp>

#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>

#include <boost/fusion/support/is_view.hpp>
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>
#include <boost/fusion/view/transform_view.hpp>

#include <boost/config.hpp>

namespace boost { namespace fusion
{
    namespace detail
    {
        struct addref
        {
            template<typename Sig>
            struct result;

            template<typename U>
            struct result<addref(U)> : add_reference<U> {};

#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
            template <typename T>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename add_reference<T>::type 
            operator()(T& x) const
            {
                return x;
            }
#else
            template <typename T>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename result<addref(T)>::type
            operator()(T&& x) const
            {
                return x;
            }
#endif
        };

        struct addconstref
        {
            template<typename Sig>
            struct result;

            template<typename U>
            struct result<addconstref(U)> 
              : add_reference<typename add_const<U>::type> 
            {};

            template <typename T>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename add_reference<typename add_const<T>::type>::type 
            operator()(T& x) const
            {
                return x;
            }

            template <typename T>
            BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
            typename add_reference<typename add_const<T>::type>::type 
            operator()(T const& x) const
            {
                return x;
            }
        };
    }

    struct nview_tag;
    struct random_access_traversal_tag;
    struct fusion_sequence_tag;

    template<typename Sequence, typename Indicies>
    struct nview
      : sequence_base<nview<Sequence, Indicies> >
    {
        typedef nview_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef random_access_traversal_tag category;

        typedef mpl::true_ is_view;
        typedef Indicies index_type;
        typedef typename result_of::size<Indicies>::type size;

        typedef typename mpl::if_<
            is_const<Sequence>, detail::addconstref, detail::addref
        >::type transform_type;
        typedef transform_view<Sequence, transform_type> transform_view_type;
        typedef typename result_of::as_vector<transform_view_type>::type 
            sequence_type;

        BOOST_CONSTEXPR BOOST_FUSION_GPU_ENABLED
        explicit nview(Sequence& val)
          : seq(sequence_type(transform_view_type(val, transform_type()))) 
        {}

        sequence_type seq;
    };

}}

// define the nview() generator functions
#include <boost/fusion/view/nview/detail/nview_impl.hpp>

#endif


