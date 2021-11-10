//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief common dataset macros
// ***************************************************************************

#ifndef BOOST_TEST_DATA_CONFIG_HPP_112611GER
#define BOOST_TEST_DATA_CONFIG_HPP_112611GER

// Boost.Test
#include <boost/test/detail/config.hpp>
#include <boost/test/detail/throw_exception.hpp>

// STL
#include <stdexcept> // for std::logic_error

// availability on features: preprocessed by doxygen

#if defined(BOOST_NO_CXX11_HDR_RANDOM) || defined(BOOST_TEST_DOXYGEN_DOC__)
//! Defined when the random dataset feature is not available
#define BOOST_TEST_NO_RANDOM_DATASET_AVAILABLE

#endif

#if defined(BOOST_NO_CXX11_HDR_TUPLE) || defined(BOOST_TEST_DOXYGEN_DOC__)

//! Defined when grid composition of datasets is not available
#define BOOST_TEST_NO_GRID_COMPOSITION_AVAILABLE

//! Defined when zip composition of datasets is not available
#define BOOST_TEST_NO_ZIP_COMPOSITION_AVAILABLE

#endif

//! Defined when the initializer_list implementation is buggy, such as for VS2013
#if defined(_MSC_VER) && _MSC_VER < 1900
#  define BOOST_TEST_ERRONEOUS_INIT_LIST
#endif

//____________________________________________________________________________//

#define BOOST_TEST_DS_ERROR( msg )        BOOST_TEST_I_THROW( std::logic_error( msg ) )
#define BOOST_TEST_DS_ASSERT( cond, msg ) BOOST_TEST_I_ASSRT( cond, std::logic_error( msg ) )

#endif // BOOST_TEST_DATA_CONFIG_HPP_112611GER
