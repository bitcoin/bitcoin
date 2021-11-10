/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_ASSIGN_KEY_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_ASSIGN_KEY_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_const_ref_a.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    struct assign_key_action
    {
        template<
            typename T,
            typename ValueT,
            typename KeyT
        >
        void act(T& ref_, ValueT const& value_, KeyT const& key_) const
        {
            ref_[ key_ ] = value_;
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
            key_type key(first_,last_);

            ref_[key] = value_;
        }
    };

    template<
        typename T,
        typename ValueT
    >
    inline ref_const_ref_value_actor<T,ValueT,assign_key_action>
        assign_key_a(T& ref_, ValueT const& value_)
    {
        return ref_const_ref_value_actor<T,ValueT,assign_key_action>(
            ref_,
            value_
            );
    }

    template<
        typename T,
        typename ValueT,
        typename KeyT
    >
    inline ref_const_ref_const_ref_actor<
        T,
        ValueT,
        KeyT,
        assign_key_action
    >
        assign_key_a(
            T& ref_,
            ValueT const& value_,
            KeyT const& key_
    )
    {
        return ref_const_ref_const_ref_actor<
            T,
            ValueT,
            KeyT,
            assign_key_action
        >(
            ref_,
            value_,
            key_
            );
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
