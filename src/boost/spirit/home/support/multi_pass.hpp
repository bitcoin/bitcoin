//  Copyright (c) 2001-2009, Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_0201AM)
#define BOOST_SPIRIT_ITERATOR_MULTI_PASS_MAR_16_2007_0201AM

#if defined(_MSC_VER)
#pragma once
#endif

//  Include everything needed for the multi_pass
//  Ownership policies
#include <boost/spirit/home/support/iterators/detail/first_owner_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/ref_counted_policy.hpp>

//  Input policies
#include <boost/spirit/home/support/iterators/detail/input_iterator_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/buffering_input_iterator_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/istream_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/lex_input_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/split_functor_input_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/functor_input_policy.hpp>

//  Checking policies
#include <boost/spirit/home/support/iterators/detail/buf_id_check_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/no_check_policy.hpp>

//  Storage policies
#include <boost/spirit/home/support/iterators/detail/fixed_size_queue_policy.hpp>
#include <boost/spirit/home/support/iterators/detail/split_std_deque_policy.hpp>

//  Main multi_pass iterator 
#include <boost/spirit/home/support/iterators/detail/combine_policies.hpp>
#include <boost/spirit/home/support/iterators/detail/multi_pass.hpp>
#include <boost/spirit/home/support/iterators/multi_pass.hpp>

#endif 
