/*
  Copyright 2008 Intel Corporation

  Use, modification and distribution are subject to the Boost Software License,
  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
  http://www.boost.org/LICENSE_1_0.txt).
*/
#ifndef BOOST_POLYGON_SORT_ADAPTOR_HPP
#define BOOST_POLYGON_SORT_ADAPTOR_HPP
#ifdef __ICC
#pragma warning(disable:2022)
#pragma warning(disable:2023)
#endif

#include <algorithm>

//! @brief polygon_sort_adaptor default implementation that calls std::sort
namespace boost {
  namespace polygon {

    template<typename iterator_type>
    struct dummy_to_delay_instantiation{
      typedef int unit_type; // default GTL unit
    };

    //! @brief polygon_sort_adaptor default implementation that calls std::sort
    template<typename T>
    struct polygon_sort_adaptor {
      //! @brief wrapper that mimics std::sort() function and takes
      // the same arguments
      template<typename RandomAccessIterator_Type>
      static void sort(RandomAccessIterator_Type _First,
                       RandomAccessIterator_Type _Last)
      {
         std::sort(_First, _Last);
      }
      //! @brief wrapper that mimics std::sort() function overload and takes
      // the same arguments
      template<typename RandomAccessIterator_Type, typename Pred_Type>
      static void sort(RandomAccessIterator_Type _First,
                       RandomAccessIterator_Type _Last,
                       const Pred_Type& _Comp)
      {
         std::sort(_First, _Last, _Comp);
      }
    };

    //! @brief user level wrapper for sorting quantities
    template <typename iter_type>
    void polygon_sort(iter_type _b_, iter_type _e_)
    {
      polygon_sort_adaptor<typename dummy_to_delay_instantiation<iter_type>::unit_type>::sort(_b_, _e_);
    }

    //! @brief user level wrapper for sorting quantities that takes predicate
    // as additional argument
    template <typename iter_type, typename pred_type>
    void polygon_sort(iter_type _b_, iter_type _e_, const pred_type& _pred_)
    {
      polygon_sort_adaptor<typename dummy_to_delay_instantiation<iter_type>::unit_type>::sort(_b_, _e_, _pred_);
    }



  } // namespace polygon
}   // namespace boost
#endif
