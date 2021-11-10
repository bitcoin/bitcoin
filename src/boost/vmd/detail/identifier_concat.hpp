
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IDENTIFIER_CONCAT_HPP)
#define BOOST_VMD_DETAIL_IDENTIFIER_CONCAT_HPP

#include <boost/preprocessor/cat.hpp>
#include <boost/vmd/detail/idprefix.hpp>

#define BOOST_VMD_DETAIL_IDENTIFIER_CONCATENATE(vseq) \
    BOOST_PP_CAT \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_REGISTRATION_PREFIX, \
        vseq \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IDENTIFIER_CONCAT_HPP */
