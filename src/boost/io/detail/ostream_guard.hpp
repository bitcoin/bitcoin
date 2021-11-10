/*
Copyright 2019-2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License, Version 1.0.
(http://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef BOOST_IO_DETAIL_OSTREAM_GUARD_HPP
#define BOOST_IO_DETAIL_OSTREAM_GUARD_HPP

#include <boost/config.hpp>
#include <iosfwd>

namespace boost {
namespace io {
namespace detail {

template<class Char, class Traits>
class ostream_guard {
public:
    explicit ostream_guard(std::basic_ostream<Char, Traits>& os) BOOST_NOEXCEPT
        : os_(&os) { }

    ~ostream_guard() BOOST_NOEXCEPT_IF(false) {
        if (os_) {
            os_->setstate(std::basic_ostream<Char, Traits>::badbit);
        }
    }

    void release() BOOST_NOEXCEPT {
        os_ = 0;
    }

private:
    ostream_guard(const ostream_guard&);
    ostream_guard& operator=(const ostream_guard&);

    std::basic_ostream<Char, Traits>* os_;
};

} /* detail */
} /* io */
} /* boost */

#endif
