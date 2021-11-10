//  Copyright (c) 2010 Nuovation System Designs, LLC
//    Grant Erickson <gerickson@nuovations.com>
//
//  Reworked somewhat by Marshall Clow; August 2010
//  
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/ for latest version.
//

#ifndef BOOST_ALGORITHM_ORDERED_HPP
#define BOOST_ALGORITHM_ORDERED_HPP

#include <functional>
#include <iterator>

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/identity.hpp>

namespace boost { namespace algorithm {

/// \fn is_sorted_until ( ForwardIterator first, ForwardIterator last, Pred p )
/// \return the point in the sequence [first, last) where the elements are unordered
///     (according to the comparison predicate 'p').
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
/// \param p     A binary predicate that returns true if two elements are ordered.
///
    template <typename ForwardIterator, typename Pred>
    BOOST_CXX14_CONSTEXPR ForwardIterator is_sorted_until ( ForwardIterator first, ForwardIterator last, Pred p )
    {
        if ( first == last ) return last;  // the empty sequence is ordered
        ForwardIterator next = first;
        while ( ++next != last )
        {
            if ( p ( *next, *first ))
                return next;
            first = next;
        }
        return last;    
    }

/// \fn is_sorted_until ( ForwardIterator first, ForwardIterator last )
/// \return the point in the sequence [first, last) where the elements are unordered
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR ForwardIterator is_sorted_until ( ForwardIterator first, ForwardIterator last )
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        return boost::algorithm::is_sorted_until ( first, last, std::less<value_type>());
    }


/// \fn is_sorted ( ForwardIterator first, ForwardIterator last, Pred p )
/// \return whether or not the entire sequence is sorted
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
/// \param p     A binary predicate that returns true if two elements are ordered.
///
    template <typename ForwardIterator, typename Pred>
    BOOST_CXX14_CONSTEXPR bool is_sorted ( ForwardIterator first, ForwardIterator last, Pred p )
    {
        return boost::algorithm::is_sorted_until (first, last, p) == last;
    }

/// \fn is_sorted ( ForwardIterator first, ForwardIterator last )
/// \return whether or not the entire sequence is sorted
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR bool is_sorted ( ForwardIterator first, ForwardIterator last )
    {
        return boost::algorithm::is_sorted_until (first, last) == last;
    }

///
/// -- Range based versions of the C++11 functions
///

/// \fn is_sorted_until ( const R &range, Pred p )
/// \return the point in the range R where the elements are unordered
///     (according to the comparison predicate 'p').
/// 
/// \param range The range to be tested.
/// \param p     A binary predicate that returns true if two elements are ordered.
///
    template <typename R, typename Pred>
    BOOST_CXX14_CONSTEXPR typename boost::lazy_disable_if_c<
        boost::is_same<R, Pred>::value, 
        typename boost::range_iterator<const R> 
    >::type is_sorted_until ( const R &range, Pred p )
    {
        return boost::algorithm::is_sorted_until ( boost::begin ( range ), boost::end ( range ), p );
    }


/// \fn is_sorted_until ( const R &range )
/// \return the point in the range R where the elements are unordered
/// 
/// \param range The range to be tested.
///
    template <typename R>
    BOOST_CXX14_CONSTEXPR typename boost::range_iterator<const R>::type is_sorted_until ( const R &range )
    {
        return boost::algorithm::is_sorted_until ( boost::begin ( range ), boost::end ( range ));
    }

/// \fn is_sorted ( const R &range, Pred p )
/// \return whether or not the entire range R is sorted
///     (according to the comparison predicate 'p').
/// 
/// \param range The range to be tested.
/// \param p     A binary predicate that returns true if two elements are ordered.
///
    template <typename R, typename Pred>
    BOOST_CXX14_CONSTEXPR typename boost::lazy_disable_if_c< boost::is_same<R, Pred>::value, boost::mpl::identity<bool> >::type
    is_sorted ( const R &range, Pred p )
    {
        return boost::algorithm::is_sorted ( boost::begin ( range ), boost::end ( range ), p );
    }


/// \fn is_sorted ( const R &range )
/// \return whether or not the entire range R is sorted
/// 
/// \param range The range to be tested.
///
    template <typename R>
    BOOST_CXX14_CONSTEXPR bool is_sorted ( const R &range )
    {
        return boost::algorithm::is_sorted ( boost::begin ( range ), boost::end ( range ));
    }


///
/// -- Range based versions of the C++11 functions
///

/// \fn is_increasing ( ForwardIterator first, ForwardIterator last )
/// \return true if the entire sequence is increasing; i.e, each item is greater than or  
///     equal to the previous one.
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
/// \note This function will return true for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_strictly_increasing instead.
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR bool is_increasing ( ForwardIterator first, ForwardIterator last )
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        return boost::algorithm::is_sorted (first, last, std::less<value_type>());
    }


/// \fn is_increasing ( const R &range )
/// \return true if the entire sequence is increasing; i.e, each item is greater than or  
///     equal to the previous one.
/// 
/// \param range The range to be tested.
///
/// \note This function will return true for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_strictly_increasing instead.
    template <typename R>
    BOOST_CXX14_CONSTEXPR bool is_increasing ( const R &range )
    {
        return is_increasing ( boost::begin ( range ), boost::end ( range ));
    }



/// \fn is_decreasing ( ForwardIterator first, ForwardIterator last )
/// \return true if the entire sequence is decreasing; i.e, each item is less than 
///     or equal to the previous one.
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
/// \note This function will return true for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_strictly_decreasing instead.
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR bool is_decreasing ( ForwardIterator first, ForwardIterator last )
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        return boost::algorithm::is_sorted (first, last, std::greater<value_type>());
    }

/// \fn is_decreasing ( const R &range )
/// \return true if the entire sequence is decreasing; i.e, each item is less than 
///     or equal to the previous one.
/// 
/// \param range The range to be tested.
///
/// \note This function will return true for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_strictly_decreasing instead.
    template <typename R>
    BOOST_CXX14_CONSTEXPR bool is_decreasing ( const R &range )
    {
        return is_decreasing ( boost::begin ( range ), boost::end ( range ));
    }



/// \fn is_strictly_increasing ( ForwardIterator first, ForwardIterator last )
/// \return true if the entire sequence is strictly increasing; i.e, each item is greater
///     than the previous one
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
/// \note This function will return false for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_increasing instead.
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR bool is_strictly_increasing ( ForwardIterator first, ForwardIterator last )
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        return boost::algorithm::is_sorted (first, last, std::less_equal<value_type>());
    }

/// \fn is_strictly_increasing ( const R &range )
/// \return true if the entire sequence is strictly increasing; i.e, each item is greater
///     than the previous one
/// 
/// \param range The range to be tested.
///
/// \note This function will return false for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_increasing instead.
    template <typename R>
    BOOST_CXX14_CONSTEXPR bool is_strictly_increasing ( const R &range )
    {
        return is_strictly_increasing ( boost::begin ( range ), boost::end ( range ));
    }


/// \fn is_strictly_decreasing ( ForwardIterator first, ForwardIterator last )
/// \return true if the entire sequence is strictly decreasing; i.e, each item is less than
///     the previous one
/// 
/// \param first The start of the sequence to be tested.
/// \param last  One past the end of the sequence
///
/// \note This function will return false for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_decreasing instead.
    template <typename ForwardIterator>
    BOOST_CXX14_CONSTEXPR bool is_strictly_decreasing ( ForwardIterator first, ForwardIterator last )
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        return boost::algorithm::is_sorted (first, last, std::greater_equal<value_type>());
    }

/// \fn is_strictly_decreasing ( const R &range )
/// \return true if the entire sequence is strictly decreasing; i.e, each item is less than
///     the previous one
/// 
/// \param range The range to be tested.
///
/// \note This function will return false for sequences that contain items that compare
///     equal. If that is not what you intended, you should use is_decreasing instead.
    template <typename R>
    BOOST_CXX14_CONSTEXPR bool is_strictly_decreasing ( const R &range )
    {
        return is_strictly_decreasing ( boost::begin ( range ), boost::end ( range ));
    }

}} // namespace boost

#endif  // BOOST_ALGORITHM_ORDERED_HPP
