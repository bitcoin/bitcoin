//  Boost string_algo library find_regex.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_FINDER_REGEX_DETAIL_HPP
#define BOOST_STRING_FINDER_REGEX_DETAIL_HPP

#include <boost/algorithm/string/config.hpp>
#include <boost/regex.hpp>

#include <boost/range/iterator_range_core.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
    namespace algorithm {
        namespace detail {

//  regex find functor -----------------------------------------------//

            // regex search result
            template<typename IteratorT>
            struct regex_search_result : 
                public iterator_range<IteratorT>
            {
                typedef regex_search_result<IteratorT> type;
                typedef iterator_range<IteratorT> base_type;
                typedef BOOST_STRING_TYPENAME base_type::value_type value_type;
                typedef BOOST_STRING_TYPENAME base_type::difference_type difference_type;
                typedef BOOST_STRING_TYPENAME base_type::const_iterator const_iterator;
                typedef BOOST_STRING_TYPENAME base_type::iterator iterator;
                typedef boost::match_results<iterator> match_results_type;

                // Construction

                // Construction from the match result
                regex_search_result( const match_results_type& MatchResults ) :
                    base_type( MatchResults[0].first, MatchResults[0].second ),
                    m_MatchResults( MatchResults ) {}
                
                // Construction of empty match. End iterator has to be specified
                regex_search_result( IteratorT End ) :
                    base_type( End, End ) {}

                regex_search_result( const regex_search_result& Other ) :
                    base_type( Other.begin(), Other.end() ),
                    m_MatchResults( Other.m_MatchResults ) {}

                // Assignment
                regex_search_result& operator=( const regex_search_result& Other )
                {
                    base_type::operator=( Other );
                    m_MatchResults=Other.m_MatchResults;
                    return *this;
                }

                // Match result retrieval
                const match_results_type& match_results() const
                {
                    return m_MatchResults;
                }

            private:
                // Saved match result
                match_results_type m_MatchResults;
            };

            // find_regex
            /*
                Regex based search functor
            */
            template<typename RegExT>
            struct find_regexF
            {
                typedef RegExT regex_type;
                typedef const RegExT& regex_reference_type;
                    
                // Construction
                find_regexF( regex_reference_type Rx, match_flag_type MatchFlags = match_default ) : 
                    m_Rx(Rx), m_MatchFlags(MatchFlags) {}   

                // Operation
                template< typename ForwardIteratorT >
                regex_search_result<ForwardIteratorT>
                operator()( 
                    ForwardIteratorT Begin, 
                    ForwardIteratorT End ) const
                {
                    typedef ForwardIteratorT input_iterator_type;
                    typedef regex_search_result<ForwardIteratorT> result_type;

                    // instantiate match result
                    match_results<input_iterator_type> result;
                    // search for a match
                    if ( ::boost::regex_search( Begin, End, result, m_Rx, m_MatchFlags ) )
                    {
                        // construct a result
                        return result_type( result );
                    }
                    else
                    {
                        // empty result
                        return result_type( End );
                    }
                }

            private:
                regex_reference_type m_Rx; // Regexp
                match_flag_type m_MatchFlags;     // match flags
            };

        } // namespace detail
    } // namespace algorithm
} // namespace boost

#endif  // BOOST_STRING_FIND_DETAIL_HPP
