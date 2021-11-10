/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_ERASE_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_ERASE_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that calss the erase method.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does):
    //      ref.erase( value );
    //      ref.erase( T::key_type(first,last) );
    //      ref.erase( key_ref );
    //
    //  Policy name:
    //      erase_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_value_actor, erase_a( ref );
    //      ref_const_ref_actor, erase_a( ref, key_ref );
    //
    //  () operators: both
    //
    //  See also ref_value_actor and ref_const_ref_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct erase_action
    {
        template<
            typename T,
            typename KeyT
        >
        void act(T& ref_, KeyT const& key_) const
        {
            ref_.erase(key_);
        }
        template<
            typename T,
            typename IteratorT
        >
        void act(
            T& ref_,
            IteratorT const& first_,
            IteratorT const& last_
            ) const
        {
            typedef typename T::key_type key_type;
            key_type key(first_,last_);

            ref_.erase(key);
        }
    };

    template<typename T>
    inline ref_value_actor<T,erase_action> erase_a(T& ref_)
    {
        return ref_value_actor<T,erase_action>(ref_);
    }

    template<
        typename T,
        typename KeyT
    >
    inline ref_const_ref_actor<T,KeyT,erase_action> erase_a(
        T& ref_,
        KeyT const& key_
    )
    {
        return ref_const_ref_actor<T,KeyT,erase_action>(ref_,key_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
