// Boost Lambda Library  - operator_lambda_func_base.hpp -----------------
//
// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org

// ------------------------------------------------------------

#ifndef BOOST_LAMBDA_OPERATOR_LAMBDA_FUNC_BASE_HPP
#define BOOST_LAMBDA_OPERATOR_LAMBDA_FUNC_BASE_HPP

namespace boost { 
namespace lambda {


// These operators cannot be implemented as apply functions of action 
// templates


// Specialization for comma.
template<class Args>
class lambda_functor_base<other_action<comma_action>, Args> {
public:
  Args args;
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    return detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS), 
           detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); 
  }


  template<class SigArgs> struct sig { 
  private:
    typedef typename
      detail::deduce_argument_types<Args, SigArgs>::type rets_t;      
  public:
    typedef typename return_type_2_comma< // comma needs special handling
      typename detail::element_or_null<0, rets_t>::type,
      typename detail::element_or_null<1, rets_t>::type
    >::type type;
  };

};  

namespace detail {

// helper traits to make the expression shorter, takes binary action
// bound argument tuple, open argument tuple and gives the return type

template<class Action, class Bound, class Open> class binary_rt {
  private:
    typedef typename
      detail::deduce_argument_types<Bound, Open>::type rets_t;      
  public:
    typedef typename return_type_2_prot<
      Action,  
      typename detail::element_or_null<0, rets_t>::type,
      typename detail::element_or_null<1, rets_t>::type
    >::type type;
};


  // same for unary actions
template<class Action, class Bound, class Open> class unary_rt {
  private:
    typedef typename
      detail::deduce_argument_types<Bound, Open>::type rets_t;      
  public:
    typedef typename return_type_1_prot<
      Action,  
      typename detail::element_or_null<0, rets_t>::type
    >::type type;
};


} // end detail

// Specialization for logical and (to preserve shortcircuiting)
// this could be done with a macro as the others, code used to be different
template<class Args>
class lambda_functor_base<logical_action<and_action>, Args> {
public:
  Args args;
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    return detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) && 
           detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); 
  }
  template<class SigArgs> struct sig { 
    typedef typename
      detail::binary_rt<logical_action<and_action>, Args, SigArgs>::type type;
  };      
};  

// Specialization for logical or (to preserve shortcircuiting)
// this could be done with a macro as the others, code used to be different
template<class Args>
class lambda_functor_base<logical_action< or_action>, Args> {
public:
  Args args;
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    return detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) || 
           detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); 
  }

  template<class SigArgs> struct sig { 
    typedef typename
      detail::binary_rt<logical_action<or_action>, Args, SigArgs>::type type;
  };      
};  

// Specialization for subscript
template<class Args>
class lambda_functor_base<other_action<subscript_action>, Args> {
public:
  Args args;
public:
  explicit lambda_functor_base(const Args& a) : args(a) {}

  template<class RET, CALL_TEMPLATE_ARGS>
  RET call(CALL_FORMAL_ARGS) const {
    return detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) 
           [detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS)]; 
  }

  template<class SigArgs> struct sig { 
    typedef typename
      detail::binary_rt<other_action<subscript_action>, Args, SigArgs>::type 
        type;
  };      
};  


#define BOOST_LAMBDA_BINARY_ACTION(SYMBOL, ACTION_CLASS)  \
template<class Args>                                                      \
class lambda_functor_base<ACTION_CLASS, Args> {                           \
public:                                                                   \
  Args args;                                                              \
public:                                                                   \
  explicit lambda_functor_base(const Args& a) : args(a) {}                \
                                                                          \
  template<class RET, CALL_TEMPLATE_ARGS>                                 \
  RET call(CALL_FORMAL_ARGS) const {                                      \
    return detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS)  \
           SYMBOL                                                         \
           detail::select(boost::tuples::get<1>(args), CALL_ACTUAL_ARGS); \
  }                                                                       \
  template<class SigArgs> struct sig {                                    \
    typedef typename                                                      \
      detail::binary_rt<ACTION_CLASS, Args, SigArgs>::type type;          \
  };                                                                      \
};  

#define BOOST_LAMBDA_PREFIX_UNARY_ACTION(SYMBOL, ACTION_CLASS)            \
template<class Args>                                                      \
class lambda_functor_base<ACTION_CLASS, Args> {                           \
public:                                                                   \
  Args args;                                                              \
public:                                                                   \
  explicit lambda_functor_base(const Args& a) : args(a) {}                \
                                                                          \
  template<class RET, CALL_TEMPLATE_ARGS>                                 \
  RET call(CALL_FORMAL_ARGS) const {                                      \
    return SYMBOL                                                         \
           detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS); \
  }                                                                       \
  template<class SigArgs> struct sig {                                    \
    typedef typename                                                      \
      detail::unary_rt<ACTION_CLASS, Args, SigArgs>::type type;           \
  };                                                                      \
};  

#define BOOST_LAMBDA_POSTFIX_UNARY_ACTION(SYMBOL, ACTION_CLASS)           \
template<class Args>                                                      \
class lambda_functor_base<ACTION_CLASS, Args> {                           \
public:                                                                   \
  Args args;                                                              \
public:                                                                   \
  explicit lambda_functor_base(const Args& a) : args(a) {}                \
                                                                          \
  template<class RET, CALL_TEMPLATE_ARGS>                                 \
  RET call(CALL_FORMAL_ARGS) const {                                      \
    return                                                                \
    detail::select(boost::tuples::get<0>(args), CALL_ACTUAL_ARGS) SYMBOL; \
  }                                                                       \
  template<class SigArgs> struct sig {                                    \
    typedef typename                                                      \
      detail::unary_rt<ACTION_CLASS, Args, SigArgs>::type type;           \
  };                                                                      \
};  

BOOST_LAMBDA_BINARY_ACTION(+,arithmetic_action<plus_action>)
BOOST_LAMBDA_BINARY_ACTION(-,arithmetic_action<minus_action>)
BOOST_LAMBDA_BINARY_ACTION(*,arithmetic_action<multiply_action>)
BOOST_LAMBDA_BINARY_ACTION(/,arithmetic_action<divide_action>)
BOOST_LAMBDA_BINARY_ACTION(%,arithmetic_action<remainder_action>)

BOOST_LAMBDA_BINARY_ACTION(<<,bitwise_action<leftshift_action>)
BOOST_LAMBDA_BINARY_ACTION(>>,bitwise_action<rightshift_action>)
BOOST_LAMBDA_BINARY_ACTION(&,bitwise_action<and_action>)
BOOST_LAMBDA_BINARY_ACTION(|,bitwise_action<or_action>)
BOOST_LAMBDA_BINARY_ACTION(^,bitwise_action<xor_action>)

BOOST_LAMBDA_BINARY_ACTION(<,relational_action<less_action>)
BOOST_LAMBDA_BINARY_ACTION(>,relational_action<greater_action>)
BOOST_LAMBDA_BINARY_ACTION(<=,relational_action<lessorequal_action>)
BOOST_LAMBDA_BINARY_ACTION(>=,relational_action<greaterorequal_action>)
BOOST_LAMBDA_BINARY_ACTION(==,relational_action<equal_action>)
BOOST_LAMBDA_BINARY_ACTION(!=,relational_action<notequal_action>)

BOOST_LAMBDA_BINARY_ACTION(+=,arithmetic_assignment_action<plus_action>)
BOOST_LAMBDA_BINARY_ACTION(-=,arithmetic_assignment_action<minus_action>)
BOOST_LAMBDA_BINARY_ACTION(*=,arithmetic_assignment_action<multiply_action>)
BOOST_LAMBDA_BINARY_ACTION(/=,arithmetic_assignment_action<divide_action>)
BOOST_LAMBDA_BINARY_ACTION(%=,arithmetic_assignment_action<remainder_action>)

BOOST_LAMBDA_BINARY_ACTION(<<=,bitwise_assignment_action<leftshift_action>)
BOOST_LAMBDA_BINARY_ACTION(>>=,bitwise_assignment_action<rightshift_action>)
BOOST_LAMBDA_BINARY_ACTION(&=,bitwise_assignment_action<and_action>)
BOOST_LAMBDA_BINARY_ACTION(|=,bitwise_assignment_action<or_action>)
BOOST_LAMBDA_BINARY_ACTION(^=,bitwise_assignment_action<xor_action>)

BOOST_LAMBDA_BINARY_ACTION(=,other_action< assignment_action>)


BOOST_LAMBDA_PREFIX_UNARY_ACTION(+, unary_arithmetic_action<plus_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(-, unary_arithmetic_action<minus_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(~, bitwise_action<not_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(!, logical_action<not_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(++, pre_increment_decrement_action<increment_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(--, pre_increment_decrement_action<decrement_action>)

BOOST_LAMBDA_PREFIX_UNARY_ACTION(&,other_action<addressof_action>)
BOOST_LAMBDA_PREFIX_UNARY_ACTION(*,other_action<contentsof_action>)

BOOST_LAMBDA_POSTFIX_UNARY_ACTION(++, post_increment_decrement_action<increment_action>)
BOOST_LAMBDA_POSTFIX_UNARY_ACTION(--, post_increment_decrement_action<decrement_action>)


#undef BOOST_LAMBDA_POSTFIX_UNARY_ACTION
#undef BOOST_LAMBDA_PREFIX_UNARY_ACTION
#undef BOOST_LAMBDA_BINARY_ACTION

} // namespace lambda
} // namespace boost

#endif










