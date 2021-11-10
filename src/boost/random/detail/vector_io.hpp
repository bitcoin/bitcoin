/* boost random/vector_io.hpp header file
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org for most recent version including documentation.
 *
 * $Id$
 */

#ifndef BOOST_RANDOM_DETAIL_VECTOR_IO_HPP
#define BOOST_RANDOM_DETAIL_VECTOR_IO_HPP

#include <vector>
#include <iosfwd>
#include <istream>
#include <boost/io/ios_state.hpp>

namespace boost {
namespace random {
namespace detail {

template<class CharT, class Traits, class T>
void print_vector(std::basic_ostream<CharT, Traits>& os,
                  const std::vector<T>& vec)
{
    typename std::vector<T>::const_iterator
        iter = vec.begin(),
        end =  vec.end();
    os << os.widen('[');
    if(iter != end) {
        os << *iter;
        ++iter;
        for(; iter != end; ++iter)
        {
            os << os.widen(' ') << *iter;
        }
    }
    os << os.widen(']');
}

template<class CharT, class Traits, class T>
void read_vector(std::basic_istream<CharT, Traits>& is, std::vector<T>& vec)
{
    CharT ch;
    if(!(is >> ch)) {
        return;
    }
    if(ch != is.widen('[')) {
        is.putback(ch);
        is.setstate(std::ios_base::failbit);
        return;
    }
    boost::io::basic_ios_exception_saver<CharT, Traits> e(is, std::ios_base::goodbit);
    T val;
    while(is >> std::ws >> val) {
        vec.push_back(val);
    }
    if(is.fail()) {
        is.clear();
        e.restore();
        if(!(is >> ch)) {
            return;
        }
        if(ch != is.widen(']')) {
            is.putback(ch);
            is.setstate(std::ios_base::failbit);
        }
    }
}

}
}
}

#endif // BOOST_RANDOM_DETAIL_VECTOR_IO_HPP
