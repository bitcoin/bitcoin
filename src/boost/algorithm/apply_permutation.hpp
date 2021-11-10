/*
  Copyright (c) Alexander Zaitsev <zamazan4ik@gmail.com>, 2017

  Distributed under the Boost Software License, Version 1.0. (See
  accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt)

  See http://www.boost.org/ for latest version.


  Based on https://blogs.msdn.microsoft.com/oldnewthing/20170104-00/?p=95115
*/

/// \file  apply_permutation.hpp
/// \brief Apply permutation to a sequence.
/// \author Alexander Zaitsev

#ifndef BOOST_ALGORITHM_APPLY_PERMUTATION_HPP
#define BOOST_ALGORITHM_APPLY_PERMUTATION_HPP

#include <algorithm>

#include <boost/config.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost { namespace algorithm
{

/// \fn apply_permutation ( RandomAccessIterator1 item_begin, RandomAccessIterator1 item_end, RandomAccessIterator2 ind_begin )
/// \brief Reorder item sequence with index sequence order
///
/// \param item_begin    The start of the item sequence
/// \param item_end		 One past the end of the item sequence
/// \param ind_begin     The start of the index sequence.
///
/// \note Item sequence size should be equal to index size. Otherwise behavior is undefined.
///       Complexity: O(N).
template<typename RandomAccessIterator1, typename RandomAccessIterator2>
void
apply_permutation(RandomAccessIterator1 item_begin, RandomAccessIterator1 item_end,
                  RandomAccessIterator2 ind_begin, RandomAccessIterator2 ind_end)
{
    typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type Diff;
    typedef typename std::iterator_traits<RandomAccessIterator2>::difference_type Index;
    using std::swap;
    Diff size = std::distance(item_begin, item_end);
    for (Diff i = 0; i < size; i++)
    {
        Diff current = i;
        while (i != ind_begin[current])
        {
            Index next = ind_begin[current];
            swap(item_begin[current], item_begin[next]);
            ind_begin[current] = current;
            current = next;
        }
        ind_begin[current] = current;
    }
}

/// \fn apply_reverse_permutation ( RandomAccessIterator1 item_begin, RandomAccessIterator1 item_end, RandomAccessIterator2 ind_begin )
/// \brief Reorder item sequence with index sequence order
///
/// \param item_begin    The start of the item sequence
/// \param item_end		 One past the end of the item sequence
/// \param ind_begin     The start of the index sequence.
///
/// \note Item sequence size should be equal to index size. Otherwise behavior is undefined.
///       Complexity: O(N).
template<typename RandomAccessIterator1, typename RandomAccessIterator2>
void
apply_reverse_permutation(
        RandomAccessIterator1 item_begin,
        RandomAccessIterator1 item_end,
        RandomAccessIterator2 ind_begin,
        RandomAccessIterator2 ind_end)
{
    typedef typename std::iterator_traits<RandomAccessIterator2>::difference_type Diff;
    using std::swap;
    Diff length = std::distance(item_begin, item_end);
    for (Diff i = 0; i < length; i++)
    {
        while (i != ind_begin[i])
        {
            Diff next = ind_begin[i];
            swap(item_begin[i], item_begin[next]);
            swap(ind_begin[i], ind_begin[next]);
        }
    }
}

/// \fn apply_permutation ( Range1 item_range, Range2 ind_range )
/// \brief Reorder item sequence with index sequence order
///
/// \param item_range    The item sequence
/// \param ind_range     The index sequence
///
/// \note Item sequence size should be equal to index size. Otherwise behavior is undefined.
///       Complexity: O(N).
template<typename Range1, typename Range2>
void
apply_permutation(Range1& item_range, Range2& ind_range)
{
    apply_permutation(boost::begin(item_range), boost::end(item_range),
                      boost::begin(ind_range), boost::end(ind_range));
}

/// \fn apply_reverse_permutation ( Range1 item_range, Range2 ind_range )
/// \brief Reorder item sequence with index sequence order
///
/// \param item_range    The item sequence
/// \param ind_range     The index sequence
///
/// \note Item sequence size should be equal to index size. Otherwise behavior is undefined.
///       Complexity: O(N).
template<typename Range1, typename Range2>
void
apply_reverse_permutation(Range1& item_range, Range2& ind_range)
{
    apply_reverse_permutation(boost::begin(item_range), boost::end(item_range),
                              boost::begin(ind_range), boost::end(ind_range));
}

}}
#endif //BOOST_ALGORITHM_APPLY_PERMUTATION_HPP
