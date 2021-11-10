/*
 *
 * Copyright (c) 2003-2005
 * John Maddock
 *
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         error_type.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression error type enumerator.
  */

#ifndef BOOST_REGEX_ERROR_TYPE_HPP
#define BOOST_REGEX_ERROR_TYPE_HPP

#ifdef __cplusplus
namespace boost{
#endif

#ifdef __cplusplus
namespace regex_constants{

enum error_type{

   error_ok = 0,         /* not used */
   error_no_match = 1,   /* not used */
   error_bad_pattern = 2,
   error_collate = 3,
   error_ctype = 4,
   error_escape = 5,
   error_backref = 6,
   error_brack = 7,
   error_paren = 8,
   error_brace = 9,
   error_badbrace = 10,
   error_range = 11,
   error_space = 12,
   error_badrepeat = 13,
   error_end = 14,    /* not used */
   error_size = 15,
   error_right_paren = 16,  /* not used */
   error_empty = 17,
   error_complexity = 18,
   error_stack = 19,
   error_perl_extension = 20,
   error_unknown = 21
};

}
}
#endif /* __cplusplus */

#endif
