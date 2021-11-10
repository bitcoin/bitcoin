///////////////////////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock.
//  Copyright Christopher Kormanyos 2013. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MP_UTYPE_HELPER_HPP
#define BOOST_MP_UTYPE_HELPER_HPP

#include <limits>
#include <cstdint>

namespace boost {
namespace multiprecision {
namespace detail {
template <const unsigned>
struct utype_helper
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<0U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<1U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<2U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<3U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<4U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<5U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<6U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<7U>
{
   using exact = boost::uint8_t;
};
template <>
struct utype_helper<8U>
{
   using exact = boost::uint8_t;
};

template <>
struct utype_helper<9U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<10U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<11U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<12U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<13U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<14U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<15U>
{
   using exact = std::uint16_t;
};
template <>
struct utype_helper<16U>
{
   using exact = std::uint16_t;
};

template <>
struct utype_helper<17U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<18U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<19U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<20U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<21U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<22U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<23U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<24U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<25U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<26U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<27U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<28U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<29U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<30U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<31U>
{
   using exact = std::uint32_t;
};
template <>
struct utype_helper<32U>
{
   using exact = std::uint32_t;
};

template <>
struct utype_helper<33U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<34U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<35U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<36U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<37U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<38U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<39U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<40U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<41U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<42U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<43U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<44U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<45U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<46U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<47U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<48U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<49U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<50U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<51U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<52U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<53U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<54U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<55U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<56U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<57U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<58U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<59U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<60U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<61U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<62U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<63U>
{
   using exact = std::uint64_t;
};
template <>
struct utype_helper<64U>
{
   using exact = std::uint64_t;
};

template <class unsigned_type>
int utype_prior(unsigned_type ui)
{
   // TBD: Implement a templated binary search for this.
   int priority_bit;

   unsigned_type priority_mask = unsigned_type(unsigned_type(1U) << (std::numeric_limits<unsigned_type>::digits - 1));

   for (priority_bit = std::numeric_limits<unsigned_type>::digits - 1; priority_bit >= 0; --priority_bit)
   {
      if (unsigned_type(priority_mask & ui) != unsigned_type(0U))
      {
         break;
      }

      priority_mask >>= 1;
   }

   return priority_bit;
}

}}} // namespace boost::multiprecision::detail

#endif // BOOST_MP_UTYPE_HELPER_HPP
