// Contains get_min_count, the core optimization of the spreadsort algorithm.
// Also has other helper functions commonly useful across variants.

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
*/

#ifndef BOOST_SORT_SPREADSORT_DETAIL_SPREAD_SORT_COMMON_HPP
#define BOOST_SORT_SPREADSORT_DETAIL_SPREAD_SORT_COMMON_HPP
#include <algorithm>
#include <vector>
#include <cstring>
#include <limits>
#include <functional>
#include <boost/static_assert.hpp>
#include <boost/sort/pdqsort/pdqsort.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/cstdint.hpp>

namespace boost {
namespace sort {
namespace spreadsort {
 namespace detail {
    //This only works on unsigned data types
    template <typename T>
    inline unsigned
    rough_log_2_size(const T& input)
    {
      unsigned result = 0;
      //The && is necessary on some compilers to avoid infinite loops
      //it doesn't significantly impair performance
      while ((result < (8*sizeof(T))) && (input >> result)) ++result;
      return result;
    }

    //Gets the minimum size to call spreadsort on to control worst-case runtime.
    //This is called for a set of bins, instead of bin-by-bin, to minimize
    //runtime overhead.
    //This could be replaced by a lookup table of sizeof(Div_type)*8 but this
    //function is more general.
    template<unsigned log_mean_bin_size,
      unsigned log_min_split_count, unsigned log_finishing_count>
    inline size_t
    get_min_count(unsigned log_range)
    {
      const size_t typed_one = 1;
      const unsigned min_size = log_mean_bin_size + log_min_split_count;
      //Assuring that constants have valid settings
      BOOST_STATIC_ASSERT(log_min_split_count <= max_splits &&
                          log_min_split_count > 0);
      BOOST_STATIC_ASSERT(max_splits > 1 &&
                          max_splits < (8 * sizeof(unsigned)));
      BOOST_STATIC_ASSERT(max_finishing_splits >= max_splits &&
                          max_finishing_splits < (8 * sizeof(unsigned)));
      BOOST_STATIC_ASSERT(log_mean_bin_size >= 0);
      BOOST_STATIC_ASSERT(log_finishing_count >= 0);
      //if we can complete in one iteration, do so
      //This first check allows the compiler to optimize never-executed code out
      if (log_finishing_count < min_size) {
        if (log_range <= min_size && log_range <= max_splits) {
          //Return no smaller than a certain minimum limit
          if (log_range <= log_finishing_count)
            return typed_one << log_finishing_count;
          return typed_one << log_range;
        }
      }
      const unsigned base_iterations = max_splits - log_min_split_count;
      //sum of n to n + x = ((x + 1) * (n + (n + x)))/2 + log_mean_bin_size
      const unsigned base_range =
          ((base_iterations + 1) * (max_splits + log_min_split_count))/2
          + log_mean_bin_size;
      //Calculating the required number of iterations, and returning
      //1 << (iteration_count + min_size)
      if (log_range < base_range) {
        unsigned result = log_min_split_count;
        for (unsigned offset = min_size; offset < log_range;
          offset += ++result);
        //Preventing overflow; this situation shouldn't occur
        if ((result + log_mean_bin_size) >= (8 * sizeof(size_t)))
          return typed_one << ((8 * sizeof(size_t)) - 1);
        return typed_one << (result + log_mean_bin_size);
      }
      //A quick division can calculate the worst-case runtime for larger ranges
      unsigned remainder = log_range - base_range;
      //the max_splits - 1 is used to calculate the ceiling of the division
      unsigned bit_length = ((((max_splits - 1) + remainder)/max_splits)
        + base_iterations + min_size);
      //Preventing overflow; this situation shouldn't occur
      if (bit_length >= (8 * sizeof(size_t)))
        return typed_one << ((8 * sizeof(size_t)) - 1);
      //n(log_range)/max_splits + C, optimizing worst-case performance
      return typed_one << bit_length;
    }

    // Resizes the bin cache and bin sizes, and initializes each bin size to 0.
    // This generates the memory overhead to use in radix sorting.
    template <class RandomAccessIter>
    inline RandomAccessIter *
    size_bins(size_t *bin_sizes, std::vector<RandomAccessIter>
  &bin_cache, unsigned cache_offset, unsigned &cache_end, unsigned bin_count)
    {
      // Clear the bin sizes
      for (size_t u = 0; u < bin_count; u++)
        bin_sizes[u] = 0;
      //Make sure there is space for the bins
      cache_end = cache_offset + bin_count;
      if (cache_end > bin_cache.size())
        bin_cache.resize(cache_end);
      return &(bin_cache[cache_offset]);
    }
  }
}
}
}

#endif
