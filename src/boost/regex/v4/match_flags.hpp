/*
 *
 * Copyright (c) 1998-2002
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         match_flags.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares match_flags type.
  */

#ifndef BOOST_REGEX_V4_MATCH_FLAGS
#define BOOST_REGEX_V4_MATCH_FLAGS

#ifdef __cplusplus
#  include <boost/cstdint.hpp>
#endif

#ifdef __cplusplus
namespace boost{
   namespace regex_constants{
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#if BOOST_MSVC >= 1800
#pragma warning(disable : 26812)
#endif
#endif

typedef enum _match_flags
{
   match_default = 0,
   match_not_bol = 1,                                /* first is not start of line */
   match_not_eol = match_not_bol << 1,               /* last is not end of line */
   match_not_bob = match_not_eol << 1,               /* first is not start of buffer */
   match_not_eob = match_not_bob << 1,               /* last is not end of buffer */
   match_not_bow = match_not_eob << 1,               /* first is not start of word */
   match_not_eow = match_not_bow << 1,               /* last is not end of word */
   match_not_dot_newline = match_not_eow << 1,       /* \n is not matched by '.' */
   match_not_dot_null = match_not_dot_newline << 1,  /* '\0' is not matched by '.' */
   match_prev_avail = match_not_dot_null << 1,       /* *--first is a valid expression */
   match_init = match_prev_avail << 1,               /* internal use */
   match_any = match_init << 1,                      /* don't care what we match */
   match_not_null = match_any << 1,                  /* string can't be null */
   match_continuous = match_not_null << 1,           /* each grep match must continue from */
                                                     /* uninterrupted from the previous one */
   match_partial = match_continuous << 1,            /* find partial matches */
   
   match_stop = match_partial << 1,                  /* stop after first match (grep) V3 only */
   match_not_initial_null = match_stop,              /* don't match initial null, V4 only */
   match_all = match_stop << 1,                      /* must find the whole of input even if match_any is set */
   match_perl = match_all << 1,                      /* Use perl matching rules */
   match_posix = match_perl << 1,                    /* Use POSIX matching rules */
   match_nosubs = match_posix << 1,                  /* don't trap marked subs */
   match_extra = match_nosubs << 1,                  /* include full capture information for repeated captures */
   match_single_line = match_extra << 1,             /* treat text as single line and ignore any \n's when matching ^ and $. */
   match_unused1 = match_single_line << 1,           /* unused */
   match_unused2 = match_unused1 << 1,               /* unused */
   match_unused3 = match_unused2 << 1,               /* unused */
   match_max = match_unused3,

   format_perl = 0,                                  /* perl style replacement */
   format_default = 0,                               /* ditto. */
   format_sed = match_max << 1,                      /* sed style replacement. */
   format_all = format_sed << 1,                     /* enable all extensions to syntax. */
   format_no_copy = format_all << 1,                 /* don't copy non-matching segments. */
   format_first_only = format_no_copy << 1,          /* Only replace first occurrence. */
   format_is_if = format_first_only << 1,            /* internal use only. */
   format_literal = format_is_if << 1,               /* treat string as a literal */

   match_not_any = match_not_bol | match_not_eol | match_not_bob 
      | match_not_eob | match_not_bow | match_not_eow | match_not_dot_newline 
      | match_not_dot_null | match_prev_avail | match_init | match_not_null
      | match_continuous | match_partial | match_stop | match_not_initial_null 
      | match_stop | match_all | match_perl | match_posix | match_nosubs
      | match_extra | match_single_line | match_unused1 | match_unused2 
      | match_unused3 | match_max | format_perl | format_default | format_sed
      | format_all | format_no_copy | format_first_only | format_is_if
      | format_literal


} match_flags;

#if defined(BOOST_BORLANDC) || (defined(_MSC_VER) && (_MSC_VER <= 1310))
typedef unsigned long match_flag_type;
#else
typedef match_flags match_flag_type;


#ifdef __cplusplus
inline match_flags operator&(match_flags m1, match_flags m2)
{ return static_cast<match_flags>(static_cast<boost::int32_t>(m1) & static_cast<boost::int32_t>(m2)); }
inline match_flags operator|(match_flags m1, match_flags m2)
{ return static_cast<match_flags>(static_cast<boost::int32_t>(m1) | static_cast<boost::int32_t>(m2)); }
inline match_flags operator^(match_flags m1, match_flags m2)
{ return static_cast<match_flags>(static_cast<boost::int32_t>(m1) ^ static_cast<boost::int32_t>(m2)); }
inline match_flags operator~(match_flags m1)
{ return static_cast<match_flags>(~static_cast<boost::int32_t>(m1)); }
inline match_flags& operator&=(match_flags& m1, match_flags m2)
{ m1 = m1&m2; return m1; }
inline match_flags& operator|=(match_flags& m1, match_flags m2)
{ m1 = m1|m2; return m1; }
inline match_flags& operator^=(match_flags& m1, match_flags m2)
{ m1 = m1^m2; return m1; }
#endif
#endif

#ifdef __cplusplus
} /* namespace regex_constants */
/*
 * import names into boost for backwards compatibility:
 */
using regex_constants::match_flag_type;
using regex_constants::match_default;
using regex_constants::match_not_bol;
using regex_constants::match_not_eol;
using regex_constants::match_not_bob;
using regex_constants::match_not_eob;
using regex_constants::match_not_bow;
using regex_constants::match_not_eow;
using regex_constants::match_not_dot_newline;
using regex_constants::match_not_dot_null;
using regex_constants::match_prev_avail;
/* using regex_constants::match_init; */
using regex_constants::match_any;
using regex_constants::match_not_null;
using regex_constants::match_continuous;
using regex_constants::match_partial;
/*using regex_constants::match_stop; */
using regex_constants::match_all;
using regex_constants::match_perl;
using regex_constants::match_posix;
using regex_constants::match_nosubs;
using regex_constants::match_extra;
using regex_constants::match_single_line;
/*using regex_constants::match_max; */
using regex_constants::format_all;
using regex_constants::format_sed;
using regex_constants::format_perl;
using regex_constants::format_default;
using regex_constants::format_no_copy;
using regex_constants::format_first_only;
/*using regex_constants::format_is_if;*/

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


} /* namespace boost */
#endif /* __cplusplus */
#endif /* include guard */

