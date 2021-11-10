///////////////////////////////////////////////////////////////////////////////
/// \file xpressive_typeof.hpp
/// Type registrations so that xpressive can be used with the Boost.Typeof library.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_XPRESSIVE_TYPEOF_H
#define BOOST_XPRESSIVE_XPRESSIVE_TYPEOF_H

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/config.hpp>
#include <boost/typeof/typeof.hpp>
#ifndef BOOST_NO_STD_LOCALE
# include <boost/typeof/std/locale.hpp>
#endif
#include <boost/proto/proto_typeof.hpp>
#include <boost/xpressive/detail/detail_fwd.hpp>

#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

BOOST_TYPEOF_REGISTER_TEMPLATE(boost::mpl::bool_, (bool))

///////////////////////////////////////////////////////////////////////////////
// Misc.
//
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::set_initializer)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::keeper_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::modifier_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::lookahead_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::lookbehind_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::check_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::mark_tag)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::word_begin)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::word_end)
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::generic_quant_tag, (unsigned int)(unsigned int))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::basic_regex, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::word_boundary, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::value, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::reference, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::local, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::placeholder, (typename)(int)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::tracking_ptr, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::regex_impl, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::let_, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::action_arg, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::named_mark, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::sub_match, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::nested_results, (typename))

///////////////////////////////////////////////////////////////////////////////
// Placeholders
//
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::mark_placeholder)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::posix_charset_placeholder)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::assert_bol_placeholder)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::assert_eol_placeholder)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::logical_newline_placeholder)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::self_placeholder)
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::assert_word_placeholder, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::range_placeholder, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::attribute_placeholder, (typename))

///////////////////////////////////////////////////////////////////////////////
// Matchers
//
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::epsilon_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::true_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::end_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::independent_end_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::any_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::assert_bos_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::assert_eos_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::mark_begin_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::mark_end_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::repeat_begin_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::alternate_end_matcher)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::attr_end_matcher)
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::assert_bol_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::assert_eol_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::literal_matcher, (typename)(typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::string_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::charset_matcher, (typename)(typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::logical_newline_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::mark_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::repeat_end_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::alternate_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::optional_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::optional_mark_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::simple_repeat_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::regex_byref_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::regex_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::posix_charset_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::assert_word_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::range_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::keeper_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::lookahead_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::lookbehind_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::set_matcher, (typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::predicate_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::action_matcher, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::attr_matcher, (typename)(typename)(typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::attr_begin_matcher, (typename))

///////////////////////////////////////////////////////////////////////////////
// Ops
//
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::push)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::push_back)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::pop)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::push_front)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::pop_back)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::pop_front)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::back)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::front)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::top)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::first)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::second)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::matched)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::length)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::str)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::insert)
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::op::make_pair)
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::as, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::static_cast_, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::dynamic_cast_, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::const_cast_, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::construct, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::op::throw_, (typename))

///////////////////////////////////////////////////////////////////////////////
// Modifiers
//
BOOST_TYPEOF_REGISTER_TYPE(boost::xpressive::detail::icase_modifier)
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::detail::locale_modifier, (typename))

///////////////////////////////////////////////////////////////////////////////
// Traits
//
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::null_regex_traits, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::cpp_regex_traits, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::c_regex_traits, (typename))
BOOST_TYPEOF_REGISTER_TEMPLATE(boost::xpressive::regex_traits, (typename)(typename))

#endif
