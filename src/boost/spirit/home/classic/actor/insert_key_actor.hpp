/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_INSERT_KEY_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_INSERT_KEY_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_value_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that insert data into an associative
    //  container using a const reference to data.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does):
    //      ref.insert( T::value_type(value,value_ref) );
    //      ref.insert( T::value_type(T::key_type(first,last), value_ref));;
    //
    //  Policy name:
    //      insert_key_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_const_ref_value_actor, insert_key_a( ref, value_ref );
    //
    //  () operators: both
    //
    //  See also ref_const_ref_value_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct insert_key_action
    {
        template<
            typename T,
            typename ValueT,
            typename ReferentT
        >
        void act(
            T& ref_,
            ValueT const& value_,
            ReferentT const& key_
            ) const
        {
            typedef typename T::value_type value_type;
            value_type key_value(key_, value_);
            ref_.insert( key_value );
        }

        template<
            typename T,
            typename ValueT,
            typename IteratorT
        >
        void act(
            T& ref_,
            ValueT const& value_,
            IteratorT const& first_,
            IteratorT const& last_
            ) const
        {
            typedef typename T::key_type key_type;
            typedef typename T::value_type value_type;

            key_type key(first_,last_);
            value_type key_value(key, value_);
            ref_.insert( key_value );
        }
    };

    template<
        typename T,
        typename ValueT
        >
    inline ref_const_ref_value_actor<T,ValueT,insert_key_action> insert_key_a(
        T& ref_,
        ValueT const& value_
        )
    {
        return ref_const_ref_value_actor<
            T,
            ValueT,
            insert_key_action
            >(ref_,value_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
