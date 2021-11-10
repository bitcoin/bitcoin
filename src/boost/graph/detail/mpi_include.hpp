#ifndef BOOST_GRAPH_DETAIL_MPI_INCLUDE_HPP_INCLUDED
#define BOOST_GRAPH_DETAIL_MPI_INCLUDE_HPP_INCLUDED

// Copyright 2018 Peter Dimov
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#if defined BOOST_GRAPH_USE_MPI
#define BOOST_GRAPH_MPI_INCLUDE(x) x
#else
#define BOOST_GRAPH_MPI_INCLUDE(x) <boost/graph/detail/empty_header.hpp>
#endif

#endif // #ifndef BOOST_GRAPH_DETAIL_MPI_INCLUDE_HPP_INCLUDED
