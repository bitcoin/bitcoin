// Details for a templated general-case hybrid-radix string_sort.

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
*/

#ifndef BOOST_SORT_SPREADSORT_DETAIL_SPREAD_SORT_HPP
#define BOOST_SORT_SPREADSORT_DETAIL_SPREAD_SORT_HPP
#include <algorithm>
#include <vector>
#include <cstring>
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
    static const int max_step_size = 64;

    //Offsetting on identical characters.  This function works a chunk of
    //characters at a time for cache efficiency and optimal worst-case
    //performance.
    template<class RandomAccessIter, class Unsigned_char_type>
    inline void
    update_offset(RandomAccessIter first, RandomAccessIter finish,
                  size_t &char_offset)
    {
      const int char_size = sizeof(Unsigned_char_type);
      size_t nextOffset = char_offset;
      int step_size = max_step_size / char_size;
      while (true) {
        RandomAccessIter curr = first;
        do {
          //Ignore empties, but if the nextOffset would exceed the length or
          //not match, exit; we've found the last matching character
          //This will reduce the step_size if the current step doesn't match.
          if ((*curr).size() > char_offset) {
            if((*curr).size() <= (nextOffset + step_size)) {
              step_size = (*curr).size() - nextOffset - 1;
              if (step_size < 1) {
                char_offset = nextOffset;
                return;
              }
            }
            const int step_byte_size = step_size * char_size;
            if (memcmp(curr->data() + nextOffset, first->data() + nextOffset, 
                       step_byte_size) != 0) {
              if (step_size == 1) {
                char_offset = nextOffset;
                return;
              }
              step_size = (step_size > 4) ? 4 : 1;
              continue;
            }
          }
          ++curr;
        } while (curr != finish);
        nextOffset += step_size;
      }
    }

    //Offsetting on identical characters.  This function works a character
    //at a time for optimal worst-case performance.
    template<class RandomAccessIter, class Get_char, class Get_length>
    inline void
    update_offset(RandomAccessIter first, RandomAccessIter finish,
                  size_t &char_offset, Get_char get_character, Get_length length)
    {
      size_t nextOffset = char_offset;
      while (true) {
        RandomAccessIter curr = first;
        do {
          //ignore empties, but if the nextOffset would exceed the length or
          //not match, exit; we've found the last matching character
          if (length(*curr) > char_offset && (length(*curr) <= (nextOffset + 1)
            || get_character((*curr), nextOffset) != get_character((*first), nextOffset))) {
            char_offset = nextOffset;
            return;
          }
        } while (++curr != finish);
        ++nextOffset;
      }
    }

    //This comparison functor assumes strings are identical up to char_offset
    template<class Data_type, class Unsigned_char_type>
    struct offset_less_than {
      offset_less_than(size_t char_offset) : fchar_offset(char_offset){}
      inline bool operator()(const Data_type &x, const Data_type &y) const
      {
        size_t minSize = (std::min)(x.size(), y.size());
        for (size_t u = fchar_offset; u < minSize; ++u) {
          BOOST_STATIC_ASSERT(sizeof(x[u]) == sizeof(Unsigned_char_type));
          if (static_cast<Unsigned_char_type>(x[u]) !=
              static_cast<Unsigned_char_type>(y[u])) {
            return static_cast<Unsigned_char_type>(x[u]) < 
              static_cast<Unsigned_char_type>(y[u]);
          }
        }
        return x.size() < y.size();
      }
      size_t fchar_offset;
    };

    //Compares strings assuming they are identical up to char_offset
    template<class Data_type, class Unsigned_char_type>
    struct offset_greater_than {
      offset_greater_than(size_t char_offset) : fchar_offset(char_offset){}
      inline bool operator()(const Data_type &x, const Data_type &y) const
      {
        size_t minSize = (std::min)(x.size(), y.size());
        for (size_t u = fchar_offset; u < minSize; ++u) {
          BOOST_STATIC_ASSERT(sizeof(x[u]) == sizeof(Unsigned_char_type));
          if (static_cast<Unsigned_char_type>(x[u]) !=
              static_cast<Unsigned_char_type>(y[u])) {
            return static_cast<Unsigned_char_type>(x[u]) > 
              static_cast<Unsigned_char_type>(y[u]);
          }
        }
        return x.size() > y.size();
      }
      size_t fchar_offset;
    };

    //This comparison functor assumes strings are identical up to char_offset
    template<class Data_type, class Get_char, class Get_length>
    struct offset_char_less_than {
      offset_char_less_than(size_t char_offset) : fchar_offset(char_offset){}
      inline bool operator()(const Data_type &x, const Data_type &y) const
      {
        size_t minSize = (std::min)(length(x), length(y));
        for (size_t u = fchar_offset; u < minSize; ++u) {
          if (get_character(x, u) != get_character(y, u)) {
            return get_character(x, u) < get_character(y, u);
          }
        }
        return length(x) < length(y);
      }
      size_t fchar_offset;
      Get_char get_character;
      Get_length length;
    };

    //String sorting recursive implementation
    template <class RandomAccessIter, class Unsigned_char_type>
    inline void
    string_sort_rec(RandomAccessIter first, RandomAccessIter last,
                    size_t char_offset,
                    std::vector<RandomAccessIter> &bin_cache,
                    unsigned cache_offset, size_t *bin_sizes)
    {
      typedef typename std::iterator_traits<RandomAccessIter>::value_type
        Data_type;
      //This section makes handling of long identical substrings much faster
      //with a mild average performance impact.
      //Iterate to the end of the empties.  If all empty, return
      while ((*first).size() <= char_offset) {
        if (++first == last)
          return;
      }
      RandomAccessIter finish = last - 1;
      //Getting the last non-empty
      for (;(*finish).size() <= char_offset; --finish);
      ++finish;
      //Offsetting on identical characters.  This section works
      //a few characters at a time for optimal worst-case performance.
      update_offset<RandomAccessIter, Unsigned_char_type>(first, finish,
                                                          char_offset);
      
      const unsigned bin_count = (1 << (sizeof(Unsigned_char_type)*8));
      //Equal worst-case of radix and comparison is when bin_count = n*log(n).
      const unsigned max_size = bin_count;
      const unsigned membin_count = bin_count + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, membin_count) + 1;

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last; ++current) {
        if ((*current).size() <= char_offset) {
          bin_sizes[0]++;
        }
        else
          bin_sizes[static_cast<Unsigned_char_type>((*current)[char_offset])
                    + 1]++;
      }
      //Assign the bin positions
      bin_cache[cache_offset] = first;
      for (unsigned u = 0; u < membin_count - 1; u++)
        bin_cache[cache_offset + u + 1] =
          bin_cache[cache_offset + u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = first;
      //handling empty bins
      RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
      next_bin_start +=  bin_sizes[0];
      RandomAccessIter * target_bin;
      //Iterating over each element in the bin of empties
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        //empties belong in this bin
        while ((*current).size() > char_offset) {
          target_bin =
            bins + static_cast<Unsigned_char_type>((*current)[char_offset]);
          iter_swap(current, (*target_bin)++);
        }
      }
      *local_bin = next_bin_start;
      //iterate backwards to find the last bin with elements in it
      //this saves iterations in multiple loops
      unsigned last_bin = bin_count - 1;
      for (; last_bin && !bin_sizes[last_bin + 1]; --last_bin);
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned u = 0; u < last_bin; ++u) {
        local_bin = bins + u;
        next_bin_start += bin_sizes[u + 1];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < next_bin_start;
            ++current) {
          //Swapping into place until the correct element has been swapped in
          for (target_bin = bins + static_cast<Unsigned_char_type>
              ((*current)[char_offset]);  target_bin != local_bin;
            target_bin = bins + static_cast<Unsigned_char_type>
              ((*current)[char_offset])) iter_swap(current, (*target_bin)++);
        }
        *local_bin = next_bin_start;
      }
      bins[last_bin] = last;
      //Recursing
      RandomAccessIter lastPos = bin_cache[cache_offset];
      //Skip this loop for empties
      for (unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_size)
          boost::sort::pdqsort(lastPos, bin_cache[u],
              offset_less_than<Data_type, Unsigned_char_type>(char_offset + 1));
        else
          string_sort_rec<RandomAccessIter, Unsigned_char_type>(lastPos,
              bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes);
      }
    }

    //Sorts strings in reverse order, with empties at the end
    template <class RandomAccessIter, class Unsigned_char_type>
    inline void
    reverse_string_sort_rec(RandomAccessIter first, RandomAccessIter last,
                            size_t char_offset,
                            std::vector<RandomAccessIter> &bin_cache,
                            unsigned cache_offset,
                            size_t *bin_sizes)
    {
      typedef typename std::iterator_traits<RandomAccessIter>::value_type
        Data_type;
      //This section makes handling of long identical substrings much faster
      //with a mild average performance impact.
      RandomAccessIter curr = first;
      //Iterate to the end of the empties.  If all empty, return
      while ((*curr).size() <= char_offset) {
        if (++curr == last)
          return;
      }
      //Getting the last non-empty
      while ((*(--last)).size() <= char_offset);
      ++last;
      //Offsetting on identical characters.  This section works
      //a few characters at a time for optimal worst-case performance.
      update_offset<RandomAccessIter, Unsigned_char_type>(curr, last,
                                                          char_offset);
      RandomAccessIter * target_bin;

      const unsigned bin_count = (1 << (sizeof(Unsigned_char_type)*8));
      //Equal worst-case of radix and comparison when bin_count = n*log(n).
      const unsigned max_size = bin_count;
      const unsigned membin_count = bin_count + 1;
      const unsigned max_bin = bin_count - 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, membin_count);
      RandomAccessIter * end_bin = &(bin_cache[cache_offset + max_bin]);

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last; ++current) {
        if ((*current).size() <= char_offset) {
          bin_sizes[bin_count]++;
        }
        else
          bin_sizes[max_bin - static_cast<Unsigned_char_type>
            ((*current)[char_offset])]++;
      }
      //Assign the bin positions
      bin_cache[cache_offset] = first;
      for (unsigned u = 0; u < membin_count - 1; u++)
        bin_cache[cache_offset + u + 1] =
          bin_cache[cache_offset + u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = last;
      //handling empty bins
      RandomAccessIter * local_bin = &(bin_cache[cache_offset + bin_count]);
      RandomAccessIter lastFull = *local_bin;
      //Iterating over each element in the bin of empties
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        //empties belong in this bin
        while ((*current).size() > char_offset) {
          target_bin =
            end_bin - static_cast<Unsigned_char_type>((*current)[char_offset]);
          iter_swap(current, (*target_bin)++);
        }
      }
      *local_bin = next_bin_start;
      next_bin_start = first;
      //iterate backwards to find the last non-empty bin
      //this saves iterations in multiple loops
      unsigned last_bin = max_bin;
      for (; last_bin && !bin_sizes[last_bin]; --last_bin);
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned u = 0; u < last_bin; ++u) {
        local_bin = bins + u;
        next_bin_start += bin_sizes[u];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < next_bin_start;
            ++current) {
          //Swapping into place until the correct element has been swapped in
          for (target_bin =
            end_bin - static_cast<Unsigned_char_type>((*current)[char_offset]);
            target_bin != local_bin;
            target_bin =
            end_bin - static_cast<Unsigned_char_type>((*current)[char_offset]))
              iter_swap(current, (*target_bin)++);
        }
        *local_bin = next_bin_start;
      }
      bins[last_bin] = lastFull;
      //Recursing
      RandomAccessIter lastPos = first;
      //Skip this loop for empties
      for (unsigned u = cache_offset; u <= cache_offset + last_bin;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_size)
          boost::sort::pdqsort(lastPos, bin_cache[u], offset_greater_than<Data_type,
                    Unsigned_char_type>(char_offset + 1));
        else
          reverse_string_sort_rec<RandomAccessIter, Unsigned_char_type>
    (lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end, bin_sizes);
      }
    }

    //String sorting recursive implementation
    template <class RandomAccessIter, class Unsigned_char_type, class Get_char,
              class Get_length>
    inline void
    string_sort_rec(RandomAccessIter first, RandomAccessIter last,
              size_t char_offset, std::vector<RandomAccessIter> &bin_cache,
              unsigned cache_offset, size_t *bin_sizes,
              Get_char get_character, Get_length length)
    {
      typedef typename std::iterator_traits<RandomAccessIter>::value_type
        Data_type;
      //This section makes handling of long identical substrings much faster
      //with a mild average performance impact.
      //Iterate to the end of the empties.  If all empty, return
      while (length(*first) <= char_offset) {
        if (++first == last)
          return;
      }
      RandomAccessIter finish = last - 1;
      //Getting the last non-empty
      for (;length(*finish) <= char_offset; --finish);
      ++finish;
      update_offset(first, finish, char_offset, get_character, length);

      const unsigned bin_count = (1 << (sizeof(Unsigned_char_type)*8));
      //Equal worst-case of radix and comparison is when bin_count = n*log(n).
      const unsigned max_size = bin_count;
      const unsigned membin_count = bin_count + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, membin_count) + 1;

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last; ++current) {
        if (length(*current) <= char_offset) {
          bin_sizes[0]++;
        }
        else
          bin_sizes[get_character((*current), char_offset) + 1]++;
      }
      //Assign the bin positions
      bin_cache[cache_offset] = first;
      for (unsigned u = 0; u < membin_count - 1; u++)
        bin_cache[cache_offset + u + 1] =
          bin_cache[cache_offset + u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = first;
      //handling empty bins
      RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
      next_bin_start +=  bin_sizes[0];
      RandomAccessIter * target_bin;
      //Iterating over each element in the bin of empties
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        //empties belong in this bin
        while (length(*current) > char_offset) {
          target_bin = bins + get_character((*current), char_offset);
          iter_swap(current, (*target_bin)++);
        }
      }
      *local_bin = next_bin_start;
      //iterate backwards to find the last bin with elements in it
      //this saves iterations in multiple loops
      unsigned last_bin = bin_count - 1;
      for (; last_bin && !bin_sizes[last_bin + 1]; --last_bin);
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned ii = 0; ii < last_bin; ++ii) {
        local_bin = bins + ii;
        next_bin_start += bin_sizes[ii + 1];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < next_bin_start;
            ++current) {
          //Swapping into place until the correct element has been swapped in
          for (target_bin = bins + get_character((*current), char_offset);
              target_bin != local_bin;
              target_bin = bins + get_character((*current), char_offset))
            iter_swap(current, (*target_bin)++);
        }
        *local_bin = next_bin_start;
      }
      bins[last_bin] = last;

      //Recursing
      RandomAccessIter lastPos = bin_cache[cache_offset];
      //Skip this loop for empties
      for (unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_size)
          boost::sort::pdqsort(lastPos, bin_cache[u], offset_char_less_than<Data_type,
                    Get_char, Get_length>(char_offset + 1));
        else
          string_sort_rec<RandomAccessIter, Unsigned_char_type, Get_char,
            Get_length>(lastPos, bin_cache[u], char_offset + 1, bin_cache,
                        cache_end, bin_sizes, get_character, length);
      }
    }

    //String sorting recursive implementation
    template <class RandomAccessIter, class Unsigned_char_type, class Get_char,
              class Get_length, class Compare>
    inline void
    string_sort_rec(RandomAccessIter first, RandomAccessIter last,
              size_t char_offset, std::vector<RandomAccessIter> &bin_cache,
              unsigned cache_offset, size_t *bin_sizes,
              Get_char get_character, Get_length length, Compare comp)
    {
      //This section makes handling of long identical substrings much faster
      //with a mild average performance impact.
      //Iterate to the end of the empties.  If all empty, return
      while (length(*first) <= char_offset) {
        if (++first == last)
          return;
      }
      RandomAccessIter finish = last - 1;
      //Getting the last non-empty
      for (;length(*finish) <= char_offset; --finish);
      ++finish;
      update_offset(first, finish, char_offset, get_character, length);

      const unsigned bin_count = (1 << (sizeof(Unsigned_char_type)*8));
      //Equal worst-case of radix and comparison is when bin_count = n*log(n).
      const unsigned max_size = bin_count;
      const unsigned membin_count = bin_count + 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, membin_count) + 1;

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last; ++current) {
        if (length(*current) <= char_offset) {
          bin_sizes[0]++;
        }
        else
          bin_sizes[get_character((*current), char_offset) + 1]++;
      }
      //Assign the bin positions
      bin_cache[cache_offset] = first;
      for (unsigned u = 0; u < membin_count - 1; u++)
        bin_cache[cache_offset + u + 1] =
          bin_cache[cache_offset + u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = first;
      //handling empty bins
      RandomAccessIter * local_bin = &(bin_cache[cache_offset]);
      next_bin_start +=  bin_sizes[0];
      RandomAccessIter * target_bin;
      //Iterating over each element in the bin of empties
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        //empties belong in this bin
        while (length(*current) > char_offset) {
          target_bin = bins + get_character((*current), char_offset);
          iter_swap(current, (*target_bin)++);
        }
      }
      *local_bin = next_bin_start;
      //iterate backwards to find the last bin with elements in it
      //this saves iterations in multiple loops
      unsigned last_bin = bin_count - 1;
      for (; last_bin && !bin_sizes[last_bin + 1]; --last_bin);
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned u = 0; u < last_bin; ++u) {
        local_bin = bins + u;
        next_bin_start += bin_sizes[u + 1];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < next_bin_start;
            ++current) {
          //Swapping into place until the correct element has been swapped in
          for (target_bin = bins + get_character((*current), char_offset);
              target_bin != local_bin;
              target_bin = bins + get_character((*current), char_offset))
            iter_swap(current, (*target_bin)++);
        }
        *local_bin = next_bin_start;
      }
      bins[last_bin] = last;

      //Recursing
      RandomAccessIter lastPos = bin_cache[cache_offset];
      //Skip this loop for empties
      for (unsigned u = cache_offset + 1; u < cache_offset + last_bin + 2;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_size)
          boost::sort::pdqsort(lastPos, bin_cache[u], comp);
        else
          string_sort_rec<RandomAccessIter, Unsigned_char_type, Get_char,
                          Get_length, Compare>
            (lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end,
             bin_sizes, get_character, length, comp);
      }
    }

    //Sorts strings in reverse order, with empties at the end
    template <class RandomAccessIter, class Unsigned_char_type, class Get_char,
              class Get_length, class Compare>
    inline void
    reverse_string_sort_rec(RandomAccessIter first, RandomAccessIter last,
              size_t char_offset, std::vector<RandomAccessIter> &bin_cache,
              unsigned cache_offset, size_t *bin_sizes,
              Get_char get_character, Get_length length, Compare comp)
    {
      //This section makes handling of long identical substrings much faster
      //with a mild average performance impact.
      RandomAccessIter curr = first;
      //Iterate to the end of the empties.  If all empty, return
      while (length(*curr) <= char_offset) {
        if (++curr == last)
          return;
      }
      //Getting the last non-empty
      while (length(*(--last)) <= char_offset);
      ++last;
      //Offsetting on identical characters.  This section works
      //a character at a time for optimal worst-case performance.
      update_offset(curr, last, char_offset, get_character, length);

      const unsigned bin_count = (1 << (sizeof(Unsigned_char_type)*8));
      //Equal worst-case of radix and comparison is when bin_count = n*log(n).
      const unsigned max_size = bin_count;
      const unsigned membin_count = bin_count + 1;
      const unsigned max_bin = bin_count - 1;
      unsigned cache_end;
      RandomAccessIter * bins = size_bins(bin_sizes, bin_cache, cache_offset,
                                          cache_end, membin_count);
      RandomAccessIter *end_bin = &(bin_cache[cache_offset + max_bin]);

      //Calculating the size of each bin; this takes roughly 10% of runtime
      for (RandomAccessIter current = first; current != last; ++current) {
        if (length(*current) <= char_offset) {
          bin_sizes[bin_count]++;
        }
        else
          bin_sizes[max_bin - get_character((*current), char_offset)]++;
      }
      //Assign the bin positions
      bin_cache[cache_offset] = first;
      for (unsigned u = 0; u < membin_count - 1; u++)
        bin_cache[cache_offset + u + 1] =
          bin_cache[cache_offset + u] + bin_sizes[u];

      //Swap into place
      RandomAccessIter next_bin_start = last;
      //handling empty bins
      RandomAccessIter * local_bin = &(bin_cache[cache_offset + bin_count]);
      RandomAccessIter lastFull = *local_bin;
      RandomAccessIter * target_bin;
      //Iterating over each element in the bin of empties
      for (RandomAccessIter current = *local_bin; current < next_bin_start;
          ++current) {
        //empties belong in this bin
        while (length(*current) > char_offset) {
          target_bin = end_bin - get_character((*current), char_offset);
          iter_swap(current, (*target_bin)++);
        }
      }
      *local_bin = next_bin_start;
      next_bin_start = first;
      //iterate backwards to find the last bin with elements in it
      //this saves iterations in multiple loops
      unsigned last_bin = max_bin;
      for (; last_bin && !bin_sizes[last_bin]; --last_bin);
      //This dominates runtime, mostly in the swap and bin lookups
      for (unsigned u = 0; u < last_bin; ++u) {
        local_bin = bins + u;
        next_bin_start += bin_sizes[u];
        //Iterating over each element in this bin
        for (RandomAccessIter current = *local_bin; current < next_bin_start;
            ++current) {
          //Swapping into place until the correct element has been swapped in
          for (target_bin = end_bin - get_character((*current), char_offset);
              target_bin != local_bin;
              target_bin = end_bin - get_character((*current), char_offset))
            iter_swap(current, (*target_bin)++);
        }
        *local_bin = next_bin_start;
      }
      bins[last_bin] = lastFull;
      //Recursing
      RandomAccessIter lastPos = first;
      //Skip this loop for empties
      for (unsigned u = cache_offset; u <= cache_offset + last_bin;
          lastPos = bin_cache[u], ++u) {
        size_t count = bin_cache[u] - lastPos;
        //don't sort unless there are at least two items to Compare
        if (count < 2)
          continue;
        //using boost::sort::pdqsort if its worst-case is better
        if (count < max_size)
          boost::sort::pdqsort(lastPos, bin_cache[u], comp);
        else
          reverse_string_sort_rec<RandomAccessIter, Unsigned_char_type,
                                  Get_char, Get_length, Compare>
            (lastPos, bin_cache[u], char_offset + 1, bin_cache, cache_end,
             bin_sizes, get_character, length, comp);
      }
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Unsigned_char_type>
    inline typename boost::enable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                      >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
                Unsigned_char_type)
    {
      size_t bin_sizes[(1 << (8 * sizeof(Unsigned_char_type))) + 1];
      std::vector<RandomAccessIter> bin_cache;
      string_sort_rec<RandomAccessIter, Unsigned_char_type>
        (first, last, 0, bin_cache, 0, bin_sizes);
    }

    template <class RandomAccessIter, class Unsigned_char_type>
    inline typename boost::disable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                       >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
                Unsigned_char_type)
    {
      //Warning that we're using boost::sort::pdqsort, even though string_sort was called
      BOOST_STATIC_ASSERT( sizeof(Unsigned_char_type) <= 2 );
      boost::sort::pdqsort(first, last);
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Unsigned_char_type>
    inline typename boost::enable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                      >::type
    reverse_string_sort(RandomAccessIter first, RandomAccessIter last,
                        Unsigned_char_type)
    {
      size_t bin_sizes[(1 << (8 * sizeof(Unsigned_char_type))) + 1];
      std::vector<RandomAccessIter> bin_cache;
      reverse_string_sort_rec<RandomAccessIter, Unsigned_char_type>
        (first, last, 0, bin_cache, 0, bin_sizes);
    }

    template <class RandomAccessIter, class Unsigned_char_type>
    inline typename boost::disable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                       >::type
    reverse_string_sort(RandomAccessIter first, RandomAccessIter last,
                Unsigned_char_type)
    {
      typedef typename std::iterator_traits<RandomAccessIter>::value_type
        Data_type;
      //Warning that we're using boost::sort::pdqsort, even though string_sort was called
      BOOST_STATIC_ASSERT( sizeof(Unsigned_char_type) <= 2 );
      boost::sort::pdqsort(first, last, std::greater<Data_type>());
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Get_char, class Get_length,
              class Unsigned_char_type>
    inline typename boost::enable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                      >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
                Get_char get_character, Get_length length, Unsigned_char_type)
    {
      size_t bin_sizes[(1 << (8 * sizeof(Unsigned_char_type))) + 1];
      std::vector<RandomAccessIter> bin_cache;
      string_sort_rec<RandomAccessIter, Unsigned_char_type, Get_char,
        Get_length>(first, last, 0, bin_cache, 0, bin_sizes, get_character, length);
    }

    template <class RandomAccessIter, class Get_char, class Get_length,
              class Unsigned_char_type>
    inline typename boost::disable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                       >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
                Get_char get_character, Get_length length, Unsigned_char_type)
    {
      //Warning that we're using boost::sort::pdqsort, even though string_sort was called
      BOOST_STATIC_ASSERT( sizeof(Unsigned_char_type) <= 2 );
      boost::sort::pdqsort(first, last);
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Get_char, class Get_length,
              class Compare, class Unsigned_char_type>
    inline typename boost::enable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                      >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
        Get_char get_character, Get_length length, Compare comp, Unsigned_char_type)
    {
      size_t bin_sizes[(1 << (8 * sizeof(Unsigned_char_type))) + 1];
      std::vector<RandomAccessIter> bin_cache;
      string_sort_rec<RandomAccessIter, Unsigned_char_type, Get_char
        , Get_length, Compare>
        (first, last, 0, bin_cache, 0, bin_sizes, get_character, length, comp);
    }

    //disable_if_c was refusing to compile, so rewrote to use enable_if_c
    template <class RandomAccessIter, class Get_char, class Get_length,
              class Compare, class Unsigned_char_type>
    inline typename boost::enable_if_c< (sizeof(Unsigned_char_type) > 2), void
                                        >::type
    string_sort(RandomAccessIter first, RandomAccessIter last,
        Get_char get_character, Get_length length, Compare comp, Unsigned_char_type)
    {
      //Warning that we're using boost::sort::pdqsort, even though string_sort was called
      BOOST_STATIC_ASSERT( sizeof(Unsigned_char_type) <= 2 );
      boost::sort::pdqsort(first, last, comp);
    }

    //Holds the bin vector and makes the initial recursive call
    template <class RandomAccessIter, class Get_char, class Get_length,
              class Compare, class Unsigned_char_type>
    inline typename boost::enable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                      >::type
    reverse_string_sort(RandomAccessIter first, RandomAccessIter last,
        Get_char get_character, Get_length length, Compare comp, Unsigned_char_type)
    {
      size_t bin_sizes[(1 << (8 * sizeof(Unsigned_char_type))) + 1];
      std::vector<RandomAccessIter> bin_cache;
      reverse_string_sort_rec<RandomAccessIter, Unsigned_char_type, Get_char,
                              Get_length, Compare>
        (first, last, 0, bin_cache, 0, bin_sizes, get_character, length, comp);
    }

    template <class RandomAccessIter, class Get_char, class Get_length,
              class Compare, class Unsigned_char_type>
    inline typename boost::disable_if_c< sizeof(Unsigned_char_type) <= 2, void
                                                                       >::type
    reverse_string_sort(RandomAccessIter first, RandomAccessIter last,
        Get_char get_character, Get_length length, Compare comp, Unsigned_char_type)
    {
      //Warning that we're using boost::sort::pdqsort, even though string_sort was called
      BOOST_STATIC_ASSERT( sizeof(Unsigned_char_type) <= 2 );
      boost::sort::pdqsort(first, last, comp);
    }
  }
}
}
}

#endif
