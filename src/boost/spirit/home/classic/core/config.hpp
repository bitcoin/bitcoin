/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    http://spirit.sourceforge.net/

  Distributed under the Boost Software License, Version 1.0. (See accompanying
  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_CONFIG_HPP)
#define BOOST_SPIRIT_CONFIG_HPP

#include <boost/config.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  Compiler check:
//
//  Historically, Spirit supported a lot of compilers, including (to some
//  extent) poorly conforming compilers such as VC6. Spirit v1.6.x will be
//  the last release that will support older poorly conforming compilers.
//  Starting from Spirit v1.8.0, ill conforming compilers will not be
//  supported. If you are still using one of these older compilers, you can
//  still use Spirit v1.6.x.
//
//  The reason why Spirit v1.6.x worked on old non-conforming compilers is
//  that the authors laboriously took the trouble of searching for
//  workarounds to make these compilers happy. The process takes a lot of
//  time and energy, especially when one encounters the dreaded ICE or
//  "Internal Compiler Error". Sometimes searching for a single workaround
//  takes days or even weeks. Sometimes, there are no known workarounds. This
//  stifles progress a lot. And, as the library gets more progressive and
//  takes on more advanced C++ techniques, the difficulty is escalated to
//  even new heights.
//
//  Spirit v1.6.x will still be supported. Maintenance and bug fixes will
//  still be applied. There will still be active development for the back-
//  porting of new features introduced in Spirit v1.8.0 (and Spirit 1.9.0)
//  to lesser able compilers; hopefully, fueled by contributions from the
//  community. For instance, there is already a working AST tree back-port
//  for VC6 and VC7 by Peder Holt.
//
//  If you got here somehow, your compiler is known to be poorly conforming
//  WRT ANSI/ISO C++ standard. Library implementers get a bad reputation when
//  someone attempts to compile the code on a non-conforming compiler. She'll
//  be confronted with tons of compiler errors when she tries to compile the
//  library. Such errors will somehow make less informed users conclude that
//  the code is poorly written. It's better for the user to see a message
//  "sorry, this code has not been ported to your compiler yet", than to see
//  pages and pages of compiler error messages.
//
/////////////////////////////////////////////////////////////////////////////////
#if     (defined(BOOST_MSVC) && (BOOST_MSVC < 1310))                            \
    ||  (defined(BOOST_BORLANDC) && (BOOST_BORLANDC <= 0x570))                      \
    ||  (defined(__GNUC__) && (__GNUC__ < 3))                                   \
    ||  (defined(__GNUC__) && (__GNUC__ == 3) && (__GNUC_MINOR__ < 1))
# error "Compiler not supported. See note in <boost/spirit/core/config.hpp>"
#else
// Pass... Compiler supported.
#endif

#endif


