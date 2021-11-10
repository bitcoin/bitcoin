// ----------------------------------------------------------------------------
// Copyright (C) 2002-2006 Marcin Kalicinski
// Copyright (C) 2009 Sebastian Redl
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see www.boost.org
// ----------------------------------------------------------------------------

#ifndef BOOST_PROPERTY_TREE_EXCEPTIONS_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_EXCEPTIONS_HPP_INCLUDED

#include <boost/property_tree/ptree_fwd.hpp>

#include <boost/any.hpp>
#include <stdexcept>
#include <string>

namespace boost { namespace property_tree
{

    /// Base class for all property tree errors. Derives from
    /// @c std::runtime_error. Call member function @c what to get human
    /// readable message associated with the error.
    class ptree_error : public std::runtime_error
    {
    public:
        /// Instantiate a ptree_error instance with the given message.
        /// @param what The message to associate with this error.
        ptree_error(const std::string &what);

        ~ptree_error() throw() BOOST_OVERRIDE;
    };


    /// Error indicating that translation from given value to the property tree
    /// data_type (or vice versa) failed. Derives from ptree_error.
    class ptree_bad_data : public ptree_error
    {
    public:
        /// Instantiate a ptree_bad_data instance with the given message and
        /// data.
        /// @param what The message to associate with this error.
        /// @param data The value associated with this error that was the source
        ///             of the translation failure.
        template<class T> ptree_bad_data(const std::string &what,
                                         const T &data);

        ~ptree_bad_data() throw() BOOST_OVERRIDE;

        /// Retrieve the data associated with this error. This is the source
        /// value that failed to be translated. You need to explicitly
        /// specify its type.
        template<class T> T data() const;
    private:
        boost::any m_data;
    };


    /// Error indicating that specified path does not exist. Derives from
    /// ptree_error.
    class ptree_bad_path : public ptree_error
    {
    public:
        /// Instantiate a ptree_bad_path with the given message and path data.
        /// @param what The message to associate with this error.
        /// @param path The path that could not be found in the property_tree.
        template<class T> ptree_bad_path(const std::string &what,
                                         const T &path);

        ~ptree_bad_path() throw() BOOST_OVERRIDE;

        /// Retrieve the invalid path. You need to explicitly specify the
        /// type of path.
        template<class T> T path() const;
    private:
        boost::any m_path;
    };

}}

#include <boost/property_tree/detail/exception_implementation.hpp>

#endif
