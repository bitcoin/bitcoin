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
#ifndef BOOST_PROPERTY_TREE_DETAIL_EXCEPTIONS_IMPLEMENTATION_HPP_INCLUDED
#define BOOST_PROPERTY_TREE_DETAIL_EXCEPTIONS_IMPLEMENTATION_HPP_INCLUDED

namespace boost { namespace property_tree
{

    namespace detail
    {

        // Helper for preparing what string in ptree_bad_path exception
        template<class P> inline
        std::string prepare_bad_path_what(const std::string &what,
                                          const P &path)
        {
            return what + " (" + path.dump() + ")";
        }

    }

    ///////////////////////////////////////////////////////////////////////////
    // ptree_error

    inline ptree_error::ptree_error(const std::string &w): 
        std::runtime_error(w) 
    {
    }

    inline ptree_error::~ptree_error() throw()
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // ptree_bad_data

    template<class D> inline
    ptree_bad_data::ptree_bad_data(const std::string &w, const D &d):
        ptree_error(w), m_data(d)
    {
    }

    inline ptree_bad_data::~ptree_bad_data() throw()
    {
    }

    template<class D> inline
    D ptree_bad_data::data() const
    {
        return boost::any_cast<D>(m_data);
    }

    ///////////////////////////////////////////////////////////////////////////
    // ptree_bad_path

    template<class P> inline
    ptree_bad_path::ptree_bad_path(const std::string &w, const P &p):
        ptree_error(detail::prepare_bad_path_what(w, p)), m_path(p)
    {

    }

    inline ptree_bad_path::~ptree_bad_path() throw()
    {
    }

    template<class P> inline
    P ptree_bad_path::path() const
    {
        return boost::any_cast<P>(m_path);
    }

}}

#endif
