// Copyright (C) 2018 Alain Miniussi <alain.miniussi -at- oca.eu>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file error_string.hpp
 *
 *  Error code to string convertion.
 */
#ifndef BOOST_MPI_ERROR_STRING_HPP
#define BOOST_MPI_ERROR_STRING_HPP

#include <boost/mpi/config.hpp>
#include <string>

namespace boost { namespace mpi {

/**
 * @brief Convert a MPI error code to an error string.
 */
std::string error_string(int err_code);

} } // end namespace boost::mpi

#endif // BOOST_MPI_ERROR_STRING_HPP
