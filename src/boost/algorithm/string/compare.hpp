//  Boost string_algo library compare.hpp header file  -------------------------//

//  Copyright Pavol Droba 2002-2006.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_COMPARE_HPP
#define BOOST_STRING_COMPARE_HPP

#include <boost/algorithm/string/config.hpp>
#include <locale>

/*! \file
    Defines element comparison predicates. Many algorithms in this library can
    take an additional argument with a predicate used to compare elements.
    This makes it possible, for instance, to have case insensitive versions
    of the algorithms.
*/

namespace boost {
    namespace algorithm {

        //  is_equal functor  -----------------------------------------------//

        //! is_equal functor
        /*!
            Standard STL equal_to only handle comparison between arguments
            of the same type. This is a less restrictive version which wraps operator ==.
        */
        struct is_equal
        {
            //! Function operator
            /*!
                Compare two operands for equality
            */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                return Arg1==Arg2;
            }
        };

        //! case insensitive version of is_equal
        /*!
            Case insensitive comparison predicate. Comparison is done using
            specified locales.
        */
        struct is_iequal
        {
            //! Constructor
            /*!
                \param Loc locales used for comparison
            */
            is_iequal( const std::locale& Loc=std::locale() ) :
                m_Loc( Loc ) {}

            //! Function operator
            /*!
                Compare two operands. Case is ignored.
            */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                #if defined(BOOST_BORLANDC) && (BOOST_BORLANDC >= 0x560) && (BOOST_BORLANDC <= 0x564) && !defined(_USE_OLD_RW_STL)
                    return std::toupper(Arg1)==std::toupper(Arg2);
                #else
                    return std::toupper<T1>(Arg1,m_Loc)==std::toupper<T2>(Arg2,m_Loc);
                #endif
            }

        private:
            std::locale m_Loc;
        };

        //  is_less functor  -----------------------------------------------//

        //! is_less functor
        /*!
            Convenient version of standard std::less. Operation is templated, therefore it is 
            not required to specify the exact types upon the construction
         */
        struct is_less
        {
            //! Functor operation
            /*!
                Compare two operands using > operator
             */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                return Arg1<Arg2;
            }
        };


        //! case insensitive version of is_less
        /*!
            Case insensitive comparison predicate. Comparison is done using
            specified locales.
        */
        struct is_iless
        {
            //! Constructor
            /*!
                \param Loc locales used for comparison
            */
            is_iless( const std::locale& Loc=std::locale() ) :
                m_Loc( Loc ) {}

            //! Function operator
            /*!
                Compare two operands. Case is ignored.
            */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                #if defined(BOOST_BORLANDC) && (BOOST_BORLANDC >= 0x560) && (BOOST_BORLANDC <= 0x564) && !defined(_USE_OLD_RW_STL)
                    return std::toupper(Arg1)<std::toupper(Arg2);
                #else
                    return std::toupper<T1>(Arg1,m_Loc)<std::toupper<T2>(Arg2,m_Loc);
                #endif
            }

        private:
            std::locale m_Loc;
        };

        //  is_not_greater functor  -----------------------------------------------//

        //! is_not_greater functor
        /*!
            Convenient version of standard std::not_greater_to. Operation is templated, therefore it is 
            not required to specify the exact types upon the construction
         */
        struct is_not_greater
        {
            //! Functor operation
            /*!
                Compare two operands using > operator
             */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                return Arg1<=Arg2;
            }
        };


        //! case insensitive version of is_not_greater
        /*!
            Case insensitive comparison predicate. Comparison is done using
            specified locales.
        */
        struct is_not_igreater
        {
            //! Constructor
            /*!
                \param Loc locales used for comparison
            */
            is_not_igreater( const std::locale& Loc=std::locale() ) :
                m_Loc( Loc ) {}

            //! Function operator
            /*!
                Compare two operands. Case is ignored.
            */
            template< typename T1, typename T2 >
                bool operator()( const T1& Arg1, const T2& Arg2 ) const
            {
                #if defined(BOOST_BORLANDC) && (BOOST_BORLANDC >= 0x560) && (BOOST_BORLANDC <= 0x564) && !defined(_USE_OLD_RW_STL)
                    return std::toupper(Arg1)<=std::toupper(Arg2);
                #else
                    return std::toupper<T1>(Arg1,m_Loc)<=std::toupper<T2>(Arg2,m_Loc);
                #endif
            }

        private:
            std::locale m_Loc;
        };


    } // namespace algorithm

    // pull names to the boost namespace
    using algorithm::is_equal;
    using algorithm::is_iequal;
    using algorithm::is_less;
    using algorithm::is_iless;
    using algorithm::is_not_greater;
    using algorithm::is_not_igreater;

} // namespace boost


#endif  // BOOST_STRING_COMPARE_HPP
