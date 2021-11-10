/*
  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_SIGNALS2_SIGNALS_COMMON_MACROS_HPP
#define BOOST_SIGNALS2_SIGNALS_COMMON_MACROS_HPP

#include <boost/config.hpp>

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES

#ifndef BOOST_SIGNALS2_MAX_ARGS
#define BOOST_SIGNALS2_MAX_ARGS 9
#endif

// signaln
#define BOOST_SIGNALS2_SIGNAL_CLASS_NAME(arity) BOOST_PP_CAT(signal, arity)
// weak_signaln
#define BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(arity) BOOST_PP_CAT(weak_, BOOST_SIGNALS2_SIGNAL_CLASS_NAME(arity))
// signaln_impl
#define BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(arity) BOOST_PP_CAT(BOOST_SIGNALS2_SIGNAL_CLASS_NAME(arity), _impl)
// argn
#define BOOST_SIGNALS2_SIGNATURE_ARG_NAME(z, n, data) BOOST_PP_CAT(arg, BOOST_PP_INC(n))
// Tn argn
#define BOOST_SIGNALS2_SIGNATURE_FULL_ARG(z, n, data) \
  BOOST_PP_CAT(T, BOOST_PP_INC(n)) BOOST_SIGNALS2_SIGNATURE_ARG_NAME(~, n, ~)
// T1 arg1, T2 arg2, ..., Tn argn
#define BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(arity) \
  BOOST_PP_ENUM(arity, BOOST_SIGNALS2_SIGNATURE_FULL_ARG, ~)
// arg1, arg2, ..., argn
#define BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(arity) BOOST_PP_ENUM(arity, BOOST_SIGNALS2_SIGNATURE_ARG_NAME, ~)
// T1, T2, ..., TN
#define BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(arity) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), T)
// R (T1, T2, ..., TN)
#define BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(arity) \
  R ( BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(arity) )
// typename prefixR, typename prefixT1, typename prefixT2, ..., typename prefixTN
#define BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_DECL(arity, prefix) \
  typename BOOST_PP_CAT(prefix, R) BOOST_PP_COMMA_IF(arity) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), typename BOOST_PP_CAT(prefix, T))
// typename R, typename T1, typename T2, ..., typename TN
#define BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(arity) \
  typename R BOOST_PP_COMMA_IF(arity) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), typename T)
// typename prefixT1, typename prefixT2, ..., typename prefixTN
#define BOOST_SIGNALS2_PREFIXED_ARGS_TEMPLATE_DECL(arity, prefix) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), typename BOOST_PP_CAT(prefix, T))
// typename T1, typename T2, ..., typename TN
#define BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(arity) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), typename T)
// prefixR, prefixT1, prefixT2, ..., prefixTN
#define BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_INSTANTIATION(arity, prefix) \
  BOOST_PP_CAT(prefix, R) BOOST_PP_COMMA_IF(arity) BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), BOOST_PP_CAT(prefix, T))
// R, T1, T2, ..., TN
#define BOOST_SIGNALS2_SIGNATURE_TEMPLATE_INSTANTIATION(arity) \
  R BOOST_PP_COMMA_IF(arity) BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), T)
// boost::functionN<R, T1, T2, ..., TN>
#define BOOST_SIGNALS2_FUNCTION_N_DECL(arity) BOOST_PP_CAT(boost::function, arity)<\
  BOOST_SIGNALS2_SIGNATURE_TEMPLATE_INSTANTIATION(arity) >
// R, const boost::signals2::connection&, T1, T2, ..., TN
#define BOOST_SIGNALS2_EXT_SLOT_TEMPLATE_INSTANTIATION(arity) \
  R, const boost::signals2::connection&  BOOST_PP_COMMA_IF(arity) \
  BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(arity), T)
// boost::functionN<R, const boost::signals2::connection &, T1, T2, ..., TN>
#define BOOST_SIGNALS2_EXT_FUNCTION_N_DECL(arity) BOOST_PP_CAT(boost::function, BOOST_PP_INC(arity))<\
  BOOST_SIGNALS2_EXT_SLOT_TEMPLATE_INSTANTIATION(arity) >
// slotN
#define BOOST_SIGNALS2_SLOT_CLASS_NAME(arity) BOOST_PP_CAT(slot, arity)
// slotN+1<R, const connection &, T1, T2, ..., TN, extended_slot_function_type>
#define BOOST_SIGNALS2_EXTENDED_SLOT_TYPE(arity) \
  BOOST_SIGNALS2_SLOT_CLASS_NAME(BOOST_PP_INC(arity))< \
  BOOST_SIGNALS2_EXT_SLOT_TEMPLATE_INSTANTIATION(arity), \
  extended_slot_function_type>
// bound_extended_slot_functionN
#define BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(arity) BOOST_PP_CAT(bound_extended_slot_function, arity)
// bound_extended_slot_function_helperN
#define BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(arity) BOOST_PP_CAT(bound_extended_slot_function_invoker, arity)
// typename function_traits<Signature>::argn_type
#define BOOST_SIGNALS2_SIGNATURE_TO_ARGN_TYPE(z, n, Signature) \
  BOOST_PP_CAT(BOOST_PP_CAT(typename function_traits<Signature>::arg, BOOST_PP_INC(n)), _type)
// typename function_traits<Signature>::result_type,
// typename function_traits<Signature>::arg1_type,
// typename function_traits<Signature>::arg2_type,
// ...,
// typename function_traits<Signature>::argn_type
#define BOOST_SIGNALS2_PORTABLE_SIGNATURE(arity, Signature) \
  typename function_traits<Signature>::result_type \
  BOOST_PP_COMMA_IF(arity) BOOST_PP_ENUM(arity, BOOST_SIGNALS2_SIGNATURE_TO_ARGN_TYPE, Signature)
// prefixTn & argn
#define BOOST_SIGNALS2_PREFIXED_FULL_REF_ARG(z, n, prefix) \
  BOOST_PP_CAT(BOOST_PP_CAT(prefix, T), BOOST_PP_INC(n)) & BOOST_SIGNALS2_SIGNATURE_ARG_NAME(~, n, ~)
// prefixT1 & arg1, prefixT2 & arg2, ..., prefixTn & argn
#define BOOST_SIGNALS2_PREFIXED_FULL_REF_ARGS(arity, prefix) \
  BOOST_PP_ENUM(arity, BOOST_SIGNALS2_PREFIXED_FULL_REF_ARG, prefix)
// Tn & argn
#define BOOST_SIGNALS2_FULL_CREF_ARG(z, n, data) \
  const BOOST_PP_CAT(T, BOOST_PP_INC(n)) & BOOST_SIGNALS2_SIGNATURE_ARG_NAME(~, n, ~)
// const T1 & arg1, const T2 & arg2, ..., const Tn & argn
#define BOOST_SIGNALS2_FULL_FORWARD_ARGS(arity) \
  BOOST_PP_ENUM(arity, BOOST_SIGNALS2_FULL_CREF_ARG, ~)
#define BOOST_SIGNALS2_FORWARDED_ARGS(arity) \
  BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(arity)
// preprocessed_arg_typeN
#define BOOST_SIGNALS2_PREPROCESSED_ARG_N_TYPE_CLASS_NAME(arity) BOOST_PP_CAT(preprocessed_arg_type, arity)

// typename R, typename T1, typename T2, ..., typename TN, typename SlotFunction
#define BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION_DECL(arity) \
  BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(arity), \
  typename SlotFunction
#define BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION

// typename R, typename T1, typename T2, ..., typename TN, typename Combiner, ...
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(arity) \
  BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(arity), \
  typename Combiner, \
  typename Group, \
  typename GroupCompare, \
  typename SlotFunction, \
  typename ExtendedSlotFunction, \
  typename Mutex
// typename R, typename T1, typename T2, ..., typename TN, typename Combiner = optional_last_value<R>, ...
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_DEFAULTED_DECL(arity) \
  BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(arity), \
  typename Combiner = optional_last_value<R>, \
  typename Group = int, \
  typename GroupCompare = std::less<Group>, \
  typename SlotFunction = BOOST_SIGNALS2_FUNCTION_N_DECL(arity), \
  typename ExtendedSlotFunction = BOOST_SIGNALS2_EXT_FUNCTION_N_DECL(arity), \
  typename Mutex = signals2::mutex
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(arity) BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(arity)
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION

#define BOOST_SIGNALS2_STD_FUNCTIONAL_BASE std_functional_base

#define BOOST_SIGNALS2_PP_COMMA_IF(arity) BOOST_PP_COMMA_IF(arity)

#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

#define BOOST_SIGNALS2_SIGNAL_CLASS_NAME(arity) signal
#define BOOST_SIGNALS2_WEAK_SIGNAL_CLASS_NAME(arity) weak_signal
#define BOOST_SIGNALS2_SIGNAL_IMPL_CLASS_NAME(arity) signal_impl
#define BOOST_SIGNALS2_SIGNATURE_TEMPLATE_DECL(arity) typename Signature
#define BOOST_SIGNALS2_ARGS_TEMPLATE_INSTANTIATION(arity) Args...
#define BOOST_SIGNALS2_SIGNATURE_TEMPLATE_INSTANTIATION(arity) R (Args...)
#define BOOST_SIGNALS2_SIGNATURE_FUNCTION_TYPE(arity) R (Args...)
#define BOOST_SIGNALS2_ARGS_TEMPLATE_DECL(arity) typename ... Args
#define BOOST_SIGNALS2_FULL_FORWARD_ARGS(arity) Args && ... args
#define BOOST_SIGNALS2_FORWARDED_ARGS(arity) std::forward<Args>(args)...
#define BOOST_SIGNALS2_SLOT_CLASS_NAME(arity) slot
#define BOOST_SIGNALS2_EXTENDED_SLOT_TYPE(arity) slot<R (const connection &, Args...), extended_slot_function_type>
#define BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_N(arity) bound_extended_slot_function
#define BOOST_SIGNALS2_BOUND_EXTENDED_SLOT_FUNCTION_INVOKER_N(arity) bound_extended_slot_function_invoker
#define BOOST_SIGNALS2_FUNCTION_N_DECL(arity) boost::function<Signature>
#define BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_DECL(arity, prefix) typename prefixSignature
#define BOOST_SIGNALS2_PREFIXED_SIGNATURE_TEMPLATE_INSTANTIATION(arity, prefix) prefixSignature
#define BOOST_SIGNALS2_SIGNATURE_FULL_ARGS(arity) Args ... args
#define BOOST_SIGNALS2_SIGNATURE_ARG_NAMES(arity) args...
#define BOOST_SIGNALS2_PORTABLE_SIGNATURE(arity, Signature) Signature

#define BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION_DECL(arity) \
  typename SlotFunction, \
  typename R, \
  typename ... Args
#define BOOST_SIGNALS2_SLOT_TEMPLATE_SPECIALIZATION \
  <R (Args...), SlotFunction>

#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_DECL(arity) \
  typename Signature, \
  typename Combiner, \
  typename Group, \
  typename GroupCompare, \
  typename SlotFunction, \
  typename ExtendedSlotFunction, \
  typename Mutex
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_DEFAULTED_DECL(arity) \
  typename Signature, \
  typename Combiner = optional_last_value<typename boost::function_traits<Signature>::result_type>, \
  typename Group = int, \
  typename GroupCompare = std::less<Group>, \
  typename SlotFunction = boost::function<Signature>, \
  typename ExtendedSlotFunction = typename detail::variadic_extended_signature<Signature>::function_type, \
  typename Mutex = signals2::mutex
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION_DECL(arity) \
  typename Combiner, \
  typename Group, \
  typename GroupCompare, \
  typename SlotFunction, \
  typename ExtendedSlotFunction, \
  typename Mutex, \
  typename R, \
  typename ... Args
#define BOOST_SIGNALS2_SIGNAL_TEMPLATE_SPECIALIZATION <\
  R (Args...), \
  Combiner, \
  Group, \
  GroupCompare, \
  SlotFunction, \
  ExtendedSlotFunction, \
  Mutex>

#define BOOST_SIGNALS2_STD_FUNCTIONAL_BASE \
  std_functional_base<Args...>

#define BOOST_SIGNALS2_PP_COMMA_IF(arity) ,

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES

#endif // BOOST_SIGNALS2_SIGNALS_COMMON_MACROS_HPP
