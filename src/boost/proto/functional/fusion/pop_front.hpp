///////////////////////////////////////////////////////////////////////////////
/// \file pop_front.hpp
/// Proto callables Fusion pop_front
//
//  Copyright 2010 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PROTO_FUNCTIONAL_FUSION_POP_FRONT_HPP_EAN_11_27_2010
#define BOOST_PROTO_FUNCTIONAL_FUSION_POP_FRONT_HPP_EAN_11_27_2010

#include <boost/fusion/include/begin.hpp>
#include <boost/fusion/include/end.hpp>
#include <boost/fusion/include/next.hpp>
#include <boost/fusion/include/pop_front.hpp>
#include <boost/proto/proto_fwd.hpp>

namespace boost { namespace proto { namespace functional
{
    /// \brief A PolymorphicFunctionObject type that invokes the
    /// \c fusion::pop_front() algorithm on its argument.
    ///
    /// A PolymorphicFunctionObject type that invokes the
    /// \c fusion::pop_front() algorithm on its argument. This is
    /// useful for defining a CallableTransform like \c pop_front(_)
    /// which removes the first child from a Proto expression node.
    /// Such a transform might be used as the first argument to the
    /// \c proto::fold\<\> transform; that is, fold all but
    /// the first child.
    struct pop_front
    {
        BOOST_PROTO_CALLABLE()

        template<typename Sig>
        struct result;

        template<typename This, typename Seq>
        struct result<This(Seq)>
          : result<This(Seq const &)>
        {};

        template<typename This, typename Seq>
        struct result<This(Seq &)>
          : fusion::result_of::pop_front<Seq>
        {};

        template<typename Seq>
        typename fusion::result_of::pop_front<Seq>::type
        operator ()(Seq &seq) const
        {
            // Work around a const-correctness issue in Fusion
            typedef typename fusion::result_of::pop_front<Seq>::type result_type;
            return result_type(fusion::next(fusion::begin(seq)), fusion::end(seq));
        }

        template<typename Seq>
        typename fusion::result_of::pop_front<Seq const>::type
        operator ()(Seq const &seq) const
        {
            return fusion::pop_front(seq);
        }
    };
}}}

#endif
