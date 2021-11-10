// Details for templated Spreadsort-based float_sort.

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
float_mem_cast fix provided by:
Scott McMurray
*/

#ifndef BOOST_SORT_SPREADSORT_DETAIL_FLOAT_SORT_HPP
#define BOOST_SORT_SPREADSORT_DETAIL_FLOAT_SORT_HPP
#include <algorithm>
#include <vector>
#include <limits>
#include <functional>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/sort/spreadsort/detail/integer_sort.hpp>
#include <boost/sort/spreadsort/detail/spreadsort_common.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace sort {
namespace spreadsort {
  namespace detail {
    //Casts a RandomAccessIter to the specified integer type
    template<class Cast_type, class RandomAccessIter>
    inline Cast_type
    cast_float_iter(const RandomAccessIter & floatiter)
    {
      typedef typename std::iterator_traits<RandomAccessIter>::value_type
        Data_type;
      //Only cast IEEE floating-point numbers, and only to same-sized integers
      BOOST_STATIC_ASSERT(sizeof(Cast_type) == sizeof(Data_type));
      BOOST_STATIC_ASSERT(std::numeric_limits<Data_type>::is_iec559);
      BOOST_STATIC_ASSERT(std::numeric_limits<Cast_type>::is_integer);
      Cast_type result;
      std::memcpy(&result, &(*floatiter), sizeof(Data_type));
      return result;
    }

    // Return true if the list is sorted.  Otherwise, find the minimum and
    // maximum.  Values are Right_shifted 0 bits before comparison.
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline bool
    is_sorted_or_find_extremes(RandomAccessIter current, RandomAccessIter last,
                  Div_type & max, Div_type & min, Right_shift rshift)
    {
      min = max = rshift(*current, 0);
      RandomAccessIter prev = current;
      bool sorted = true;
      while (++current < last) {
        Div_type value = rshift(*current, 0);
        sorted &= *current >= *prev;
        prev = current;
        if (max < value)
          max = value;
        else if (value < min)
          min = value;
      }
      return sorted;
    }

    // Return true if the list is sorted.  Otherwise, find the minimum and
    // maximum.  Uses comp to check if the data is already sorted.
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    inline bool
    is_sorted_or_find_extremes(RandomAccessIter current, RandomAccessIter last,
                               Div_type & max, Div_type & min, 
                               Right_shift rshift, Compare comp)
    {
      min = max = rshift(*current, 0);
      RandomAccessIter prev = current;
      bool sorted = true;
      while (++current < last) {
        Div_type value = rshift(*current, 0);
        sorted &= !comp(*current, *prev);
        prev = current;
        if (max < value)
          max = value;
        else if (value < min)
          min = value;
      }
      return sorted;
    }

    //Specialized swap loops for floating-point casting
    template <class RandomAccessIter, class Div_type>
    inline void inner_float_swap_loop(RandomAccessIter * bins,
                        const RandomAccessIter & nextbinstart, unsigned ii
                        , const unsigned log_divisor, const Div_type div_min)
    {
      RandomAccessIter * local_bin = bins + ii;
      for (RandomAccessIter current = *local_bin; current < nextbinstart;
          ++current) {
        for (RandomAccessIter * target_bin =
            (bins + ((cast_float_iter<Div_type, RandomAccessIter>(current) >>
                      log_divisor) - div_min));  target_bin != local_bin;
          target_bin = bins + ((cast_float_iter<Div_type, RandomAccessIter>
                               (current) >> log_divisor) - div_min)) {
          typename std::iterator_traits<RandomAccessIter>::value_type tmp;
          RandomAccessIter b = (*target_bin)++;
          RandomAccessIter * b_bin = bins + ((cast_float_iter<Div_type,
                              RandomAccessIter>(b) >> log_divisor) - div_min);
          //Three-way swap; if the item to be swapped doesn't belong in the
          //current bin, swap it to where it belongs
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

    template <class RandomAccessIter, class Div_type>
    inline void float_swap_loop(RandomAccessIter * bins,
                          RandomAccessIter & nextbinstart, unsigned ii,
                          const size_t *bin_sizes,
                          const unsigned log_divisor, const Div_type div_min)
    {
      nextbinstart += bin_sizes[ii];
      inner_float_swap_loop<RandomAccessIter, Div_type>
        (bins, nextbinstart, ii, log_divisor, div_min);
    }

    // Return true if the list is sorted.  Otherwise, find the minimum and
    // maximum.  Values are cast to Cast_type before comparison.
    template <class RandomAccessIter, class Cast_type>
    inline bool
    is_sorted_or_find_extremes(RandomAccessIter current, RandomAccessIter last,
                  Cast_type & max, Cast_type & min)
    {
      min = max = cast_float_iter<Cast_type, RandomAccessIter>(current);
      RandomAccessIter prev = current;
      bool sorted = true;
      while (++current < last) {
        Cast_type value = cast_float_iter<Cast_type, RandomAccessIter>(current);
        sorted &= *current >= *prev;
        prev = current;
        if (max < value)
          max = value;
        else if (value < min)
          min = value;
      }
      return sorted;
    }

    //Special-case sorting of positive floats with casting
    template <class RandomAccessIter, class Div_type, class Size_type>
    inline void
    positive_float_sort_rec(RandomAccessIter first, RandomAccessIter last,
              std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
              , size_t *bin_sizes)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes<RandomAccessIter, Div_type>(first, last, 
                                                                max, min))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned((cast_float_iter<Div_type, RandomAccessIter>(
            current++) >> log_divisor) - div_min)]++;
      bins[0] = first;
      for (unsigned u = 0; u < bin_count - 1; u++)
        bins[u + 1] = bins[u] + bin_sizes[u];


      //Swap into place
      RandomAccessIter nextbinstart = first;
      for (unsigned u = 0; u < bin_count - 1; ++u)
        float_swap_loop<RandomAccessIter, Div_type>
          (bins, nextbinstart, u, bin_sizes, log_divisor, div_min);
      bins[bin_count - 1] = last;

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (unsigned u = cache_offset; u < cache_end; lastPos = bin_cache[u],
          ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u]);
        else
          positive_float_sort_rec<RandomAccessIter, Div_type, Size_type>
            (lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes);
      }
    }

    //Sorting negative floats
    //Bins are iterated in reverse because max_neg_float = min_neg_int
    template <class RandomAccessIter, class Div_type, class Size_type>
    inline void
    negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last,
                        std::vector<RandomAccessIter> &bin_cache,
                        unsigned cache_offset, size_t *bin_sizes)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes<RandomAccessIter, Div_type>(first, last, 
                                                                 max, min))
        return;

      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned((cast_float_iter<Div_type, RandomAccessIter>(
            current++) >> log_divisor) - div_min)]++;
      bins[bin_count - 1] = first;
      for (int ii = bin_count - 2; ii >= 0; --ii)
        bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];

      //Swap into place
      RandomAccessIter nextbinstart = first;
      //The last bin will always have the correct elements in it
      for (int ii = bin_count - 1; ii > 0; --ii)
        float_swap_loop<RandomAccessIter, Div_type>
          (bins, nextbinstart, ii, bin_sizes, log_divisor, div_min);
      //Update the end position because we don't process the last bin
      bin_cache[cache_offset] = last;

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_end - 1; ii >= static_cast<int>(cache_offset);
          lastPos = bin_cache[ii], --ii) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii]);
        else
          negative_float_sort_rec<RandomAccessIter, Div_type, Size_type>
            (lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes);
      }
    }

    //Sorting negative floats
    //Bins are iterated in reverse order because max_neg_float = min_neg_int
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Size_type>
    inline void
    negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last,
              std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
              , size_t *bin_sizes, Right_shift rshift)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes(first, last, max, min, rshift))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      bins[bin_count - 1] = first;
      for (int ii = bin_count - 2; ii >= 0; --ii)
        bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];

      //Swap into place
      RandomAccessIter nextbinstart = first;
      //The last bin will always have the correct elements in it
      for (int ii = bin_count - 1; ii > 0; --ii)
        swap_loop<RandomAccessIter, Div_type, Right_shift>
          (bins, nextbinstart, ii, rshift, bin_sizes, log_divisor, div_min);
      //Update the end position of the unprocessed last bin
      bin_cache[cache_offset] = last;

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_end - 1; ii >= static_cast<int>(cache_offset);
          lastPos = bin_cache[ii], --ii) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii]);
        else
          negative_float_sort_rec<RandomAccessIter, Div_type, Right_shift,
                                  Size_type>
            (lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes, rshift);
      }
    }

    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare, class Size_type>
    inline void
    negative_float_sort_rec(RandomAccessIter first, RandomAccessIter last,
            std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset,
            size_t *bin_sizes, Right_shift rshift, Compare comp)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes(first, last, max, min, rshift, comp))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      bins[bin_count - 1] = first;
      for (int ii = bin_count - 2; ii >= 0; --ii)
        bins[ii] = bins[ii + 1] + bin_sizes[ii + 1];

      //Swap into place
      RandomAccessIter nextbinstart = first;
      //The last bin will always have the correct elements in it
      for (int ii = bin_count - 1; ii > 0; --ii)
        swap_loop<RandomAccessIter, Div_type, Right_shift>
          (bins, nextbinstart, ii, rshift, bin_sizes, log_divisor, div_min);
      //Update the end position of the unprocessed last bin
      bin_cache[cache_offset] = last;

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Recursing
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_end - 1; ii >= static_cast<int>(cache_offset);
          lastPos = bin_cache[ii], --ii) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii], comp);
        else
          negative_float_sort_rec<RandomAccessIter, Div_type, Right_shift,
                                  Compare, Size_type>(lastPos, bin_cache[ii],
                                                      bin_cache, cache_end,
                                                      bin_sizes, rshift, comp);
      }
    }

    //Casting special-case for floating-point sorting
    template <class RandomAccessIter, class Div_type, class Size_type>
    inline void
    float_sort_rec(RandomAccessIter first, RandomAccessIter last,
                std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
                , size_t *bin_sizes)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes<RandomAccessIter, Div_type>(first, last, 
                                                                max, min))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned((cast_float_iter<Div_type, RandomAccessIter>(
            current++) >> log_divisor) - div_min)]++;
      //The index of the first positive bin
      //Must be divided small enough to fit into an integer
      unsigned first_positive = (div_min < 0) ? unsigned(-div_min) : 0;
      //Resetting if all bins are negative
      if (cache_offset + first_positive > cache_end)
        first_positive = cache_end - cache_offset;
      //Reversing the order of the negative bins
      //Note that because of the negative/positive ordering direction flip
      //We can not depend upon bin order and positions matching up
      //so bin_sizes must be reused to contain the end of the bin
      if (first_positive > 0) {
        bins[first_positive - 1] = first;
        for (int ii = first_positive - 2; ii >= 0; --ii) {
          bins[ii] = first + bin_sizes[ii + 1];
          bin_sizes[ii] += bin_sizes[ii + 1];
        }
        //Handling positives following negatives
        if (first_positive < bin_count) {
          bins[first_positive] = first + bin_sizes[0];
          bin_sizes[first_positive] += bin_sizes[0];
        }
      }
      else
        bins[0] = first;
      for (unsigned u = first_positive; u < bin_count - 1; u++) {
        bins[u + 1] = first + bin_sizes[u];
        bin_sizes[u + 1] += bin_sizes[u];
      }

      //Swap into place
      RandomAccessIter nextbinstart = first;
      for (unsigned u = 0; u < bin_count; ++u) {
        nextbinstart = first + bin_sizes[u];
        inner_float_swap_loop<RandomAccessIter, Div_type>
          (bins, nextbinstart, u, log_divisor, div_min);
      }

      if (!log_divisor)
        return;

      //Handling negative values first
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_offset + first_positive - 1; 
           ii >= static_cast<int>(cache_offset);
           lastPos = bin_cache[ii--]) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii]);
        //sort negative values using reversed-bin spreadsort
        else
          negative_float_sort_rec<RandomAccessIter, Div_type, Size_type>
            (lastPos, bin_cache[ii], bin_cache, cache_end, bin_sizes);
      }

      for (unsigned u = cache_offset + first_positive; u < cache_end;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u]);
        //sort positive values using normal spreadsort
        else
          positive_float_sort_rec<RandomAccessIter, Div_type, Size_type>
            (lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes);
      }
    }

    //Functor implementation for recursive sorting
    template <class RandomAccessIter, class Div_type, class Right_shift
      , class Size_type>
    inline void
    float_sort_rec(RandomAccessIter first, RandomAccessIter last,
              std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset
              , size_t *bin_sizes, Right_shift rshift)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes(first, last, max, min, rshift))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      //The index of the first positive bin
      unsigned first_positive = (div_min < 0) ? unsigned(-div_min) : 0;
      //Resetting if all bins are negative
      if (cache_offset + first_positive > cache_end)
        first_positive = cache_end - cache_offset;
      //Reversing the order of the negative bins
      //Note that because of the negative/positive ordering direction flip
      //We can not depend upon bin order and positions matching up
      //so bin_sizes must be reused to contain the end of the bin
      if (first_positive > 0) {
        bins[first_positive - 1] = first;
        for (int ii = first_positive - 2; ii >= 0; --ii) {
          bins[ii] = first + bin_sizes[ii + 1];
          bin_sizes[ii] += bin_sizes[ii + 1];
        }
        //Handling positives following negatives
        if (static_cast<unsigned>(first_positive) < bin_count) {
          bins[first_positive] = first + bin_sizes[0];
          bin_sizes[first_positive] += bin_sizes[0];
        }
      }
      else
        bins[0] = first;
      for (unsigned u = first_positive; u < bin_count - 1; u++) {
        bins[u + 1] = first + bin_sizes[u];
        bin_sizes[u + 1] += bin_sizes[u];
      }

      //Swap into place
      RandomAccessIter next_bin_start = first;
      for (unsigned u = 0; u < bin_count; ++u) {
        next_bin_start = first + bin_sizes[u];
        inner_swap_loop<RandomAccessIter, Div_type, Right_shift>
          (bins, next_bin_start, u, rshift, log_divisor, div_min);
      }

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Handling negative values first
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_offset + first_positive - 1; 
           ii >= static_cast<int>(cache_offset);
           lastPos = bin_cache[ii--]) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii]);
        //sort negative values using reversed-bin spreadsort
        else
          negative_float_sort_rec<RandomAccessIter, Div_type,
            Right_shift, Size_type>(lastPos, bin_cache[ii], bin_cache,
                                    cache_end, bin_sizes, rshift);
      }

      for (unsigned u = cache_offset + first_positive; u < cache_end;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u]);
        //sort positive values using normal spreadsort
        else
          spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Size_type,
                          float_log_mean_bin_size, float_log_min_split_count,
                          float_log_finishing_count>
            (lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, rshift);
      }
    }

    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare, class Size_type>
    inline void
    float_sort_rec(RandomAccessIter first, RandomAccessIter last,
            std::vector<RandomAccessIter> &bin_cache, unsigned cache_offset,
            size_t *bin_sizes, Right_shift rshift, Compare comp)
    {
      Div_type max, min;
      if (is_sorted_or_find_extremes(first, last, max, min, rshift, comp))
        return;
      unsigned log_divisor = get_log_divisor<float_log_mean_bin_size>(
          last - first, rough_log_2_size(Size_type(max - min)));
      Div_type div_min = min >> log_divisor;
      Div_type div_max = max >> log_divisor;
      unsigned bin_count = unsigned(div_max - div_min) + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, bin_count);

      //Calculating the size of each bin
      for (RandomAccessIter current = first; current != last;)
        bin_sizes[unsigned(rshift(*(current++), log_divisor) - div_min)]++;
      //The index of the first positive bin
      unsigned first_positive = 
        (div_min < 0) ? static_cast<unsigned>(-div_min) : 0;
      //Resetting if all bins are negative
      if (cache_offset + first_positive > cache_end)
        first_positive = cache_end - cache_offset;
      //Reversing the order of the negative bins
      //Note that because of the negative/positive ordering direction flip
      //We can not depend upon bin order and positions matching up
      //so bin_sizes must be reused to contain the end of the bin
      if (first_positive > 0) {
        bins[first_positive - 1] = first;
        for (int ii = first_positive - 2; ii >= 0; --ii) {
          bins[ii] = first + bin_sizes[ii + 1];
          bin_sizes[ii] += bin_sizes[ii + 1];
        }
        //Handling positives following negatives
        if (static_cast<unsigned>(first_positive) < bin_count) {
          bins[first_positive] = first + bin_sizes[0];
          bin_sizes[first_positive] += bin_sizes[0];
        }
      }
      else
        bins[0] = first;
      for (unsigned u = first_positive; u < bin_count - 1; u++) {
        bins[u + 1] = first + bin_sizes[u];
        bin_sizes[u + 1] += bin_sizes[u];
      }

      //Swap into place
      RandomAccessIter next_bin_start = first;
      for (unsigned u = 0; u < bin_count; ++u) {
        next_bin_start = first + bin_sizes[u];
        inner_swap_loop<RandomAccessIter, Div_type, Right_shift>
          (bins, next_bin_start, u, rshift, log_divisor, div_min);
      }

      //Return if we've completed bucketsorting
      if (!log_divisor)
        return;

      //Handling negative values first
      size_t max_count = get_min_count<float_log_mean_bin_size,
                                       float_log_min_split_count,
                                       float_log_finishing_count>(log_divisor);
      RandomAccessIter lastPos = first;
      for (int ii = cache_offset + first_positive - 1; 
           ii >= static_cast<int>(cache_offset);
           lastPos = bin_cache[ii--]) {
        size_t count = bin_cache[ii] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[ii], comp);
        //sort negative values using reversed-bin spreadsort
        else
          negative_float_sort_rec<RandomAccessIter, Div_type, Right_shift,
                                  Compare, Size_type>(lastPos, bin_cache[ii],
                                                      bin_cache, cache_end,
                                                      bin_sizes, rshift, comp);
      }

      for (unsigned u = cache_offset + first_positive; u < cache_end;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        if (count < 2)
          continue;
        if (count < max_count)
          boost::sort::pdqsort(lastPos, bin_cache[u], comp);
        //sort positive values using normal spreadsort
        else
          spreadsort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
                          Size_type, float_log_mean_bin_size,
                          float_log_min_split_count, float_log_finishing_count>
      (lastPos, bin_cache[u], bin_cache, cache_end, bin_sizes, rshift, comp);
      }
    }

    //Checking whether the value type is a float, and trying a 32-bit integer
    template <class RandomAccessIter>
    inline typename boost::enable_if_c< sizeof(boost::uint32_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type)
      && std::numeric_limits<typename
      std::iterator_traits<RandomAccessIter>::value_type>::is_iec559,
      void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, boost::int32_t, boost::uint32_t>
        (first, last, bin_cache, 0, bin_sizes);
    }

    //Checking whether the value type is a double, and using a 64-bit integer
    template <class RandomAccessIter>
    inline typename boost::enable_if_c< sizeof(boost::uint64_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type)
      && std::numeric_limits<typename
      std::iterator_traits<RandomAccessIter>::value_type>::is_iec559,
      void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, boost::int64_t, boost::uint64_t>
        (first, last, bin_cache, 0, bin_sizes);
    }

    template <class RandomAccessIter>
    inline typename boost::disable_if_c< (sizeof(boost::uint64_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type)
      || sizeof(boost::uint32_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type))
      && std::numeric_limits<typename
      std::iterator_traits<RandomAccessIter>::value_type>::is_iec559,
      void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last)
    {
      BOOST_STATIC_ASSERT(!(sizeof(boost::uint64_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type)
      || sizeof(boost::uint32_t) ==
      sizeof(typename std::iterator_traits<RandomAccessIter>::value_type))
      || !std::numeric_limits<typename
      std::iterator_traits<RandomAccessIter>::value_type>::is_iec559);
      boost::sort::pdqsort(first, last);
    }

    //These approaches require the user to do the typecast
    //with rshift but default comparision
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline typename boost::enable_if_c< sizeof(size_t) >= sizeof(Div_type),
      void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, Div_type, Right_shift, size_t>
        (first, last, bin_cache, 0, bin_sizes, rshift);
    }

    //maximum integer size with rshift but default comparision
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline typename boost::enable_if_c< sizeof(size_t) < sizeof(Div_type)
      && sizeof(boost::uintmax_t) >= sizeof(Div_type), void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, Div_type, Right_shift, boost::uintmax_t>
        (first, last, bin_cache, 0, bin_sizes, rshift);
    }

    //sizeof(Div_type) doesn't match, so use boost::sort::pdqsort
    template <class RandomAccessIter, class Div_type, class Right_shift>
    inline typename boost::disable_if_c< sizeof(boost::uintmax_t) >=
      sizeof(Div_type), void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift)
    {
      BOOST_STATIC_ASSERT(sizeof(boost::uintmax_t) >= sizeof(Div_type));
      boost::sort::pdqsort(first, last);
    }

    //specialized comparison
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    inline typename boost::enable_if_c< sizeof(size_t) >= sizeof(Div_type),
      void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift, Compare comp)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
        size_t>
        (first, last, bin_cache, 0, bin_sizes, rshift, comp);
    }

    //max-sized integer with specialized comparison
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    inline typename boost::enable_if_c< sizeof(size_t) < sizeof(Div_type)
      && sizeof(boost::uintmax_t) >= sizeof(Div_type), void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift, Compare comp)
    {
      size_t bin_sizes[1 << max_finishing_splits];
      std::vector<RandomAccessIter> bin_cache;
      float_sort_rec<RandomAccessIter, Div_type, Right_shift, Compare,
        boost::uintmax_t>
        (first, last, bin_cache, 0, bin_sizes, rshift, comp);
    }

    //sizeof(Div_type) doesn't match, so use boost::sort::pdqsort
    template <class RandomAccessIter, class Div_type, class Right_shift,
              class Compare>
    inline typename boost::disable_if_c< sizeof(boost::uintmax_t) >=
      sizeof(Div_type), void >::type
    float_sort(RandomAccessIter first, RandomAccessIter last, Div_type,
               Right_shift rshift, Compare comp)
    {
      BOOST_STATIC_ASSERT(sizeof(boost::uintmax_t) >= sizeof(Div_type));
      boost::sort::pdqsort(first, last, comp);
    }
  }
}
}
}

#endif
