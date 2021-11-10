///////////////////////////////////////////////////////////////////////////////
//  Copyright 2021 John Maddock.
//  Copyright Christopher Kormanyos 2021. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MP_DETAIL_STATIC_ARRAY_HPP
#define BOOST_MP_DETAIL_STATIC_ARRAY_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace boost { namespace multiprecision { namespace backends { namespace detail {
template <class ValueType, const std::uint32_t ElemNumber>
struct static_array : public std::array<ValueType, std::size_t(ElemNumber)>
{
private:
   using base_class_type = std::array<ValueType, std::size_t(ElemNumber)>;

public:
   static_array() noexcept
   {
      base_class_type::fill(typename base_class_type::value_type(0u));
   }

   static_array(std::initializer_list<std::uint32_t> lst) noexcept
   {
      std::copy(lst.begin(),
                lst.begin() + (std::min)(std::size_t(lst.size()), std::size_t(ElemNumber)),
                base_class_type::begin());

      std::fill(base_class_type::begin() + (std::min)(std::size_t(lst.size()), std::size_t(ElemNumber)),
                base_class_type::end(),
                typename base_class_type::value_type(0u));
   }
};
}}}} // namespace boost::multiprecision::backends::detail

#endif // BOOST_MP_DETAIL_STATIC_ARRAY_HPP
