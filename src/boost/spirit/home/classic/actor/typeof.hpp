/*=============================================================================
    Copyright (c) 2006 Tobias Schwinger
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ACTOR_TYPEOF_HPP)
#define BOOST_SPIRIT_ACTOR_TYPEOF_HPP

#include <boost/spirit/home/classic/namespace.hpp>
#include <boost/typeof/typeof.hpp>

namespace boost { namespace spirit {

BOOST_SPIRIT_CLASSIC_NAMESPACE_BEGIN

    template<typename T, typename ActionT> class ref_actor;

    template<typename T, typename ActionT> class ref_value_actor;

    template<typename T, typename ValueT, typename ActionT> 

    class ref_const_ref_actor;
    template<typename T, typename ValueT, typename ActionT> 

    class ref_const_ref_value_actor;
    template<typename T, typename Value1T, typename Value2T, typename ActionT> 

    class ref_const_ref_const_ref_actor;

    struct assign_action; 
    struct clear_action;
    struct increment_action;
    struct decrement_action;
    struct push_back_action;
    struct push_front_action;
    struct insert_key_action;
    struct insert_at_action;
    struct assign_key_action;
    
    template<typename T> class swap_actor;

BOOST_SPIRIT_CLASSIC_NAMESPACE_END

}} // namespace BOOST_SPIRIT_CLASSIC_NS


#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()


BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::ref_actor,2)
#if !defined(BOOST_SPIRIT_CORE_TYPEOF_HPP)
// this part also lives in the core master header and is deprecated there...
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::ref_value_actor,2)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::ref_const_ref_actor,3)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::assign_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::push_back_action)
#endif
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::ref_const_ref_value_actor,3)
BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::ref_const_ref_const_ref_actor,4)

BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::clear_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::increment_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::decrement_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::push_front_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::insert_key_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::insert_at_action)
BOOST_TYPEOF_REGISTER_TYPE(BOOST_SPIRIT_CLASSIC_NS::assign_key_action)

BOOST_TYPEOF_REGISTER_TEMPLATE(BOOST_SPIRIT_CLASSIC_NS::swap_actor,1)

#endif

