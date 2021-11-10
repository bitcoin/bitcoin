// Details for templated Spreadsort-based integer_sort.

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
*/

#ifndef BOOST_SORT_SPREADSORT_DETAIL_INTEGER_SORT_HPP
#define BOOST_SORT_SPREADSORT_DETAIL_INTEGER_SORT_HPP
#include <algorithm>
#include <vector>
#include <limits>
#include <functional>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/sort/spreadsort/detail/spreadsort_common.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace sort {
namespace spreadsort {
  namespace detail {
    // Return true if the list is sorted.  Otherwise, find the minimum and
    // maximum using <.
    template <class RandomAccessIter>
    inline bool
    is_sorted_or_find_extremes(RandomAccessIter current, RandomAccessIter last,
                               RandomAccessIter & max, RandomAccessIter & min)
    {
      min = max = current;
      //This assumes we have more than 1 element based on prior checks.
      while (!(*(current + 1) < *current)) {
        //If everything is in sorted order, return
        if (++current == last - 1)
          return true;
      }

      //The maximum is the last sorted element
      max = current;
      //Start from the first unsorted element
      while (++current < last) {
        if (*max < *current)
          max = current;
        else if (*current < *min)
          min = current;
      }
      return false;
    }

    // Return true if the list is sorted.  Otherwise, find the minimum and
    // maximum.
    // Use a user-defined comparison operator
    template <class RandomAccessIter, class Compare>
    inline bool
    is_sorted_or_find_extremes(RandomAccessIter current, RandomAccessIter last,
                RandomAccessIter & max, RandomAccessIter & min, Compare comp)
    {
      min = max = current;
      while (!comp(*(current + 1), *current)) {
        //If everything is in sorted order, return
        if (++current == last - 1)
          return true;
      }

      //The maximum is the last sorted element
      max = current;
      while (++current < last) {
        if (comp(*max, *current))
          max = current;
        else if (comp(*current, *min))
          min = current;
      }
      return false;
    }

    //Gets a non-negative right bit shift to operate as a logarithmic divisor
    template<unsigned log_mean_bin_size>
    inline int
    get_log_divisor(size_t count, int log_range)
    {
      int log_divisor;
      //If we can finish in one iteration without exceeding either
      //(2 to the max_finishing_splits) or n bins, do so
      if ((log_divisor = log_range - rough_log_2_size(count)) <= 0 && 
         log_range <= max_finishing_splits)
        log_divisor = 0; 
      else {
        //otherwise divide the data into an optimized number of pieces
        log_divisor += log_mean_bin_size;
        //Cannot exceed max_splits or cache misses slow down bin lookups
        if ((log_range - log_divisor) > max_splits)
          log_divisor = log_range - max_splits;
      }
      return log_divisor;
    }

    //Implementation for recursive integer sorting
    template <class RandomAccessIter, class Div_type, class Size_type>
    inline void
    spreadsort_rec(RandomAccessIter first, RandomAccessIter last,
              std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
              , size_t *bin_sizes)
    {
      //This step is roughly 10% of runtime, but it helps avoid worst-case
      //behavior and improve behavior with real data
      //If you know the maximum and minimum ahead of time, you can pass those
      //values in and skip this step for the first iteration
      RandomAccessIter max, min;
      if (is_sorted_or_find_extremes(first, last, max, min))
        return;
      RandomAccessIter * target_bin;
      unsigned log_divisor = get_log_divisor<int_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type((*max >> 0) - (*min >> 0))));
      Div_type div_min = *min >> log_divisor;
      Div_type div_max = *max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins =
        size_bins(bin_sizes, bin_cache, cache_offset, cache_end, bin_count);

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[size_t((*(current++) >> log_divisor) - div_min)]++;
      //Assign the bin positions
      bins[0] = first;
      for (unsigned u = 0; u < bin_count - 1; u++)
        bins[u + 1] = bins[u] + bin_sizes[u];

      RandomAccessIter nextbinstart = first;
      //Swap into place
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned u = 0; u < bin_count - 1; ++u) {
        RandomAccessIter * local_bin = bins + u;
        nextbinstart += bin_sizes[u];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < nextbinstart;
            ++current) {
          //Swapping elements in current into place until the correct
          //element has been swapped in
          for (target_bin = (bins + ((*current >> log_divisor) - div_min));
              target_bin != local_bin;
            target_bin = bins + ((*current >> log_divisor) - div_min)) {
            //3-way swap; this is about 1% faster than a 2-way swap
            //The main advantage is less copies are involved per item
            //put in the correct place
            typename std::iterator_traits<RandomAccessIter>::value_type tmp;
            RandomAccessIter b = (*target_bin)++;
            RandomAccessIter * b_bin = bins + ((*b >> log_divisor) - div_min);
            if (b_bin != local_bin) {
              RandomAccessIter c = (*b_bin)++;
              tmp = *c;
              *c = *b;
            }
            else
              tmp = *b;
            *b = *current;
            *current = tmp;
          }
        }
        *local_bin = nextbinstart;
      }
      bins[bin_count - 1] = last;

      //If we've bucketsorted, the array is sorted and we should skip recursion
      if (!log_divisor)
        return;
      //log_divisor is the remaining range; calculating the comparison threshold
      size_t max_count =
        get_min_count<int_log_mean_bin_size, int_log_min_split_count,
                      int_log_finishing_count>(log_divisor);

      //Recursing
      RandomAccessIter lastPos = first;
      for (unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u],
          ++u) {
        Size_type count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u]);
        else
          spreadsort_rec<RandomAccessIter, Div_type, Size_type>(lastPos,
                                                                 bin_cache[u],
                                                                 bin_cache,
                                                                 cache_end,
                                                                 bin_sizes);
      }
    }

    //Generic bitshift-based 3-way swapping code
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline void inner_swap_loop(RandomAccessIter * bins,
      const RandomAccessIter & next_bin_start, unsigned ii, Right_shift &rshift
      , const unsigned log_divisor, const Div_type div_min)
    {
      RandomAccessIter * local_bin = bins + ii;
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        for (RandomAccessIter * target_bin =
            (bins + (rshift(*current, log_divisor) - div_min));
            target_bin != local_bin;
            target_bin = bins + (rshift(*current, log_divisor) - div_min)) {
          typename std::iterator_traits<RandomAccessIter>::value_type tmp;
          RandomAccessIter b = (*target_bin)++;
          RandomAccessIter * b_bin =
            bins + (rshift(*b, log_divisor) - div_min);
          //Three-way swap; if the item to be swapped doesn't belong
          //in the current bin, swap it to where it belongs
          if (b_bin != local_bin) {
            RandomAccessIter c = (*b_bin)++;
            tmp = *c;
            *c = *b;
          }
          //Note: we could increment current once the swap is done in this case
          //but that seems to impair performance
          else
            tmp = *b;
          *b = *current;
          *current = tmp;
        }
      }
      *local_bin = next_bin_start;
    }

    //Standard swapping wrapper for ascending values
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline void swap_loop(RandomAccessIter * bins,
             RandomAccessIter & next_bin_start, unsigned ii, Right_shift &rshift
             , const size_t *bin_sizes
             , const unsigned log_divisor, const Div_type div_min)
    {
      next_bin_start += bin_sizes[ii];
      inner_swap_loop<RandomAccessIter, Div_type, Right_shift>(bins,
                              next_bin_start, ii, rshift, log_divisor, div_min);
    }

    //Functor implementation for recursive sorting
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare, class Size_type, unsigned log_mean_bin_size,
                unsigned log_min_split_count, unsigned log_finishing_count>
    inline void
    spreadsort_rec(RandomAccessIter first, RandomAccessIter last,
          std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
          , size_t *bin_sizes, Right_shift rshift, Compare comp)
    {
      RandomAccessIter max, min;
      if (is_sorted_or_find_extremes(first, last, max, min, comp))
        return;
      unsigned log_divisor = get_log_divisor<log_mean_bin_size>(last - first,
            rough_log_2_size(Size_type(rshift(*max, 0) - rshift(*min, 0))));
      Div_type div_min = rshift(*min, log_divisor);
      Div_type div_max = rshift(*max, log_divisor);
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      bins[0] = first;
      for (unsigned u = 0; u < bin_count - 1; u++)
        bins[u + 1] = bins[u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = first;
      for (unsigned u = 0; u < bin_count - 1; ++u)
        swap_loop<RandomAccessIter, Div_type, Right_shift>(bins, next_bin_start,
                                  u, rshift, bin_sizes, log_divisor, div_min);
      bins[bin_count - 1] = last;

      //If we've bucketsorted, the array is sorted
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<log_mean_bin_size, log_min_split_count,
                          log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u],
          ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u], comp);
        else
          spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
        Size_type, log_mean_bin_size, log_min_split_count, log_finishing_count>
      (lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, rshift, comp);
      }
    }

    //Functor implementation for recursive sorting with only Shift overridden
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Size_type, unsigned log_mean_bin_size,
              unsigned log_min_split_count, unsigned log_finishing_count>
    inline void
    spreadsort_rec(RandomAccessIter first, RandomAccessIter last,
              std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
              , size_t *bin_sizes, Right_shift rshift)
    {
      RandomAccessIter max, min;
      if (is_sorted_or_find_extremes(first, last, max, min))
        return;
      unsigned log_divisor = get_log_divisor<log_mean_bin_size>(last - first,
            rough_log_2_size(Size_type(rshift(*max, 0) - rshift(*min, 0))));
      Div_type div_min = rshift(*min, log_divisor);
      Div_type div_max = rshift(*max, log_divisor);
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      bins[0] = first;
      for (unsigned u = 0; u < bin_count - 1; u++)
        bins[u + 1] = bins[u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter nextbinstart = first;
      for (unsigned ii = 0; ii < bin_count - 1; ++ii)
        swap_loop<RandomAccessIter, Div_type, Right_shift>(bins, nextbinstart,
                                ii, rshift, bin_sizes, log_divisor, div_min);
      bins[bin_count - 1] = last;

      //If we've bucketsorted, the array is sorted
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<log_mean_bin_size, log_min_split_count,
                          log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u],
          ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u]);
        else
          spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Size_type,
          log_mean_bin_size, log_min_split_count, log_finishing_count>(lastPos,
                      bin_cache[u], bin_cache, cache_end, bin_sizes, rshift);
      }
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Div_type>
    //Only use spreadsort if the integer can fit in a size_t
    inline typename boost::enable_if_c< sizeof(Div_type) <= sizeof(size_t),
                                                            void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, size_t>(first, last,
          bin_cache, 0, bin_sizes);
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Div_type>
    //Only use spreadsort if the integer can fit in a uintmax_t
    inline typename boost::enable_if_c< (sizeof(Div_type) > sizeof(size_t))
      && sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, boost::uintmax_t>(first,
          last, bin_cache, 0, bin_sizes);
    }

    template <class RandomAccessIter, class Div_type>
    inline typename boost::disable_if_c< sizeof(Div_type) <= sizeof(size_t)
      || sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    //defaulting to boost::sort::pdqsort when integer_sort won't work
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type)
    {
      //Warning that we're using boost::sort::pdqsort, even though integer_sort was called
      BOOST_STATIC_ASSERT( sizeof(Div_type) <= sizeof(size_t) );
      boost::sort::pdqsort(first, last);
    }


    //Same for the full functor version
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    //Only use spreadsort if the integer can fit in a size_t
    inline typename boost::enable_if_c< sizeof(Div_type) <= sizeof(size_t),
                                 void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift, Compare comp)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
          size_t, int_log_mean_bin_size, int_log_min_split_count, 
                        int_log_finishing_count>
          (first, last, bin_cache, 0, bin_sizes, shift, comp);
    }

    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    //Only use spreadsort if the integer can fit in a uintmax_t
    inline typename boost::enable_if_c< (sizeof(Div_type) > sizeof(size_t))
      && sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift, Compare comp)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
                        boost::uintmax_t, int_log_mean_bin_size,
                        int_log_min_split_count, int_log_finishing_count>
          (first, last, bin_cache, 0, bin_sizes, shift, comp);
    }

    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    inline typename boost::disable_if_c< sizeof(Div_type) <= sizeof(size_t)
      || sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    //defaulting to boost::sort::pdqsort when integer_sort won't work
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift, Compare comp)
    {
      //Warning that we're using boost::sort::pdqsort, even though integer_sort was called
      BOOST_STATIC_ASSERT( sizeof(Div_type) <= sizeof(size_t) );
      boost::sort::pdqsort(first, last, comp);
    }


    //Same for the right shift version
    template <class RandomAccessIter, class Div_type, class Right_shift>
    //Only use spreadsort if the integer can fit in a size_t
    inline typename boost::enable_if_c< sizeof(Div_type) <= sizeof(size_t),
                                 void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, Right_shift, size_t,
          int_log_mean_bin_size, int_log_min_split_count, 
                        int_log_finishing_count>
          (first, last, bin_cache, 0, bin_sizes, shift);
    }

    template <class RandomAccessIter, class Div_type, class Right_shift>
    //Only use spreadsort if the integer can fit in a uintmax_t
    inline typename boost::enable_if_c< (sizeof(Div_type) > sizeof(size_t))
      && sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      spreadsort_rec<RandomAccessIter, Div_type, Right_shift,
                        boost::uintmax_t, int_log_mean_bin_size,
                        int_log_min_split_count, int_log_finishing_count>
          (first, last, bin_cache, 0, bin_sizes, shift);
    }

    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline typename boost::disable_if_c< sizeof(Div_type) <= sizeof(size_t)
      || sizeof(Div_type) <= sizeof(boost::uintmax_t), void >::type
    //defaulting to boost::sort::pdqsort when integer_sort won't work
    integer_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
                Right_shift shift)
    {
      //Warning that we're using boost::sort::pdqsort, even though integer_sort was called
      BOOST_STATIC_ASSERT( sizeof(Div_type) <= sizeof(size_t) );
      boost::sort::pdqsort(first, last);
    }
  }
}
}
}

#endif
