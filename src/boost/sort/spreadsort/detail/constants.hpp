//constant definitions for the Boost Sort library

//          Copyright Steven J. Ross 2001 - 2014
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/sort for library home page.
#ifndef BOOST_SORT_SPREADSORT_DETAIL_CONSTANTS
#define BOOST_SORT_SPREADSORT_DETAIL_CONSTANTS
namespace boost {
namespace sort {
namespace spreadsort {
namespace detail {
//Tuning constants
//This should be tuned to your processor cache;
//if you go too large you get cache misses on bins
//The smaller this number, the less worst-case memory usage.
//If too small, too many recursions slow down spreadsort
enum { max_splits = 11,
//It's better to have a few cache misses and finish sorting
//than to run another iteration
max_finishing_splits = max_splits + 1,
//Sets the minimum number of items per bin.
int_log_mean_bin_size = 2,
//Used to force a comparison-based sorting for small bins, if it's faster.
//Minimum value 1
int_log_min_split_count = 9,
//This is the minimum split count to use spreadsort when it will finish in one
//iteration.  Make this larger the faster boost::sort::pdqsort is relative to integer_sort.
int_log_finishing_count = 31,
//Sets the minimum number of items per bin for floating point.
float_log_mean_bin_size = 2,
//Used to force a comparison-based sorting for small bins, if it's faster.
//Minimum value 1
float_log_min_split_count = 8,
//This is the minimum split count to use spreadsort when it will finish in one
//iteration.  Make this larger the faster boost::sort::pdqsort is relative to float_sort.
float_log_finishing_count = 4,
//There is a minimum size below which it is not worth using spreadsort
min_sort_size = 1000 };
}
}
}
}
#endif
