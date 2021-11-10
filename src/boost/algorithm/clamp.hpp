/* 
   Copyright (c) Marshall Clow 2008-2012.

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

 Revision history:
   27 June 2009 mtc First version
   23 Oct  2010 mtc Added predicate version
   
*/

/// \file clamp.hpp
/// \brief Clamp algorithm
/// \author Marshall Clow
///
/// Suggested by olafvdspek in https://svn.boost.org/trac/boost/ticket/3215

#ifndef BOOST_ALGORITHM_CLAMP_HPP
#define BOOST_ALGORITHM_CLAMP_HPP

#include <functional>       //  For std::less
#include <iterator>         //  For std::iterator_traits
#include <cassert>

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/mpl/identity.hpp>      // for identity
#include <boost/utility/enable_if.hpp> // for boost::disable_if

namespace boost { namespace algorithm {

/// \fn clamp ( T const& val, 
///               typename boost::mpl::identity<T>::type const & lo, 
///               typename boost::mpl::identity<T>::type const & hi, Pred p )
/// \return the value "val" brought into the range [ lo, hi ]
///     using the comparison predicate p.
///     If p ( val, lo ) return lo.
///     If p ( hi, val ) return hi.
///     Otherwise, return the original value.
/// 
/// \param val   The value to be clamped
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
/// \param p     A predicate to use to compare the values.
///                 p ( a, b ) returns a boolean.
///
  template<typename T, typename Pred> 
  BOOST_CXX14_CONSTEXPR T const & clamp ( T const& val, 
    typename boost::mpl::identity<T>::type const & lo, 
    typename boost::mpl::identity<T>::type const & hi, Pred p )
  {
//    assert ( !p ( hi, lo ));    // Can't assert p ( lo, hi ) b/c they might be equal
    return p ( val, lo ) ? lo : p ( hi, val ) ? hi : val;
  } 


/// \fn clamp ( T const& val, 
///               typename boost::mpl::identity<T>::type const & lo, 
///               typename boost::mpl::identity<T>::type const & hi )
/// \return the value "val" brought into the range [ lo, hi ].
///     If the value is less than lo, return lo.
///     If the value is greater than "hi", return hi.
///     Otherwise, return the original value.
///
/// \param val   The value to be clamped
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
///
  template<typename T> 
  BOOST_CXX14_CONSTEXPR T const& clamp ( const T& val, 
    typename boost::mpl::identity<T>::type const & lo, 
    typename boost::mpl::identity<T>::type const & hi )
  {
    return boost::algorithm::clamp ( val, lo, hi, std::less<T>());
  } 

/// \fn clamp_range ( InputIterator first, InputIterator last, OutputIterator out, 
///       std::iterator_traits<InputIterator>::value_type const & lo, 
///       std::iterator_traits<InputIterator>::value_type const & hi )
/// \return clamp the sequence of values [first, last) into [ lo, hi ]
/// 
/// \param first The start of the range of values
/// \param last  One past the end of the range of input values
/// \param out   An output iterator to write the clamped values into
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
///
  template<typename InputIterator, typename OutputIterator> 
  BOOST_CXX14_CONSTEXPR OutputIterator clamp_range ( InputIterator first, InputIterator last, OutputIterator out,
    typename std::iterator_traits<InputIterator>::value_type const & lo, 
    typename std::iterator_traits<InputIterator>::value_type const & hi )
  {
  // this could also be written with bind and std::transform
    while ( first != last )
        *out++ = boost::algorithm::clamp ( *first++, lo, hi );
    return out;
  } 

/// \fn clamp_range ( const Range &r, OutputIterator out, 
///       typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & lo,
///       typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & hi )
/// \return clamp the sequence of values [first, last) into [ lo, hi ]
/// 
/// \param r     The range of values to be clamped
/// \param out   An output iterator to write the clamped values into
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
///
  template<typename Range, typename OutputIterator> 
  BOOST_CXX14_CONSTEXPR typename boost::disable_if_c<boost::is_same<Range, OutputIterator>::value, OutputIterator>::type
  clamp_range ( const Range &r, OutputIterator out,
    typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & lo, 
    typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & hi )
  {
    return boost::algorithm::clamp_range ( boost::begin ( r ), boost::end ( r ), out, lo, hi );
  } 


/// \fn clamp_range ( InputIterator first, InputIterator last, OutputIterator out, 
///       std::iterator_traits<InputIterator>::value_type const & lo, 
///       std::iterator_traits<InputIterator>::value_type const & hi, Pred p )
/// \return clamp the sequence of values [first, last) into [ lo, hi ]
///     using the comparison predicate p.
/// 
/// \param first The start of the range of values
/// \param last  One past the end of the range of input values
/// \param out   An output iterator to write the clamped values into
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
/// \param p     A predicate to use to compare the values.
///                 p ( a, b ) returns a boolean.

///
  template<typename InputIterator, typename OutputIterator, typename Pred> 
  BOOST_CXX14_CONSTEXPR OutputIterator clamp_range ( InputIterator first, InputIterator last, OutputIterator out,
    typename std::iterator_traits<InputIterator>::value_type const & lo, 
    typename std::iterator_traits<InputIterator>::value_type const & hi, Pred p )
  {
  // this could also be written with bind and std::transform
    while ( first != last )
        *out++ = boost::algorithm::clamp ( *first++, lo, hi, p );
    return out;
  } 

/// \fn clamp_range ( const Range &r, OutputIterator out, 
///       typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & lo,
///       typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & hi,
///       Pred p )
/// \return clamp the sequence of values [first, last) into [ lo, hi ]
///     using the comparison predicate p.
/// 
/// \param r     The range of values to be clamped
/// \param out   An output iterator to write the clamped values into
/// \param lo    The lower bound of the range to be clamped to
/// \param hi    The upper bound of the range to be clamped to
/// \param p     A predicate to use to compare the values.
///                 p ( a, b ) returns a boolean.
//
//  Disable this template if the first two parameters are the same type;
//  In that case, the user will get the two iterator version.
  template<typename Range, typename OutputIterator, typename Pred> 
  BOOST_CXX14_CONSTEXPR typename boost::disable_if_c<boost::is_same<Range, OutputIterator>::value, OutputIterator>::type
  clamp_range ( const Range &r, OutputIterator out,
    typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & lo, 
    typename std::iterator_traits<typename boost::range_iterator<const Range>::type>::value_type const & hi,
    Pred p )
  {
    return boost::algorithm::clamp_range ( boost::begin ( r ), boost::end ( r ), out, lo, hi, p );
  } 


}}

#endif // BOOST_ALGORITHM_CLAMP_HPP
