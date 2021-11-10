//=======================================================================
// Copyright 2001 University of Notre Dame.
// Copyright 2003 Jeremy Siek
// Authors: Lie-Quan Lee, Jeremy Siek, and Douglas Gregor
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================

#ifndef BOOST_GRAPH_DLL_IMPORT_EXPORT_HPP
#define BOOST_GRAPH_DLL_IMPORT_EXPORT_HPP

#include <boost/config.hpp>

#if defined(BOOST_ALL_DYN_LINK) || defined(BOOST_GRAPH_DYN_LINK)
#ifdef BOOST_GRAPH_SOURCE
#define BOOST_GRAPH_DECL BOOST_SYMBOL_EXPORT
#else
#define BOOST_GRAPH_DECL BOOST_SYMBOL_IMPORT
#endif // BOOST_GRAPH_SOURCE
#endif // DYN_LINK

#ifndef BOOST_GRAPH_DECL
#define BOOST_GRAPH_DECL
#endif

#endif // BOOST_GRAPH_DLL_IMPORT_EXPORT_HPP
