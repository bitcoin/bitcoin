/*
Copyright 2019 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_IO_OSTREAM_PUT_HPP
#define BOOST_IO_OSTREAM_PUT_HPP

#include <boost/io/detail/buffer_fill.hpp>
#include <boost/io/detail/ostream_guard.hpp>

namespace boost {
namespace io {

template<class charT, class traits>
inline std::basic_ostream<charT, traits>&
ostream_put(std::basic_ostream<charT, traits>& os, const charT* data,
    std::size_t size)
{
    typedef std::basic_ostream<charT, traits> stream;
    detail::ostream_guard<charT, traits> guard(os);
    typename stream::sentry entry(os);
    if (entry) {
        std::basic_streambuf<charT, traits>& buf = *os.rdbuf();
        std::size_t width = static_cast<std::size_t>(os.width());
        if (width <= size) {
            if (static_cast<std::size_t>(buf.sputn(data, size)) != size) {
                return os;
            }
        } else if ((os.flags() & stream::adjustfield) == stream::left) {
            if (static_cast<std::size_t>(buf.sputn(data, size)) != size ||
                !detail::buffer_fill(buf, os.fill(), width - size)) {
                return os;
            }
        } else if (!detail::buffer_fill(buf, os.fill(), width - size) ||
            static_cast<std::size_t>(buf.sputn(data, size)) != size) {
            return os;
        }
        os.width(0);
    }
    guard.release();
    return os;
}

} /* io */
} /* boost */

#endif
