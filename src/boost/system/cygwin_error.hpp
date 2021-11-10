//  boost/system/cygwin_error.hpp  -------------------------------------------//

//  Copyright Beman Dawes 2007

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See library home page at http://www.boost.org/libs/system

#ifndef BOOST_SYSTEM_CYGWIN_ERROR_HPP
#define BOOST_SYSTEM_CYGWIN_ERROR_HPP

#include <boost/config/pragma_message.hpp>

#if !defined(BOOST_ALLOW_DEPRECATED_HEADERS)
  BOOST_PRAGMA_MESSAGE("This header is deprecated and is slated for removal."
  " If you want it retained, please open an issue in github.com/boostorg/system.")
#endif

//  This header is effectively empty for compiles on operating systems where
//  it is not applicable.

# ifdef __CYGWIN__

#include <boost/system/error_code.hpp>

namespace boost
{
  namespace system
  {
    //  To construct an error_code after a API error:
    //
    //      error_code( errno, system_category() )

    //  User code should use the portable "posix" enums for POSIX errors; this
    //  allows such code to be portable to non-POSIX systems. For the non-POSIX
    //  errno values that POSIX-based systems typically provide in addition to 
    //  POSIX values, use the system specific enums below.

   namespace cygwin_error
    {
      enum cygwin_errno
      {
        no_net = ENONET,
        no_package = ENOPKG,
        no_share = ENOSHARE
      };
    }  // namespace cygwin_error

    template<> struct is_error_code_enum<cygwin_error::cygwin_errno>
      { static const bool value = true; };

    namespace cygwin_error
    {
      inline error_code make_error_code( cygwin_errno e )
        { return error_code( e, system_category() ); }
    }
  }
}

#endif  // __CYGWIN__

#endif  // BOOST_SYSTEM_CYGWIN_ERROR_HPP
