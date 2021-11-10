
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_SPINLOCK_STATUS_H
#define BOOST_FIBERS_SPINLOCK_STATUS_H

namespace boost {
namespace fibers {
namespace detail {

enum class spinlock_status {
    locked = 0,
    unlocked
};

}}}

#endif // BOOST_FIBERS_SPINLOCK_STATUS_H
