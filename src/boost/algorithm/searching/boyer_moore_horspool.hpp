/* 
   Copyright (c) Marshall Clow 2010-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org
*/

#ifndef BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP
#define BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP

#include <iterator>     // for std::iterator_traits

#include <boost/config.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/algorithm/searching/detail/bm_traits.hpp>
#include <boost/algorithm/searching/detail/debugging.hpp>

// #define  BOOST_ALGORITHM_BOYER_MOORE_HORSPOOL_DEBUG_HPP

namespace boost { namespace algorithm {

/*
    A templated version of the boyer-moore-horspool searching algorithm.
    
    Requirements:
        * Random access iterators
        * The two iterator types (patIter and corpusIter) must 
            "point to" the same underlying type.
        * Additional requirements may be imposed buy the skip table, such as:
        ** Numeric type (array-based skip table)
        ** Hashable type (map-based skip table)

http://www-igm.univ-mlv.fr/%7Elecroq/string/node18.html

*/

    template <typename patIter, typename traits = detail::BM_traits<patIter> >
    class boyer_moore_horspool {
        typedef typename std::iterator_traits<patIter>::difference_type difference_type;
    public:
        boyer_moore_horspool ( patIter first, patIter last ) 
                : pat_first ( first ), pat_last ( last ),
                  k_pattern_length ( std::distance ( pat_first, pat_last )),
                  skip_ ( k_pattern_length, k_pattern_length ) {
                  
        //  Build the skip table
            std::size_t i = 0;
            if ( first != last )    // empty pattern?
                for ( patIter iter = first; iter != last-1; ++iter, ++i )
                    skip_.insert ( *iter, k_pattern_length - 1 - i );
#ifdef BOOST_ALGORITHM_BOYER_MOORE_HORSPOOL_DEBUG_HPP
            skip_.PrintSkipTable ();
#endif
            }
            
        ~boyer_moore_horspool () {}
        
        /// \fn operator ( corpusIter corpus_first, corpusIter corpus_last)
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        ///
        template <typename corpusIter>
        std::pair<corpusIter, corpusIter>
        operator () ( corpusIter corpus_first, corpusIter corpus_last ) const {
            BOOST_STATIC_ASSERT (( boost::is_same<
                typename std::iterator_traits<patIter>::value_type, 
                typename std::iterator_traits<corpusIter>::value_type>::value ));

            if ( corpus_first == corpus_last ) return std::make_pair(corpus_last, corpus_last);   // if nothing to search, we didn't find it!
            if (    pat_first ==    pat_last ) return std::make_pair(corpus_first, corpus_first); // empty pattern matches at start

            const difference_type k_corpus_length  = std::distance ( corpus_first, corpus_last );
        //  If the pattern is larger than the corpus, we can't find it!
            if ( k_corpus_length < k_pattern_length )
                return std::make_pair(corpus_last, corpus_last);
    
        //  Do the search 
            return this->do_search ( corpus_first, corpus_last );
            }
            
        template <typename Range>
        std::pair<typename boost::range_iterator<Range>::type, typename boost::range_iterator<Range>::type>
        operator () ( Range &r ) const {
            return (*this) (boost::begin(r), boost::end(r));
            }

    private:
/// \cond DOXYGEN_HIDE
        patIter pat_first, pat_last;
        const difference_type k_pattern_length;
        typename traits::skip_table_t skip_;

        /// \fn do_search ( corpusIter corpus_first, corpusIter corpus_last )
        /// \brief Searches the corpus for the pattern that was passed into the constructor
        /// 
        /// \param corpus_first The start of the data to search (Random Access Iterator)
        /// \param corpus_last  One past the end of the data to search
        /// \param k_corpus_length The length of the corpus to search
        ///
        template <typename corpusIter>
        std::pair<corpusIter, corpusIter>
        do_search ( corpusIter corpus_first, corpusIter corpus_last ) const {
            corpusIter curPos = corpus_first;
            const corpusIter lastPos = corpus_last - k_pattern_length;
            while ( curPos <= lastPos ) {
            //  Do we match right where we are?
                std::size_t j = k_pattern_length - 1;
                while ( pat_first [j] == curPos [j] ) {
                //  We matched - we're done!
                    if ( j == 0 )
                        return std::make_pair(curPos, curPos + k_pattern_length);
                    j--;
                    }
        
                curPos += skip_ [ curPos [ k_pattern_length - 1 ]];
                }
            
            return std::make_pair(corpus_last, corpus_last);
            }
// \endcond
        };

/*  Two ranges as inputs gives us four possibilities; with 2,3,3,4 parameters
    Use a bit of TMP to disambiguate the 3-argument templates */

/// \fn boyer_moore_horspool_search ( corpusIter corpus_first, corpusIter corpus_last, 
///       patIter pat_first, patIter pat_last )
/// \brief Searches the corpus for the pattern.
/// 
/// \param corpus_first The start of the data to search (Random Access Iterator)
/// \param corpus_last  One past the end of the data to search
/// \param pat_first    The start of the pattern to search for (Random Access Iterator)
/// \param pat_last     One past the end of the data to search for
///
    template <typename patIter, typename corpusIter>
    std::pair<corpusIter, corpusIter> boyer_moore_horspool_search ( 
                  corpusIter corpus_first, corpusIter corpus_last, 
                  patIter pat_first, patIter pat_last )
    {
        boyer_moore_horspool<patIter> bmh ( pat_first, pat_last );
        return bmh ( corpus_first, corpus_last );
    }

    template <typename PatternRange, typename corpusIter>
    std::pair<corpusIter, corpusIter> boyer_moore_horspool_search ( 
        corpusIter corpus_first, corpusIter corpus_last, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        boyer_moore_horspool<pattern_iterator> bmh ( boost::begin(pattern), boost::end (pattern));
        return bmh ( corpus_first, corpus_last );
    }
    
    template <typename patIter, typename CorpusRange>
    typename boost::disable_if_c<
        boost::is_same<CorpusRange, patIter>::value, 
        std::pair<typename boost::range_iterator<CorpusRange>::type, typename boost::range_iterator<CorpusRange>::type> >
    ::type
    boyer_moore_horspool_search ( CorpusRange &corpus, patIter pat_first, patIter pat_last )
    {
        boyer_moore_horspool<patIter> bmh ( pat_first, pat_last );
        return bm (boost::begin (corpus), boost::end (corpus));
    }
    
    template <typename PatternRange, typename CorpusRange>
    std::pair<typename boost::range_iterator<CorpusRange>::type, typename boost::range_iterator<CorpusRange>::type>
    boyer_moore_horspool_search ( CorpusRange &corpus, const PatternRange &pattern )
    {
        typedef typename boost::range_iterator<const PatternRange>::type pattern_iterator;
        boyer_moore_horspool<pattern_iterator> bmh ( boost::begin(pattern), boost::end (pattern));
        return bmh (boost::begin (corpus), boost::end (corpus));
    }


    //  Creator functions -- take a pattern range, return an object
    template <typename Range>
    boost::algorithm::boyer_moore_horspool<typename boost::range_iterator<const Range>::type>
    make_boyer_moore_horspool ( const Range &r ) {
        return boost::algorithm::boyer_moore_horspool
            <typename boost::range_iterator<const Range>::type> (boost::begin(r), boost::end(r));
        }
    
    template <typename Range>
    boost::algorithm::boyer_moore_horspool<typename boost::range_iterator<Range>::type>
    make_boyer_moore_horspool ( Range &r ) {
        return boost::algorithm::boyer_moore_horspool
            <typename boost::range_iterator<Range>::type> (boost::begin(r), boost::end(r));
        }

}}

#endif  //  BOOST_ALGORITHM_BOYER_MOORE_HORSPOOOL_SEARCH_HPP
