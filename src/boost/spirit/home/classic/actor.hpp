/*=============================================================================
    Copyright (c) 2003 Jonathan de Halleux (dehalleux@pelikhan.com)
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#ifndef BOOST_SPIRIT_ACTOR_HPP
#define BOOST_SPIRIT_ACTOR_HPP

#include <boost/spirit/home/classic/version.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Actors documentation and convention
//
//  Actors
//
//  Actors are predefined semantic action functors. They are used to do an
//  action on the parse result if the parser has had a successful match. An
//  example of actor is the append_actor described in the Spirit
//  documentation.
//
//  The action takes place through a call to the () operator: single argument
//  () operator call for character parsers and two argument (first,last) call
//  for phrase parsers. Actors should implement at least one of the two ()
//  operator.
//
//  Actor instances are not created directly since they usually involve a
//  number of template parameters. Instead generator functions ("helper
//  functions") are provided to generate actors according to their arguments.
//  All helper functions have the "_a" suffix. For example, append_actor is
//  created using the append_a function.
//
//  Policy holder actors and policy actions
//
//  A lot of actors need to store reference to one or more objects. For
//  example, actions on container need to store a reference to the container.
//  Therefore, this kind of actor have been broken down into
//
//      - a action policy that does the action (act method),
//      - a policy holder actor that stores the references and feeds the act
//        method.
//
//  Policy holder actors
//
//      Policy holder have the following naming convention:
//          <member>_ >> *<member> >> !value >> actor
//      where member are the policy stored member, they can be of type:
//
//       - ref, a reference,
//       - const_ref, a const reference,
//       - value, by value,
//       - empty, no stored members
//       - !value states if the policy uses the parse result or not.
//
//  The available policy holder are enumerated below:
//
//      - empty_actor, nothing stored, feeds parse result
//      - value_actor, 1 object stored by value, feeds value
//      - ref_actor, 1 reference stored, feeds ref
//      - ref_value_actor, 1 reference stored, feeds ref and parse result
//
//  Doc. convention
//
//      - ref is a reference to an object stored in a policy holder actor,
//      - value_ref,value1_ref, value2_ref are a const reference stored in a
//          policy holder actor,
//      - value is the parse result in the single argument () operator,
//      - first,last are the parse result in the two argument () operator
//
//  Actors (generator functions) and quick description
//
//      - assign_a(ref)                 assign parse result to ref
//      - assign_a(ref, value_ref)      assign value_ref to ref
//      - increment_a(ref)              increment ref
//      - decrement_a(ref)              decrement ref
//      - push_back_a(ref)              push back the parse result in ref
//      - push_back_a(ref, value_ref)   push back value_ref in ref
//      - push_front_a(ref)             push front the parse result in ref
//      - push_front_a(ref, value_ref)  push front value_ref in ref
//      - insert_key_a(ref,value_ref)   insert value_ref in ref using the
//                                      parse result as key
//      - insert_at_a(ref, key_ref)     insert the parse result in ref at key_ref
//      - insert_at_a(ref, key_ref      insert value_ref in ref at key_ref
//          , value_ref)                
//      - assign_key_a(ref, value_ref)  assign value_ref in ref using the
//                                      parse result as key
//      - erase_a(ref, key)             erase data at key from ref
//      - clear_a(ref)                  clears ref
//      - swap_a(aref, bref)            swaps aref and bref
//
///////////////////////////////////////////////////////////////////////////////

#include <boost/spirit/home/classic/actor/ref_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_value_actor.hpp>
#include <boost/spirit/home/classic/actor/ref_const_ref_const_ref_a.hpp>

#include <boost/spirit/home/classic/actor/assign_actor.hpp>
#include <boost/spirit/home/classic/actor/clear_actor.hpp>
#include <boost/spirit/home/classic/actor/increment_actor.hpp>
#include <boost/spirit/home/classic/actor/decrement_actor.hpp>
#include <boost/spirit/home/classic/actor/push_back_actor.hpp>
#include <boost/spirit/home/classic/actor/push_front_actor.hpp>
#include <boost/spirit/home/classic/actor/erase_actor.hpp>
#include <boost/spirit/home/classic/actor/insert_key_actor.hpp>
#include <boost/spirit/home/classic/actor/insert_at_actor.hpp>
#include <boost/spirit/home/classic/actor/assign_key_actor.hpp>
#include <boost/spirit/home/classic/actor/swap_actor.hpp>

#endif
