//Templated Spreadsort-based implementation of float_sort and float_mem_cast

//          Copyright Steven J. Ross 2001 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/sort/ for library home page.

/*
Some improvements suggested by:
Phil Endecott and Frank Gennari
float_mem_cast fix provided by:
Scott McMurray
*/

#ifndef BOOST_FLOAT_SORT_HPP
#define BOOST_FLOAT_SORT_HPP
#include <algorithm>
#include <vector>
#include <cstring>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/sort/spreadsort/detail/constants.hpp>
#include <boost/sort/spreadsort/detail/float_sort.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

namespace boost {
namespace sort {
namespace spreadsort {

  /*!
  \brief Casts a float to the specified integer type.

  \tparam Data_type Floating-point IEEE 754/IEC559 type.
  \tparam Cast_type Integer type (same size) to which to cast.

  \par Example:
  \code
  struct rightshift {
    int operator()(const DATA_TYPE &x, const unsigned offset) const {
      return float_mem_cast<KEY_TYPE, CAST_TYPE>(x.key) >> offset;
    }
  };
  \endcode
  */
  template<class Data_type, class Cast_type>
  inline Cast_type
  float_mem_cast(const Data_type & data)
  {
    // Only cast IEEE floating-point numbers, and only to a same-sized integer.
    BOOST_STATIC_ASSERT(sizeof(Cast_type) == sizeof(Data_type));
    BOOST_STATIC_ASSERT(std::numeric_limits<Data_type>::is_iec559);
    BOOST_STATIC_ASSERT(std::numeric_limits<Cast_type>::is_integer);
    Cast_type result;
    std::memcpy(&result, &data, sizeof(Cast_type));
    return result;
  }


  /*!
    \brief @c float_sort with casting to the appropriate size.

    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.

Some performance plots of runtime vs. n and log(range) are provided:\n
   <a href="../../doc/graph/windows_float_sort.htm"> windows_float_sort</a>
   \n
   <a href="../../doc/graph/osx_float_sort.htm"> osx_float_sort</a>



   \par A simple example of sorting some floating-point is:
   \code
     vector<float> vec;
     vec.push_back(1.0);
     vec.push_back(2.3);
     vec.push_back(1.3);
     spreadsort(vec.begin(), vec.end());
   \endcode
   \par The sorted vector contains ascending values "1.0 1.3 2.3".

  */
  template <class RandomAccessIter>
  inline void float_sort(RandomAccessIter first, RandomAccessIter last)
  {
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else
      detail::float_sort(first, last);
  }

    /*!
    \brief Floating-point sort algorithm using range.

    \param[in] range Range [first, last) for sorting.

  */
  template <class Range>
  inline void float_sort(Range& range)
  {
    float_sort(boost::begin(range), boost::end(range));
  }

  /*!
    \brief Floating-point sort algorithm using random access iterators with just right-shift functor.

    \param[in] first Iterator pointer to first element.
    \param[in] last Iterator pointing to one beyond the end of data.
    \param[in] rshift Functor that returns the result of shifting the value_type right a specified number of bits.

  */
  template <class RandomAccessIter, class Right_shift>
  inline void float_sort(RandomAccessIter first, RandomAccessIter last,
                         Right_shift rshift)
  {
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last);
    else
      detail::float_sort(first, last, rshift(*first, 0), rshift);
  }

    /*!
    \brief Floating-point sort algorithm using range with just right-shift functor.

    \param[in] range Range [first, last) for sorting.
    \param[in] rshift Functor that returns the result of shifting the value_type right a specified number of bits.

  */
  template <class Range, class Right_shift>
  inline void float_sort(Range& range, Right_shift rshift)
  {
      float_sort(boost::begin(range), boost::end(range), rshift);
  }


  /*!
   \brief Float sort algorithm using random access iterators with both right-shift and user-defined comparison operator.

   \param[in] first Iterator pointer to first element.
   \param[in] last Iterator pointing to one beyond the end of data.
   \param[in] rshift Functor that returns the result of shifting the value_type right a specified number of bits.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.
  */

  template <class RandomAccessIter, class Right_shift, class Compare>
  inline void float_sort(RandomAccessIter first, RandomAccessIter last,
                         Right_shift rshift, Compare comp)
  {
    if (last - first < detail::min_sort_size)
      boost::sort::pdqsort(first, last, comp);
    else
      detail::float_sort(first, last, rshift(*first, 0), rshift, comp);
  }


    /*!
   \brief Float sort algorithm using range with both right-shift and user-defined comparison operator.

   \param[in] range Range [first, last) for sorting.
   \param[in] rshift Functor that returns the result of shifting the value_type right a specified number of bits.
   \param[in] comp A binary functor that returns whether the first element passed to it should go before the second in order.
  */

  template <class Range, class Right_shift, class Compare>
  inline void float_sort(Range& range, Right_shift rshift, Compare comp)
  {
      float_sort(boost::begin(range), boost::end(range), rshift, comp);
  }
}
}
}

#endif
