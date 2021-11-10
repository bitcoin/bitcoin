//  Boost string_algo library yes_no_type.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_YES_NO_TYPE_DETAIL_HPP
#define BOOST_STRING_YES_NO_TYPE_DETAIL_HPP

namespace boost {
    namespace algorithm {

        // taken from boost mailing-list
        // when yes_no_type will become officially
        // a part of boost distribution, this header
        // will be deprecated
        template<int I> struct size_descriptor 
        {
            typedef char (& type)[I];
        }; 

        typedef size_descriptor<1>::type yes_type;
        typedef size_descriptor<2>::type no_type;

    } // namespace algorithm
} // namespace boost


#endif  // BOOST_STRING_YES_NO_TYPE_DETAIL_HPP
