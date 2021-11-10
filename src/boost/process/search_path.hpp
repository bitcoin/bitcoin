// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/search_path.hpp
 *
 * Defines a function to search for an executable in path.
 */

#ifndef BOOST_PROCESS_SEARCH_PATH_HPP
#define BOOST_PROCESS_SEARCH_PATH_HPP

#include <boost/process/detail/config.hpp>
#include <boost/process/environment.hpp>

#if defined(BOOST_POSIX_API)
#include <boost/process/detail/posix/search_path.hpp>
#elif defined(BOOST_WINDOWS_API)
#include <boost/process/detail/windows/search_path.hpp>
#endif


namespace boost { namespace process {

/**
 * Searches for an executable in path.
 *
 * filename must be a basename including the file extension.
 * It must not include any directory separators (like a slash).
 * On Windows the file extension may be omitted. The function
 * will then try the various file extensions for executables on
 * Windows to find filename.
 *
 * \param filename The base of the filename to find
 *
 * \param path the set of paths to search, defaults to the "PATH" environment variable.
 *
 * \returns the absolute path to the executable filename or an
 *          empty string if filename isn't found
 */
inline boost::filesystem::path search_path(const boost::filesystem::path &filename,
                                    const std::vector<boost::filesystem::path> path = ::boost::this_process::path())
{
    return ::boost::process::detail::api::search_path(filename, path);
}
}}

#endif
