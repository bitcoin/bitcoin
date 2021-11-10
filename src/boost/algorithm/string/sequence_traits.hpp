//  Boost string_algo library sequence_traits.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_SEQUENCE_TRAITS_HPP
#define BOOST_STRING_SEQUENCE_TRAITS_HPP

#include <boost/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/algorithm/string/yes_no_type.hpp>

/*! \file
    Traits defined in this header are used by various algorithms to achieve
    better performance for specific containers.
    Traits provide fail-safe defaults. If a container supports some of these
    features, it is possible to specialize the specific trait for this container.
    For lacking compilers, it is possible of define an override for a specific tester
    function.

    Due to a language restriction, it is not currently possible to define specializations for
    stl containers without including the corresponding header. To decrease the overhead
    needed by this inclusion, user can selectively include a specialization
    header for a specific container. They are located in boost/algorithm/string/stl
    directory. Alternatively she can include boost/algorithm/string/std_collection_traits.hpp
    header which contains specializations for all stl containers.
*/

namespace boost {
    namespace algorithm {

//  sequence traits  -----------------------------------------------//


        //! Native replace trait
        /*!
            This trait specifies that the sequence has \c std::string like replace method
        */
        template< typename T >
        class has_native_replace
        {

        public:
#    if BOOST_WORKAROUND( __IBMCPP__, <= 600 )
            enum { value = false };
#    else
            BOOST_STATIC_CONSTANT(bool, value=false);
#    endif // BOOST_WORKAROUND( __IBMCPP__, <= 600 )


            typedef mpl::bool_<has_native_replace<T>::value> type;
        };


        //! Stable iterators trait
        /*!
            This trait specifies that the sequence has stable iterators. It means
            that operations like insert/erase/replace do not invalidate iterators.
        */
        template< typename T >
        class has_stable_iterators
        {
        public:
#    if BOOST_WORKAROUND( __IBMCPP__, <= 600 )
            enum { value = false };
#    else
            BOOST_STATIC_CONSTANT(bool, value=false);
#    endif // BOOST_WORKAROUND( __IBMCPP__, <= 600 )

            typedef mpl::bool_<has_stable_iterators<T>::value> type;
        };


        //! Const time insert trait
        /*!
            This trait specifies that the sequence's insert method has
            constant time complexity.
        */
        template< typename T >
        class has_const_time_insert
        {
        public:
#    if BOOST_WORKAROUND( __IBMCPP__, <= 600 )
            enum { value = false };
#    else
            BOOST_STATIC_CONSTANT(bool, value=false);
#    endif // BOOST_WORKAROUND( __IBMCPP__, <= 600 )

            typedef mpl::bool_<has_const_time_insert<T>::value> type;
        };


        //! Const time erase trait
        /*!
            This trait specifies that the sequence's erase method has
            constant time complexity.
        */
        template< typename T >
        class has_const_time_erase
        {
        public:
#    if BOOST_WORKAROUND( __IBMCPP__, <= 600 )
            enum { value = false };
#    else
            BOOST_STATIC_CONSTANT(bool, value=false);
#    endif // BOOST_WORKAROUND( __IBMCPP__, <= 600 )

            typedef mpl::bool_<has_const_time_erase<T>::value> type;
        };

    } // namespace algorithm
} // namespace boost


#endif  // BOOST_STRING_SEQUENCE_TRAITS_HPP
