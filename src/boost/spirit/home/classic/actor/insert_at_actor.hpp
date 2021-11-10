/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_INSERT_AT_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_INSERT_AT_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_const_ref_a.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that insert data into an associative 
    //  container using a const reference to a key.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does):
    //      ref.insert( T::value_type(key_ref,value) );
    //      ref.insert( T::value_type(key_ref, T::mapped_type(first,last)));;
    //      ref.insert( T::value_type(key_ref,value_ref) );
    //
    //  Policy name:
    //      insert_at_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_const_ref_value_actor, insert_at_a( ref, key_ref ); 
    //      ref_const_ref_const_ref_actor, insert_a( ref, key_ref, value_ref );
    //
    //  () operators: both
    //
    //  See also ref_const_ref_value_actor and ref_const_ref_const_ref_actor 
    //  for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct insert_at_action
    {
        template<
            typename T,
            typename ReferentT,
            typename ValueT
        >
        void act(
            T& ref_, 
            ReferentT const& key_,
            ValueT const& value_
            ) const
        {
            typedef typename T::value_type value_type;
            ref_.insert( value_type(key_, value_) );
        }

        template<
            typename T,
            typename ReferentT,
            typename IteratorT
        >
        void act(
            T& ref_, 
            ReferentT const& key_,
            IteratorT const& first_, 
            IteratorT const& last_
            ) const
        {
            typedef typename T::mapped_type mapped_type;
            typedef typename T::value_type value_type;

            mapped_type value(first_,last_);
            value_type key_value(key_, value);
            ref_.insert( key_value );
        }
    };

    template<
        typename T,
        typename ReferentT
        >
    inline ref_const_ref_value_actor<T,ReferentT,insert_at_action> 
    insert_at_a(
            T& ref_,
            ReferentT const& key_
        )
    {
        return ref_const_ref_value_actor<
            T,
            ReferentT,
            insert_at_action
            >(ref_,key_);
    }

    template<
        typename T,
        typename ReferentT,
        typename ValueT
    >
    inline ref_const_ref_const_ref_actor<T,ReferentT,ValueT,insert_at_action> 
    insert_at_a(
                T& ref_,
                ReferentT const& key_,
                ValueT const& value_
            )
    {
        return ref_const_ref_const_ref_actor<
            T,
            ReferentT,
            ValueT,
            insert_at_action
            >(ref_,key_,value_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
