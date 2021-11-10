//  Boost string_algo library predicate_facade.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_PREDICATE_FACADE_HPP
#define BOOST_STRING_PREDICATE_FACADE_HPP

#include <boost/algorithm/string/config.hpp>

/*
 \file boost/algorith/string/predicate_facade.hpp
 This file contains predicate_facade definition. This template class is used
 to identify classification predicates, so they can be combined using
 composition operators.
*/

namespace boost {
    namespace algorithm {

//  predicate facade ------------------------------------------------------//

        //! Predicate facade
        /*!
            This class allows to recognize classification
            predicates, so that they can be combined using
            composition operators.
            Every classification predicate must be derived from this class.
        */
        template<typename Derived>
        struct predicate_facade {};

    } // namespace algorithm
} // namespace boost


#endif  // BOOST_STRING_CLASSIFICATION_DETAIL_HPP
