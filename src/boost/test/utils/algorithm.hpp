//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
/// @file
/// Addition to STL algorithms
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_ALGORITHM_HPP
#define BOOST_TEST_UTILS_ALGORITHM_HPP

// STL
#include <utility>
#include <algorithm> // std::find
#include <functional> // std::bind1st or std::bind

#include <boost/test/detail/suppress_warnings.hpp>

#ifdef BOOST_NO_CXX98_BINDERS
#define BOOST_TEST_BIND1ST(F,A) std::bind( (F), (A), std::placeholders::_1 )
#else
#define BOOST_TEST_BIND1ST(F,A) std::bind1st( (F), (A) )
#endif

//____________________________________________________________________________//

namespace boost {
namespace unit_test {
namespace utils {

/// @brief this algorithm search through two collections for first mismatch position that get returned as a pair
/// of iterators, first pointing to the mismatch position in first collection, second iterator in second one
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
template <class InputIter1, class InputIter2>
inline std::pair<InputIter1, InputIter2>
mismatch( InputIter1 first1, InputIter1 last1,
          InputIter2 first2, InputIter2 last2 )
{
    while( first1 != last1 && first2 != last2 && *first1 == *first2 ) {
        ++first1;
        ++first2;
    }

    return std::pair<InputIter1, InputIter2>(first1, first2);
}

//____________________________________________________________________________//

/// @brief this algorithm search through two collections for first mismatch position that get returned as a pair
/// of iterators, first pointing to the mismatch position in first collection, second iterator in second one. This algorithms
/// uses supplied predicate for collection elements comparison
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
/// @param pred - predicate to be used for search
template <class InputIter1, class InputIter2, class Predicate>
inline std::pair<InputIter1, InputIter2>
mismatch( InputIter1 first1, InputIter1 last1,
          InputIter2 first2, InputIter2 last2,
          Predicate pred )
{
    while( first1 != last1 && first2 != last2 && pred( *first1, *first2 ) ) {
        ++first1;
        ++first2;
    }

    return std::pair<InputIter1, InputIter2>(first1, first2);
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for first element that does not belong a second one
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
template<class ForwardIterator1, class ForwardIterator2>
inline ForwardIterator1
find_first_not_of( ForwardIterator1 first1, ForwardIterator1 last1,
                   ForwardIterator2 first2, ForwardIterator2 last2 )
{
    while( first1 != last1 ) {
        if( std::find( first2, last2, *first1 ) == last2 )
            break;
        ++first1;
    }

    return first1;
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for first element that does not satisfy binary
/// predicate in conjunction will any element in second collection
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
/// @param pred - predicate to be used for search
template<class ForwardIterator1, class ForwardIterator2, class Predicate>
inline ForwardIterator1
find_first_not_of( ForwardIterator1 first1, ForwardIterator1 last1,
                   ForwardIterator2 first2, ForwardIterator2 last2,
                   Predicate pred )
{
    while( first1 != last1 ) {
        if( std::find_if( first2, last2, BOOST_TEST_BIND1ST( pred, *first1 ) ) == last2 )
            break;
        ++first1;
    }

    return first1;
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for last element that belongs to a second one
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
template<class BidirectionalIterator1, class ForwardIterator2>
inline BidirectionalIterator1
find_last_of( BidirectionalIterator1 first1, BidirectionalIterator1 last1,
              ForwardIterator2 first2, ForwardIterator2 last2 )
{
    if( first1 == last1 || first2 == last2 )
        return last1;

    BidirectionalIterator1 it1 = last1;
    while( --it1 != first1 && std::find( first2, last2, *it1 ) == last2 ) {}

    return it1 == first1 && std::find( first2, last2, *it1 ) == last2 ? last1 : it1;
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for last element that satisfy binary
/// predicate in conjunction will at least one element in second collection
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
/// @param pred - predicate to be used for search
template<class BidirectionalIterator1, class ForwardIterator2, class Predicate>
inline BidirectionalIterator1
find_last_of( BidirectionalIterator1 first1, BidirectionalIterator1 last1,
              ForwardIterator2 first2, ForwardIterator2 last2,
              Predicate pred )
{
    if( first1 == last1 || first2 == last2 )
        return last1;

    BidirectionalIterator1 it1 = last1;
    while( --it1 != first1 && std::find_if( first2, last2, BOOST_TEST_BIND1ST( pred, *it1 ) ) == last2 ) {}

    return it1 == first1 && std::find_if( first2, last2, BOOST_TEST_BIND1ST( pred, *it1 ) ) == last2 ? last1 : it1;
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for last element that does not belong to a second one
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
template<class BidirectionalIterator1, class ForwardIterator2>
inline BidirectionalIterator1
find_last_not_of( BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                  ForwardIterator2 first2, ForwardIterator2 last2 )
{
    if( first1 == last1 || first2 == last2 )
        return last1;

    BidirectionalIterator1 it1 = last1;
    while( --it1 != first1 && std::find( first2, last2, *it1 ) != last2 ) {}

    return it1 == first1 && std::find( first2, last2, *it1 ) != last2 ? last1 : it1;
}

//____________________________________________________________________________//

/// @brief this algorithm search through first collection for last element that does not satisfy binary
/// predicate in conjunction will any element in second collection
///
/// @param first1 - first collection begin iterator
/// @param last1 - first collection end iterator
/// @param first2 - second collection begin iterator
/// @param last2 - second collection end iterator
/// @param pred - predicate to be used for search
template<class BidirectionalIterator1, class ForwardIterator2, class Predicate>
inline BidirectionalIterator1
find_last_not_of( BidirectionalIterator1 first1, BidirectionalIterator1 last1,
                  ForwardIterator2 first2, ForwardIterator2 last2,
                  Predicate pred )
{
    if( first1 == last1 || first2 == last2 )
        return last1;

    BidirectionalIterator1 it1 = last1;
    while( --it1 != first1 && std::find_if( first2, last2, BOOST_TEST_BIND1ST( pred, *it1 ) ) != last2 ) {}

    return it1 == first1 && std::find_if( first2, last2, BOOST_TEST_BIND1ST( pred, *it1 ) ) == last2 ? last1 : it1;
}

//____________________________________________________________________________//


/// @brief This algorithm replaces all occurrences of a set of substrings by another substrings
///
/// @param str - string of operation
/// @param first1 - iterator to the beginning of the substrings to replace
/// @param last1 - iterator to the end of the substrings to replace
/// @param first2 - iterator to the beginning of the substrings to replace with
/// @param last2 - iterator to the end of the substrings to replace with
template<class StringClass, class ForwardIterator>
inline StringClass
replace_all_occurrences_of( StringClass str,
                            ForwardIterator first1, ForwardIterator last1,
                            ForwardIterator first2, ForwardIterator last2)
{
    for(; first1 != last1 && first2 != last2; ++first1, ++first2) {
        std::size_t found = str.find( *first1 );
        while( found != StringClass::npos ) {
            str.replace(found, first1->size(), *first2 );
            found = str.find( *first1, found + first2->size() );
        }
    }

    return str;
}

/// @brief This algorithm replaces all occurrences of a string with basic wildcards
/// with another (optionally containing wildcards as well).
///
/// @param str - string to transform
/// @param it_string_to_find - iterator to the beginning of the substrings to replace
/// @param it_string_to_find_end - iterator to the end of the substrings to replace
/// @param it_string_to_replace - iterator to the beginning of the substrings to replace with
/// @param it_string_to_replace_end - iterator to the end of the substrings to replace with
///
/// The wildcard is the symbol '*'. Only a unique wildcard per string is supported. The replacement
/// string may also contain a wildcard, in which case it is considered as a placeholder to the content
/// of the wildcard in the source string.
/// Example:
/// - In order to replace the occurrences of @c 'time=\"some-variable-value\"' to a constant string,
///   one may use @c 'time=\"*\"' as the string to search for, and 'time=\"0.0\"' as the replacement string.
/// - In order to replace the occurrences of 'file.cpp(XX)' per 'file.cpp:XX', where XX is a variable to keep,
///   on may use @c 'file.cpp(*)' as the string to search for, and 'file.cpp:*' as the replacement string.
template<class StringClass, class ForwardIterator>
inline StringClass
replace_all_occurrences_with_wildcards(
    StringClass str,
    ForwardIterator it_string_to_find, ForwardIterator it_string_to_find_end,
    ForwardIterator it_string_to_replace, ForwardIterator it_string_to_replace_end)
{
    for(; it_string_to_find != it_string_to_find_end && it_string_to_replace != it_string_to_replace_end;
        ++it_string_to_find, ++ it_string_to_replace) {

        std::size_t wildcard_pos = it_string_to_find->find("*");
        if(wildcard_pos == StringClass::npos) {
            ForwardIterator it_to_find_current_end(it_string_to_find);
            ForwardIterator it_to_replace_current_end(it_string_to_replace);
            str = replace_all_occurrences_of(
                str,
                it_string_to_find, ++it_to_find_current_end,
                it_string_to_replace, ++it_to_replace_current_end);
            continue;
        }

        std::size_t wildcard_pos_replace = it_string_to_replace->find("*");

        std::size_t found_begin = str.find( it_string_to_find->substr(0, wildcard_pos) );
        while( found_begin != StringClass::npos ) {
            std::size_t found_end = str.find(it_string_to_find->substr(wildcard_pos+1), found_begin + wildcard_pos + 1); // to simplify
            if( found_end != StringClass::npos ) {

                if( wildcard_pos_replace == StringClass::npos ) {
                    StringClass replace_content = *it_string_to_replace;
                    str.replace(
                        found_begin,
                        found_end + (it_string_to_find->size() - wildcard_pos - 1 ) - found_begin,
                        replace_content);
                } else {
                    StringClass replace_content =
                        it_string_to_replace->substr(0, wildcard_pos_replace)
                        + str.substr(found_begin + wildcard_pos,
                                     found_end - found_begin - wildcard_pos)
                        + it_string_to_replace->substr(wildcard_pos_replace+1) ;
                    str.replace(
                        found_begin,
                        found_end + (it_string_to_find->size() - wildcard_pos - 1 ) - found_begin,
                        replace_content);

                }
            }

            // may adapt the restart to the replacement and be more efficient
            found_begin = str.find( it_string_to_find->substr(0, wildcard_pos), found_begin + 1 );
       }
    }

    return str;
}

} // namespace utils
} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UTILS_ALGORITHM_HPP
