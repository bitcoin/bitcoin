///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock.
//  Copyright Christopher Kormanyos 2013. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MP_DETAIL_DYNAMIC_ARRAY_HPP
#define BOOST_MP_DETAIL_DYNAMIC_ARRAY_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/multiprecision/detail/rebind.hpp>

namespace boost { namespace multiprecision { namespace backends { namespace detail {
template <class ValueType, const std::uint32_t ElemNumber, class my_allocator>
struct dynamic_array : public std::vector<ValueType, typename rebind<ValueType, my_allocator>::type>
{
private:
   using base_class_type = std::vector<ValueType, typename rebind<ValueType, my_allocator>::type>;

public:
   dynamic_array()
      : base_class_type(static_cast<typename base_class_type::size_type>(ElemNumber),
                        static_cast<typename base_class_type::value_type>(0u)) { }

   dynamic_array(std::initializer_list<std::uint32_t> lst)
      : base_class_type(static_cast<typename base_class_type::size_type>(ElemNumber),
                        static_cast<typename base_class_type::value_type>(0u))
   {
      std::copy(lst.begin(),
                lst.begin() + (std::min)(std::size_t(lst.size()), std::size_t(ElemNumber)),
                data());
   }

         typename base_class_type::value_type* data()       { return &(*(this->begin())); }
   const typename base_class_type::value_type* data() const { return &(*(this->begin())); }
};
}}}} // namespace boost::multiprecision::backends::detail

#endif // BOOST_MP_DETAIL_DYNAMIC_ARRAY_HPP
