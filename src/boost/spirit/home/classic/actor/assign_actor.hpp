/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_ASSIGN_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_ASSIGN_ACTOR_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/spirit/home/classic/actor/ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_actor.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    ///////////////////////////////////////////////////////////////////////////
    //  Summary:
    //  A semantic action policy that applies the assignment operator.
    //  (This doc uses convention available in actors.hpp)
    //
    //  Actions (what it does):
    //      ref = value;
    //      ref = T(first,last);
    //      ref = value_ref;
    //
    //  Policy name:
    //      assign_action
    //
    //  Policy holder, corresponding helper method:
    //      ref_value_actor, assign_a( ref );
    //      ref_const_ref_actor, assign_a( ref, value_ref );
    //
    //  () operators: both
    //
    //  See also ref_value_actor and ref_const_ref_actor for more details.
    ///////////////////////////////////////////////////////////////////////////
    struct assign_action
    {
        template<
            typename T,
            typename ValueT
        >
        void act(T& ref_, ValueT const& value_) const
        {
            ref_ = value_;
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
            typedef T value_type;
#ifndef BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS
            value_type value(first_,last_);
#else
            value_type value;
            std::copy(first_, last_, std::inserter(value, value.end()));
#endif
            ref_ = value;
        }
    };

    // Deprecated. Please use assign_a
    template<typename T>
    inline ref_value_actor<T,assign_action> assign(T& ref_)
    {
        return ref_value_actor<T,assign_action>(ref_);
    }

    template<typename T>
    inline ref_value_actor<T,assign_action> assign_a(T& ref_)
    {
        return ref_value_actor<T,assign_action>(ref_);
    }

    template<
        typename T,
        typename ValueT
    >
    inline ref_const_ref_actor<T,ValueT,assign_action> assign_a(
        T& ref_,
        ValueT const& value_
    )
    {
        return ref_const_ref_actor<T,ValueT,assign_action>(ref_,value_);
    }

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}}

#endif
