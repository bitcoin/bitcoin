// -- operator_actions.hpp - Boost Lambda Library ----------------------

// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://lambda.cs.utu.fi 

#ifndef BOOST_LAMBDA_OPERATOR_ACTIONS_HPP
#define BOOST_LAMBDA_OPERATOR_ACTIONS_HPP

namespace boost { 
namespace lambda {


// -- artihmetic ----------------------

class plus_action {};
class minus_action {};
class multiply_action {};
class divide_action {};
class remainder_action {};

// -- bitwise  -------------------

class leftshift_action {};
class rightshift_action {};
class xor_action {};


// -- bitwise/logical -------------------

class and_action {};
class or_action {};
class not_action {};

// -- relational -------------------------

class less_action {};
class greater_action {};
class lessorequal_action {};
class greaterorequal_action {};
class equal_action {};
class notequal_action {};

// -- increment/decrement ------------------------------

class increment_action {};
class decrement_action {};

// -- void return ------------------------------

// -- other  ------------------------------

class addressof_action {};
  // class comma_action {}; // defined in actions.hpp
class contentsof_action {};
// class member_pointer_action {}; (defined in member_ptr.hpp)


// -- actioun group templates --------------------

template <class Action> class arithmetic_action;
template <class Action> class bitwise_action;
template <class Action> class logical_action;
template <class Action> class relational_action;
template <class Action> class arithmetic_assignment_action;
template <class Action> class bitwise_assignment_action;
template <class Action> class unary_arithmetic_action;
template <class Action> class pre_increment_decrement_action;
template <class Action> class post_increment_decrement_action;

// ---------------------------------------------------------

  // actions, for which the existence of protect is checked in return type 
  // deduction.

template <class Act> struct is_protectable<arithmetic_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct is_protectable<bitwise_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct is_protectable<logical_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct is_protectable<relational_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> 
struct is_protectable<arithmetic_assignment_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct is_protectable<bitwise_assignment_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct is_protectable<unary_arithmetic_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> 
struct is_protectable<pre_increment_decrement_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <class Act> struct 
is_protectable<post_increment_decrement_action<Act> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template <> struct is_protectable<other_action<addressof_action> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template <> struct is_protectable<other_action<contentsof_action> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};

template<> struct is_protectable<other_action<subscript_action> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};
template<> struct is_protectable<other_action<assignment_action> > {
  BOOST_STATIC_CONSTANT(bool, value = true);
};

// NOTE: comma action is also protectable, but the specialization is
  // in actions.hpp


} // namespace lambda 
} // namespace boost

#endif







