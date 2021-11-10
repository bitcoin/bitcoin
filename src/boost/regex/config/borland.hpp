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
  *   FILE         boost/regex/config/borland.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: regex borland-specific config setup.
  */


#if defined(__BORLANDC__) && !defined(__clang__)
#  if (__BORLANDC__ == 0x550) || (__BORLANDC__ == 0x551)
      // problems with std::basic_string and dll RTL:
#     if defined(_RTLDLL) && defined(_RWSTD_COMPILE_INSTANTIATE)
#        ifdef BOOST_REGEX_BUILD_DLL
#           error _RWSTD_COMPILE_INSTANTIATE must not be defined when building regex++ as a DLL
#        else
#           pragma message("Defining _RWSTD_COMPILE_INSTANTIATE when linking to the DLL version of the RTL may produce memory corruption problems in std::basic_string, as a result of separate versions of basic_string's static data in the RTL and you're exe/dll: be warned!!")
#        endif
#     endif
#     ifndef _RTLDLL
         // this is harmless for a staic link:
#        define _RWSTD_COMPILE_INSTANTIATE
#     endif
      // external templates cause problems for some reason:
#     define BOOST_REGEX_NO_EXTERNAL_TEMPLATES
#  endif
#  if (__BORLANDC__ <= 0x540) && !defined(BOOST_REGEX_NO_LIB) && !defined(_NO_VCL)
      // C++ Builder 4 and earlier, we can't tell whether we should be using
      // the VCL runtime or not, do a static link instead:
#     define BOOST_REGEX_STATIC_LINK
#  endif
   //
   // VCL support:
   // if we're building a console app then there can't be any VCL (can there?)
#  if !defined(__CONSOLE__) && !defined(_NO_VCL)
#     define BOOST_REGEX_USE_VCL
#  endif
   //
   // if this isn't Win32 then don't automatically select link
   // libraries:
   //
#  ifndef _Windows
#     ifndef BOOST_REGEX_NO_LIB
#        define BOOST_REGEX_NO_LIB
#     endif
#     ifndef BOOST_REGEX_STATIC_LINK
#        define BOOST_REGEX_STATIC_LINK
#     endif
#  endif

#if __BORLANDC__ < 0x600
//
// string workarounds:
//
#include <cstring>
#undef strcmp
#undef strcpy
#endif

#endif


