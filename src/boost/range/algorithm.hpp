///////////////////////////////////////////////////////////////////////////////
/// \file algorithm.hpp
///   Includes the range-based versions of the algorithms in the
///   C++ standard header file <algorithm>
//
/////////////////////////////////////////////////////////////////////////////

// Copyright 2009 Neil Groves.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Acknowledgements:
// This code uses combinations of ideas, techniques and code snippets
// from: Thorsten Ottosen, Eric Niebler, Jeremy Siek,
// and Vladimir Prus'
//
// The original mutating algorithms that served as the first version
// were originally written by Vladimir Prus'
// <ghost@cs.msu.su> code from Boost Wiki

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef BOOST_RANGE_ALGORITHM_HPP_INCLUDED_01012009
#define BOOST_RANGE_ALGORITHM_HPP_INCLUDED_01012009

#include <boost/range/concepts.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/difference_type.hpp>
#include <boost/range/detail/range_return.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/next_prior.hpp>
#include <algorithm>

// Non-mutating algorithms
#include <boost/range/algorithm/adjacent_find.hpp>
#include <boost/range/algorithm/count.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_end.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/lexicographical_compare.hpp>
#include <boost/range/algorithm/mismatch.hpp>
#include <boost/range/algorithm/search.hpp>
#include <boost/range/algorithm/search_n.hpp>

// Mutating algorithms
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/copy_backward.hpp>
#include <boost/range/algorithm/fill.hpp>
#include <boost/range/algorithm/fill_n.hpp>
#include <boost/range/algorithm/generate.hpp>
#include <boost/range/algorithm/inplace_merge.hpp>
#include <boost/range/algorithm/merge.hpp>
#include <boost/range/algorithm/nth_element.hpp>
#include <boost/range/algorithm/partial_sort.hpp>
#include <boost/range/algorithm/partial_sort_copy.hpp>
#include <boost/range/algorithm/partition.hpp>
#include <boost/range/algorithm/random_shuffle.hpp>
#include <boost/range/algorithm/remove.hpp>
#include <boost/range/algorithm/remove_copy.hpp>
#include <boost/range/algorithm/remove_copy_if.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <boost/range/algorithm/replace.hpp>
#include <boost/range/algorithm/replace_copy.hpp>
#include <boost/range/algorithm/replace_copy_if.hpp>
#include <boost/range/algorithm/replace_if.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/reverse_copy.hpp>
#include <boost/range/algorithm/rotate.hpp>
#include <boost/range/algorithm/rotate_copy.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/stable_partition.hpp>
#include <boost/range/algorithm/stable_sort.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/unique_copy.hpp>

// Binary search
#include <boost/range/algorithm/binary_search.hpp>
#include <boost/range/algorithm/equal_range.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/algorithm/upper_bound.hpp>

// Set operations of sorted ranges
#include <boost/range/algorithm/set_algorithm.hpp>

// Heap operations
#include <boost/range/algorithm/heap_algorithm.hpp>

// Minimum and Maximum
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/min_element.hpp>

// Permutations
#include <boost/range/algorithm/permutation.hpp>

#endif // include guard

