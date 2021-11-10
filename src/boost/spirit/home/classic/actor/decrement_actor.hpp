/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_DECREMENT_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_DECREMENT_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that calls the -- operator on a reference.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions:
    //      --ref;
    //
    //  Policy name:
    //      decrement_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_actor, decrement_a( ref );
    //
    //  () operators: both.
    //
    //  See also ref_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct decrement_action
    {
        template<
            typename T
        >
        void act(T& ref_) const
        {
            --ref_;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // helper method that creates a and_assign_actor.
    ///////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline ref_actor<T,decrement_action> decrement_a(T& ref_)
    {
        return ref_actor<T,decrement_action>(ref_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
