
//  (C) Copyright Edward Diener 2011-2015
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(BOOST_VMD_DETAIL_IS_TYPE_HPP)
#define BOOST_VMD_DETAIL_IS_TYPE_HPP

#include <boost/preprocessor/control/iif.hpp>
#include <boost/vmd/identity.hpp>
#include <boost/vmd/detail/identifier_type.hpp>
#include <boost/vmd/detail/is_type_type.hpp>
#include <boost/vmd/detail/is_identifier.hpp>
#include <boost/vmd/detail/type_registration.hpp>

#define BOOST_VMD_DETAIL_IS_TYPE_IMP_TYPE(vseq) \
    BOOST_VMD_DETAIL_IS_TYPE_TYPE \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_TYPE(vseq) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_TYPE_IMP_TYPE_D(d,vseq) \
    BOOST_VMD_DETAIL_IS_TYPE_TYPE \
        ( \
        BOOST_VMD_DETAIL_IDENTIFIER_TYPE_D(d,vseq) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_TYPE(vseq) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_IDENTIFIER_SINGLE(vseq), \
            BOOST_VMD_DETAIL_IS_TYPE_IMP_TYPE, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (vseq) \
        ) \
/**/

#define BOOST_VMD_DETAIL_IS_TYPE_D(d,vseq) \
    BOOST_VMD_IDENTITY_RESULT \
        ( \
        BOOST_PP_IIF \
            ( \
            BOOST_VMD_DETAIL_IS_IDENTIFIER_SINGLE_D(d,vseq), \
            BOOST_VMD_DETAIL_IS_TYPE_IMP_TYPE_D, \
            BOOST_VMD_IDENTITY(0) \
            ) \
        (d,vseq) \
        ) \
/**/

#endif /* BOOST_VMD_DETAIL_IS_TYPE_HPP */
