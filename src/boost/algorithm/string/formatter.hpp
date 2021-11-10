//  Boost string_algo library formatter.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_FORMATTER_HPP
#define BOOST_STRING_FORMATTER_HPP

#include <boost/range/value_type.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/as_literal.hpp>

#include <boost/algorithm/string/detail/formatter.hpp>

/*! \file
    Defines Formatter generators. Formatter is a functor which formats
    a string according to given parameters. A Formatter works
    in conjunction with a Finder. A Finder can provide additional information
    for a specific Formatter. An example of such a cooperation is regex_finder
    and regex_formatter.

    Formatters are used as pluggable components for replace facilities. 
    This header contains generator functions for the Formatters provided in this library.
*/

namespace boost {
    namespace algorithm {

// generic formatters  ---------------------------------------------------------------//

        //! Constant formatter
        /*!
            Constructs a \c const_formatter. Const formatter always returns
            the same value, regardless of the parameter.

            \param Format A predefined value used as a result for formatting
            \return An instance of the \c const_formatter object.
        */
        template<typename RangeT>
        inline detail::const_formatF<
            iterator_range<
                BOOST_STRING_TYPENAME range_const_iterator<RangeT>::type> >
        const_formatter(const RangeT& Format)
        {
            return detail::const_formatF<
                iterator_range<
                    BOOST_STRING_TYPENAME range_const_iterator<RangeT>::type> >(::boost::as_literal(Format));
        }

        //! Identity formatter
        /*!
            Constructs an \c identity_formatter. Identity formatter always returns
            the parameter.

            \return An instance of the \c identity_formatter object.
        */
        template<typename RangeT>
        inline detail::identity_formatF<
            iterator_range<
                BOOST_STRING_TYPENAME range_const_iterator<RangeT>::type> >
        identity_formatter()
        {
            return detail::identity_formatF<
                iterator_range<
                    BOOST_STRING_TYPENAME range_const_iterator<RangeT>::type> >();
        }

        //! Empty formatter
        /*!
            Constructs an \c empty_formatter. Empty formatter always returns an empty
            sequence. 

            \param Input container used to select a correct value_type for the
                         resulting empty_container<>.
            \return An instance of the \c empty_formatter object.
        */
        template<typename RangeT>
        inline detail::empty_formatF< 
            BOOST_STRING_TYPENAME range_value<RangeT>::type>
        empty_formatter(const RangeT&)
        {
            return detail::empty_formatF<
                BOOST_STRING_TYPENAME range_value<RangeT>::type>();
        }

        //! Empty formatter
        /*!
            Constructs a \c dissect_formatter. Dissect formatter uses a specified finder
            to extract a portion of the formatted sequence. The first finder's match is returned 
            as a result

            \param Finder a finder used to select a portion of the formatted sequence
            \return An instance of the \c dissect_formatter object.
        */
        template<typename FinderT>
        inline detail::dissect_formatF< FinderT >
        dissect_formatter(const FinderT& Finder)
        {
            return detail::dissect_formatF<FinderT>(Finder);
        }


    } // namespace algorithm

    // pull the names to the boost namespace
    using algorithm::const_formatter;
    using algorithm::identity_formatter;
    using algorithm::empty_formatter;
    using algorithm::dissect_formatter;

} // namespace boost


#endif  // BOOST_FORMATTER_HPP
