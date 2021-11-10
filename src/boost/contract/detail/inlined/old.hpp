
#ifndef BOOST_CONTRACT_DETAIL_INLINED_OLD_HPP_
#define BOOST_CONTRACT_DETAIL_INLINED_OLD_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

// IMPORTANT: Do NOT use config macros BOOST_CONTRACT_... in this file so lib
// .cpp does not need recompiling if config changes (recompile only user code).

#include <boost/contract/old.hpp>
#include <boost/contract/detail/declspec.hpp>

namespace boost { namespace contract {

BOOST_CONTRACT_DETAIL_DECLINLINE
old_value null_old() { return old_value(); }

BOOST_CONTRACT_DETAIL_DECLINLINE
old_pointer make_old(old_value const& old) {
    return old_pointer(0, old);
}

BOOST_CONTRACT_DETAIL_DECLINLINE
old_pointer make_old(virtual_* v, old_value const& old) {
    return old_pointer(v, old);
}

} } // namespacd

#endif // #include guard

