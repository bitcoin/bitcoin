#ifndef BOOST_METAPARSE_V1_IMPL_NO_CHAR_HPP
#define BOOST_METAPARSE_V1_IMPL_NO_CHAR_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>

#ifdef BOOST_NO_CHAR
#  error BOOST_NO_CHAR already defined
#endif
#define BOOST_NO_CHAR EOF

#endif

