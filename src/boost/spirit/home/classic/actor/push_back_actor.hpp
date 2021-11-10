/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_PUSH_BACK_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_PUSH_BACK_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //
    //  A semantic action policy that appends a value to the back of a
    //  container.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does and what ref, value_ref must support):
    //      ref.push_back( value );
    //      ref.push_back( T::value_type(first,last) );
    //      ref.push_back( value_ref );
    //
    //  Policy name:
    //      push_back_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_value_actor, push_back_a( ref );
    //      ref_const_ref_actor, push_back_a( ref, value_ref );
    //
    //  () operators: both
    //
    //  See also ref_value_actor and ref_const_ref_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct push_back_action
    {
        template<
            typename T,
            typename ValueT
        >
        void act(T& ref_, ValueT const& value_) const
        {
            ref_.push_back( value_ );
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
            typedef typename T::value_type value_type;
            value_type value(first_,last_);

            ref_.push_back( value );
        }
    };

//  Deprecated interface. Use push_back_a
    template<typename T>
    inline ref_value_actor<T,push_back_action> 
    append(T& ref_)
    {
        return ref_value_actor<T,push_back_action>(ref_);
    }

    template<typename T>
    inline ref_value_actor<T,push_back_action> 
    push_back_a(T& ref_)
    {
        return ref_value_actor<T,push_back_action>(ref_);
    }

    template<
        typename T,
        typename ValueT
    >
    inline ref_const_ref_actor<T,ValueT,push_back_action> 
    push_back_a(
        T& ref_,
        ValueT const& value_
    )
    {
        return ref_const_ref_actor<T,ValueT,push_back_action>(ref_,value_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
