/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_SWAP_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_SWAP_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_value_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that swaps values.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does):
    //      ref.swap( value_ref );
    //
    //  Policy name:
    //      swap_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_value_actor, swap_a( ref );
    //      ref_const_ref_actor, swap_a( ref, value_ref );
    //
    //  () operators: both
    //
    //  See also ref_value_actor and ref_const_ref_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    template<
        typename T
    >
    class swap_actor
    {
    private:
        T& ref;
        T& swap_ref;

    public:
        swap_actor(
            T& ref_,
            T& swap_ref_)
            : ref(ref_), swap_ref(swap_ref_)
        {};

        template<typename T2>
        void operator()(T2 const& /*val*/) const
        {
            ref.swap(swap_ref);
        }


        template<typename IteratorT>
        void operator()(
            IteratorT const& /*first*/,
            IteratorT const& /*last*/
            ) const
        {
            ref.swap(swap_ref);
        }
    };

    template<
        typename T
    >
    inline swap_actor<T> swap_a(
        T& ref_,
        T& swap_ref_
    )
    {
        return swap_actor<T>(ref_,swap_ref_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
