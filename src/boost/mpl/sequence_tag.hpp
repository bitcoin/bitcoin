
#ifndef BOOST_MPL_SEQUENCE_TAG_HPP_INCLUDED
#define BOOST_MPL_SEQUENCE_TAG_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id$
// $Date$
// $Revision$

#include <boost/mpl/sequence_tag_fwd.hpp>
#include <boost/mpl/aux_/has_tag.hpp>
#include <boost/mpl/aux_/has_begin.hpp>
#include <boost/mpl/aux_/na_spec.hpp>
#include <boost/mpl/aux_/is_msvc_eti_arg.hpp>
#include <boost/mpl/aux_/config/eti.hpp>
#include <boost/mpl/aux_/yes_no.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>

namespace boost { namespace mpl {

// agurt, 27/nov/02: have to use a simplistic 'sequence_tag' implementation
// on MSVC to avoid dreadful "internal structure overflow" error
#if BOOST_WORKAROUND(BOOST_MSVC, < 1300) \
    || defined(BOOST_MPL_CFG_NO_HAS_XXX)

template<
      typename BOOST_MPL_AUX_NA_PARAM(Sequence)
    >
struct sequence_tag
{
    typedef typename Sequence::tag type;
};

#elif BOOST_WORKAROUND(BOOST_MSVC, == 1300)

// agurt, 07/feb/03: workaround for what seems to be MSVC 7.0-specific ETI issue

namespace aux {

template< bool >
struct sequence_tag_impl
{
    template< typename Sequence > struct result_
    {
        typedef typename Sequence::tag type;
    };
};

template<>
struct sequence_tag_impl<false>
{
    template< typename Sequence > struct result_
    {
        typedef int type;
    };
};

} // namespace aux

template<
      typename BOOST_MPL_AUX_NA_PARAM(Sequence)
    >
struct sequence_tag
    : aux::sequence_tag_impl< !aux::is_msvc_eti_arg<Sequence>::value >
        ::template result_<Sequence>
{
};

#else

namespace aux {

template< bool has_tag_, bool has_begin_ >
struct sequence_tag_impl
{
    // agurt 24/nov/02: MSVC 6.5 gets confused in 'sequence_tag_impl<true>' 
    // specialization below, if we name it 'result_' here
    template< typename Sequence > struct result2_;
};

#   define AUX_CLASS_SEQUENCE_TAG_SPEC(has_tag, has_begin, result_type) \
template<> struct sequence_tag_impl<has_tag,has_begin> \
{ \
    template< typename Sequence > struct result2_ \
    { \
        typedef result_type type; \
    }; \
}; \
/**/

AUX_CLASS_SEQUENCE_TAG_SPEC(true, true, typename Sequence::tag)
AUX_CLASS_SEQUENCE_TAG_SPEC(true, false, typename Sequence::tag)
AUX_CLASS_SEQUENCE_TAG_SPEC(false, true, nested_begin_end_tag)
AUX_CLASS_SEQUENCE_TAG_SPEC(false, false, non_sequence_tag)

#   undef AUX_CLASS_SEQUENCE_TAG_SPEC

} // namespace aux

template<
      typename BOOST_MPL_AUX_NA_PARAM(Sequence)
    >
struct sequence_tag
    : aux::sequence_tag_impl<
          ::boost::mpl::aux::has_tag<Sequence>::value
        , ::boost::mpl::aux::has_begin<Sequence>::value
        >::template result2_<Sequence>
{
};

#endif // BOOST_MSVC

BOOST_MPL_AUX_NA_SPEC(1, sequence_tag)

}}

#endif // BOOST_MPL_SEQUENCE_TAG_HPP_INCLUDED
