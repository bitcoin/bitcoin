
#ifndef BOOST_CONTRACT_DETAIL_INLINED_DETAIL_CHECKING_HPP_
#define BOOST_CONTRACT_DETAIL_INLINED_DETAIL_CHECKING_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

// IMPORTANT: Do NOT use config macros BOOST_CONTRACT_... in this file so lib
// .cpp does not need recompiling if config changes (recompile only user code).

#include <boost/contract/detail/checking.hpp>
#include <boost/contract/detail/declspec.hpp>
#include <boost/thread/lock_guard.hpp>

namespace boost { namespace contract { namespace detail {

BOOST_CONTRACT_DETAIL_DECLINLINE
void checking::init_unlocked() { flag::ref() = true; }

BOOST_CONTRACT_DETAIL_DECLINLINE
void checking::init_locked() {
    boost::lock_guard<boost::mutex> lock(mutex::ref());
    init_unlocked();
}

BOOST_CONTRACT_DETAIL_DECLINLINE
void checking::done_unlocked() { flag::ref() = false; }

BOOST_CONTRACT_DETAIL_DECLINLINE
void checking::done_locked() {
    boost::lock_guard<boost::mutex> lock(mutex::ref());
    done_unlocked();
}

BOOST_CONTRACT_DETAIL_DECLINLINE
bool checking::already_unlocked() { return flag::ref(); }
    
BOOST_CONTRACT_DETAIL_DECLINLINE
bool checking::already_locked() {
    boost::lock_guard<boost::mutex> lock(mutex::ref());
    return already_unlocked();
}

} } } // namespace

#endif

