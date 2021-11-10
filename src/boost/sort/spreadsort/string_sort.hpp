//Templated hybrid string_sort

//          Copyright Steven J. Ross 2001 - 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort/ for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
*/

#ifndef BOOST_STRING_SORT_HPP
#define BOOST_STRING_SORT_HPP
#include <algorithm>
#include <vector>
#include <cstring>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/sort/spreadsort/detail/string_sort.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
namespace sort {
namespace spreadsort {

/*! \brief String sort algorithm using random access iterators, allowing character-type overloads.\n
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \tparam RandomAccessIter <a href="http://www.cplusplus.com/reference/iterator/RandomAccessIterator/">Random access iterator</a>
   \tparam Unsigned_char_type  Unsigned character type used for string.
   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] unused value with the same type as the result of the [] operator, defining the Unsigned_char_type.  The actual value is unused.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/

  template <class RandomAccessIter, class Unsigned_char_type>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last,
                          Unsigned_char_type unused)
  {
    //Don't sort if it's too small to optimize
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else
      detail::string_sort(first, last, unused);
  }

/*! \brief String sort algorithm using range, allowing character-type overloads.\n
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \tparam Unsigned_char_type  Unsigned character type used for string.
   \param[in] range Range [first, last) for sorting.
   \param[in] unused value with the same type as the result of the [] operator, defining the Unsigned_char_type.  The actual value is unused.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/

template <class Range, class Unsigned_char_type>
inline void string_sort(Range& range, Unsigned_char_type unused)
{
  string_sort(boost::begin(range), boost::end(range), unused);
}

/*! \brief String sort algorithm using random access iterators, wraps using default of unsigned char.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>
   \n
   <a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last)
  {
    unsigned char unused = '\0';
    string_sort(first, last, unused);
  }

/*! \brief String sort algorithm using range, wraps using default of unsigned char.
  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>
   \n
   <a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] range Range [first, last) for sorting.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range>
inline void string_sort(Range& range)
{
  string_sort(boost::begin(range), boost::end(range));
}

/*! \brief String sort algorithm using random access iterators, allowing character-type overloads.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>


   \tparam RandomAccessIter <a href="http://www.cplusplus.com/reference/iterator/RandomAccessIterator/">Random access iterator</a>
   \tparam Comp Functor type to use for comparison.
   \tparam Unsigned_char_type Unsigned character type used for string.

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.
   \param[in] unused value with the same type as the result of the [] operator, defining the Unsigned_char_type.  The actual value is unused.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
  template <class RandomAccessIter, class Compare, class Unsigned_char_type>
  inline void reverse_string_sort(RandomAccessIter first,
                RandomAccessIter last, Compare comp, Unsigned_char_type unused)
  {
    //Don't sort if it's too small to optimize.
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last, comp);
    else
      detail::reverse_string_sort(first, last, unused);
  }

/*! \brief String sort algorithm using range, allowing character-type overloads.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_integer_sort.htm"> windows_integer_sort</a>
   \n
   <a href="../../doc/graph/osx_integer_sort.htm"> osx_integer_sort</a>


   \tparam Comp Functor type to use for comparison.
   \tparam Unsigned_char_type Unsigned character type used for string.

   \param[in] range Range [first, last) for sorting.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.
   \param[in] unused value with the same type as the result of the [] operator, defining the Unsigned_char_type.  The actual value is unused.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
template <class Range, class Compare, class Unsigned_char_type>
inline void reverse_string_sort(Range& range, Compare comp, Unsigned_char_type unused)
{
  reverse_string_sort(boost::begin(range), boost::end(range), comp, unused);
}

/*! \brief String sort algorithm using random access iterators,  wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms.\n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
  template <class RandomAccessIter, class Compare>
  inline void reverse_string_sort(RandomAccessIter first,
                                  RandomAccessIter last, Compare comp)
  {
    unsigned char unused = '\0';
    reverse_string_sort(first, last, comp, unused);
  }

/*! \brief String sort algorithm using range, wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] range Range [first, last) for sorting.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).
*/
template <class Range, class Compare>
inline void reverse_string_sort(Range& range, Compare comp)
{
  reverse_string_sort(boost::begin(range), boost::end(range), comp);
}

/*! \brief String sort algorithm using random access iterators,  wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.

   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \pre @c RandomAccessIter @c value_type supports the @c operator>>,
   which returns an integer-type right-shifted a specified number of bits.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter, class Get_char, class Get_length>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last,
                          Get_char get_character, Get_length length)
  {
    //Don't sort if it's too small to optimize
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else {
      //skipping past empties, which allows us to get the character type
      //.empty() is not used so as not to require a user declaration of it
      while (!length(*first)) {
        if (++first == last)
          return;
      }
      detail::string_sort(first, last, get_character, length, get_character((*first), 0));
    }
  }

/*! \brief String sort algorithm using range, wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>

   \param[in] range Range [first, last) for sorting.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.

   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws  std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range, class Get_char, class Get_length>
inline void string_sort(Range& range, Get_char get_character, Get_length length)
{
  string_sort(boost::begin(range), boost::end(range), get_character, length);
}


/*! \brief String sort algorithm using random access iterators,  wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>


   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.


   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter, class Get_char, class Get_length,
            class Compare>
  inline void string_sort(RandomAccessIter first, RandomAccessIter last,
                          Get_char get_character, Get_length length, Compare comp)
  {
    //Don't sort if it's too small to optimize
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last, comp);
    else {
      //skipping past empties, which allows us to get the character type
      //.empty() is not used so as not to require a user declaration of it
      while (!length(*first)) {
        if (++first == last)
          return;
      }
      detail::string_sort(first, last, get_character, length, comp,
                          get_character((*first), 0));
    }
  }

/*! \brief String sort algorithm using range, wraps using default of @c unsigned char.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

  \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>


   \param[in] range Range [first, last) for sorting.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.


   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range, class Get_char, class Get_length, class Compare>
inline void string_sort(Range& range,
                        Get_char get_character, Get_length length, Compare comp)
{
  string_sort(boost::begin(range), boost::end(range), get_character, length, comp);
}

/*! \brief Reverse String sort algorithm using random access iterators.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

 \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>


   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.


   \pre [@c first, @c last) is a valid range.
   \pre @c RandomAccessIter @c value_type is mutable.
   \pre @c RandomAccessIter @c value_type is <a href="http://en.cppreference.com/w/cpp/concept/LessThanComparable">LessThanComparable</a>
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
  template <class RandomAccessIter, class Get_char, class Get_length,
            class Compare>
  inline void reverse_string_sort(RandomAccessIter first,
    RandomAccessIter last, Get_char get_character, Get_length length, Compare comp)
  {
    //Don't sort if it's too small to optimize
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last, comp);
    else {
      //skipping past empties, which allows us to get the character type
      //.empty() is not used so as not to require a user declaration of it
      while (!length(*(--last))) {
        //If there is just one non-empty at the beginning, this is sorted
        if (first == last)
          return;
      }
      //making last just after the end of the non-empty part of the array
      detail::reverse_string_sort(first, last + 1, get_character, length, comp,
                                  get_character((*last), 0));
    }
  }

/*! \brief Reverse String sort algorithm using range.

  (All variants fall back to @c boost::sort::pdqsort if the data size is too small, < @c detail::min_sort_size).

 \details @c string_sort is a fast templated in-place hybrid radix/comparison algorithm,
which in testing tends to be roughly 50% to 2X faster than @c std::sort for large tests (>=100kB).\n
\par
Worst-case performance is <em>  O(N * (lg(range)/s + s)) </em>,
so @c string_sort is asymptotically faster
than pure comparison-based algorithms. \n\n
Some performance plots of runtime vs. n and log(range) are provided:\n
<a href="../../doc/graph/windows_string_sort.htm"> windows_string_sort</a>\n
<a href="../../doc/graph/osx_string_sort.htm"> osx_string_sort</a>


   \param[in] range Range [first, last) for sorting.
   \param[in] get_character Bracket functor equivalent to @c operator[], taking a number corresponding to the character offset.
   \param[in] length Functor to get the length of the string in characters.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.


   \pre [@c first, @c last) is a valid range.
   \post The elements in the range [@c first, @c last) are sorted in ascending order.

   \return @c void.

   \throws std::exception Propagates exceptions if any of the element comparisons, the element swaps (or moves),
   the right shift, subtraction of right-shifted elements, functors,
   or any operations on iterators throw.

   \warning Throwing an exception may cause data loss. This will also throw if a small vector resize throws, in which case there will be no data loss.
   \warning Invalid arguments cause undefined behaviour.
   \note @c spreadsort function provides a wrapper that calls the fastest sorting algorithm available for a data type,
   enabling faster generic-programming.

   \remark The lesser of <em> O(N*log(N)) </em> comparisons and <em> O(N*log(K/S + S)) </em>operations worst-case, where:
   \remark  *  N is @c last - @c first,
   \remark  *  K is the log of the range in bits (32 for 32-bit integers using their full range),
   \remark  *  S is a constant called max_splits, defaulting to 11 (except for strings where it is the log of the character size).

*/
template <class Range, class Get_char, class Get_length,
        class Compare>
inline void reverse_string_sort(Range& range, Get_char get_character, Get_length length, Compare comp)
{
    reverse_string_sort(boost::begin(range), boost::end(range), get_character, length, comp);
}
}
}
}

#endif
