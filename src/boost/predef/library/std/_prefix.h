/*
Copyright Rene Rivera 2008-2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_PREDEF_LIBRARY_STD__PREFIX_H
#define BOOST_PREDEF_LIBRARY_STD__PREFIX_H

/*
We need to include an STD header to gives us the context
of which library we are using. The "smallest" code-wise header
seems to be <exception>. Boost uses <utility> but as far
as I can tell (RR) it's not a stand-alone header in most
implementations. Using <exception> also has the benefit of
being available in EC++, so we get a chance to make this work
for embedded users. And since it's not a header impacted by TR1
there's no magic needed for inclusion in the face of the
Boost.TR1 library.
*/
#include <boost/predef/detail/_exception.h>

#endif
