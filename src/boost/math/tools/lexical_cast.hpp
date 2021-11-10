//  (C) Copyright Matt Borland 2021
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_LEXICAL_CAST
#define BOOST_MATH_TOOLS_LEXICAL_CAST

#include <boost/math/tools/is_standalone.hpp>

#ifndef BOOST_MATH_STANDALONE
#include <boost/lexical_cast.hpp>
#else

#ifndef BOOST_MATH_NO_LEXICAL_CAST
#define BOOST_MATH_NO_LEXICAL_CAST
#endif

namespace boost {
    template <typename T1, typename T2>
    inline T1 lexical_cast(const T2)
    {
        static_assert(sizeof(T1) == 0, "boost.lexical_cast can not be used in standalone mode. Please disable standalone mode and try again");
        return T1(0);
    }
}
#endif // BOOST_MATH_STANDALONE
#endif // BOOST_MATH_TOOLS_LEXICAL_CAST
