//  Boost string_algo library util.hpp header file  ---------------------------//

//  Copyright Pavol Droba 2002-2003.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/ for updates, documentation, and revision history.

#ifndef BOOST_STRING_UTIL_DETAIL_HPP
#define BOOST_STRING_UTIL_DETAIL_HPP

#include <boost/algorithm/string/config.hpp>
#include <functional>
#include <boost/range/iterator_range_core.hpp>

namespace boost {
    namespace algorithm {
        namespace detail {

//  empty container  -----------------------------------------------//

            //  empty_container 
            /*
                This class represents always empty container,
                containing elements of type CharT.

                It is supposed to be used in a const version only
            */
            template< typename CharT >
            struct empty_container 
            {
                typedef empty_container<CharT> type;        
                typedef CharT value_type;
                typedef std::size_t size_type;
                typedef std::ptrdiff_t difference_type;
                typedef const value_type& reference;
                typedef const value_type& const_reference;
                typedef const value_type* iterator;
                typedef const value_type* const_iterator;

                
                // Operations
                const_iterator begin() const
                {
                    return reinterpret_cast<const_iterator>(0);
                }

                const_iterator end() const
                {
                    return reinterpret_cast<const_iterator>(0);
                }

                bool empty() const
                {
                    return false;
                }

                size_type size() const
                {
                    return 0;
                }
            };
    
//  bounded copy algorithm  -----------------------------------------------//

            // Bounded version of the std::copy algorithm
            template<typename InputIteratorT, typename OutputIteratorT>
            inline OutputIteratorT bounded_copy(
                InputIteratorT First, 
                InputIteratorT Last, 
                OutputIteratorT DestFirst,
                OutputIteratorT DestLast )
            {
                InputIteratorT InputIt=First;
                OutputIteratorT OutputIt=DestFirst;
                for(; InputIt!=Last && OutputIt!=DestLast; InputIt++, OutputIt++ )
                {
                    *OutputIt=*InputIt;
                }

                return OutputIt;
            }

//  iterator range utilities -----------------------------------------//

            // copy range functor
            template< 
                typename SeqT, 
                typename IteratorT=BOOST_STRING_TYPENAME SeqT::const_iterator >
            struct copy_iterator_rangeF
            {
                typedef iterator_range<IteratorT> argument_type;
                typedef SeqT result_type;
                SeqT operator()( const iterator_range<IteratorT>& Range ) const
                {
                    return copy_range<SeqT>(Range);
                }
            };

        } // namespace detail
    } // namespace algorithm
} // namespace boost


#endif  // BOOST_STRING_UTIL_DETAIL_HPP
