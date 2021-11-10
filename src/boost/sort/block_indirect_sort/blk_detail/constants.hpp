//----------------------------------------------------------------------------
/// @file constants.hpp
/// @brief This file contains the constants values used in the algorithms
///
/// @author Copyright (c) 2016 Francisco José Tapia (fjtapia@gmail.com )\n
///         Distributed under the Boost Software License, Version 1.0.\n
///         ( See accompanying file LICENSE_1_0.txt or copy at
///           http://www.boost.org/LICENSE_1_0.txt  )
/// @version 0.1
///
/// @remarks
//-----------------------------------------------------------------------------
#ifndef __BOOST_SORT_PARALLEL_DETAIL_CONSTANTS_HPP
#define __BOOST_SORT_PARALLEL_DETAIL_CONSTANTS_HPP

// This value is the block size in the block_indirect_sort algorithm
#define BOOST_BLOCK_SIZE 1024

// This value represent the group size in the block_indirect_sort algorithm
#define BOOST_GROUP_SIZE 64

// This value is the minimal number of threads for to use the
// block_indirect_sort algorithm
#define BOOST_NTHREAD_BORDER 6

#endif
