
#ifndef BOOST_CONTRACT_DETAIL_CHECK_HPP_
#define BOOST_CONTRACT_DETAIL_CHECK_HPP_

// Copyright (C) 2008-2018 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0 (see accompanying
// file LICENSE_1_0.txt or a copy at http://www.boost.org/LICENSE_1_0.txt).
// See: http://www.boost.org/doc/libs/release/libs/contract/doc/html/index.html

#include <boost/contract/core/config.hpp> 
#ifndef BOOST_CONTRACT_NO_CHECKS
    #include <boost/contract/core/exception.hpp>

    /* PRIVATE */

    #ifndef BOOST_CONTRACT_ALL_DISABLE_NO_ASSERTION
        #include <boost/contract/detail/checking.hpp>
        #include <boost/contract/detail/name.hpp>

        #define BOOST_CONTRACT_CHECK_IF_NOT_CHECKING_ALREADY_ \
            if(!boost::contract::detail::checking::already())
        #define BOOST_CONTRACT_CHECK_CHECKING_VAR_(guard) \
            /* this name somewhat unique to min var shadow warnings */ \
            boost::contract::detail::checking BOOST_CONTRACT_DETAIL_NAME2( \
                    guard, __LINE__);
    #else
        #define BOOST_CONTRACT_CHECK_IF_NOT_CHECKING_ALREADY_ /* nothing */
        #define BOOST_CONTRACT_CHECK_CHECKING_VAR_(guard) /* nothing */
    #endif
    
    /* PUBLIC */
    
    #define BOOST_CONTRACT_DETAIL_CHECK(assertion) \
        { \
            try { \
                BOOST_CONTRACT_CHECK_IF_NOT_CHECKING_ALREADY_ \
                { \
                    BOOST_CONTRACT_CHECK_CHECKING_VAR_(k) \
                    { assertion; } \
                } \
            } catch(...) { boost::contract::check_failure(); } \
        }
#else
    #define BOOST_CONTRACT_DETAIL_CHECK(assertion) {}
#endif

#endif // #include guard

