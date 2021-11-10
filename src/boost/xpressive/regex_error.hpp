///////////////////////////////////////////////////////////////////////////////
/// \file regex_error.hpp
/// Contains the definition of the regex_error exception class.
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_REGEX_ERROR_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_REGEX_ERROR_HPP_EAN_10_04_2005

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#include <string>
#include <stdexcept>
#include <boost/throw_exception.hpp>
#include <boost/current_function.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/xpressive/regex_constants.hpp>

//{{AFX_DOC_COMMENT
///////////////////////////////////////////////////////////////////////////////
// This is a hack to get Doxygen to show the inheritance relation between
// regex_error and std::runtime_error.
#ifdef BOOST_XPRESSIVE_DOXYGEN_INVOKED
/// INTERNAL ONLY
namespace std
{
    /// INTERNAL ONLY
    struct runtime_error {};
}
#endif
//}}AFX_DOC_COMMENT

namespace boost { namespace xpressive
{

////////////////////////////////////////////////////////////////////////////////
//  regex_error
//
/// \brief The class regex_error defines the type of objects thrown as
/// exceptions to report errors during the conversion from a string representing
/// a regular expression to a finite state machine.
struct regex_error
  : std::runtime_error
  , boost::exception
{
    /// Constructs an object of class regex_error.
    /// \param code The error_type this regex_error represents.
    /// \param str The message string of this regex_error.
    /// \post code() == code
    explicit regex_error(regex_constants::error_type code, char const *str = "")
      : std::runtime_error(str)
      , boost::exception()
      , code_(code)
    {
    }

    /// Accessor for the error_type value
    /// \return the error_type code passed to the constructor
    /// \throw nothrow
    regex_constants::error_type code() const
    {
        return this->code_;
    }

    /// Destructor for class regex_error
    /// \throw nothrow
    virtual ~regex_error() throw()
    {}

private:

    regex_constants::error_type code_;
};

namespace detail
{
    inline bool ensure_(
        bool cond
      , regex_constants::error_type code
      , char const *msg
      , char const *fun
      , char const *file
      , unsigned long line
    )
    {
        if(!cond)
        {
            #ifndef BOOST_EXCEPTION_DISABLE
            boost::throw_exception(
                boost::xpressive::regex_error(code, msg)
                    << boost::throw_function(fun)
                    << boost::throw_file(file)
                    << boost::throw_line((int)line)
            );
            #else
            boost::throw_exception(boost::xpressive::regex_error(code, msg));
            #endif
        }
        return true;
    }
}

#define BOOST_XPR_ENSURE_(pred, code, msg)                                                          \
    boost::xpressive::detail::ensure_(!!(pred), code, msg, BOOST_CURRENT_FUNCTION, __FILE__, __LINE__)  \
    /**/

}} // namespace boost::xpressive

#endif
