///////////////////////////////////////////////////////////////////////////////
//  Copyright 2021 John Maddock.
//  Copyright Christopher Kormanyos 2021. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_MP_DETAIL_TABLES_HPP
#define BOOST_MP_DETAIL_TABLES_HPP

#include <algorithm>
#include <array>
#include <cstdint>

namespace boost { namespace multiprecision { namespace backends { namespace detail {
struct a029750
{
   static constexpr std::uint32_t a029750_as_constexpr(const std::uint32_t value)
   {
      // Sloane's A029750 List of numbers of the form 2^k times 1, 3, 5 or 7.
      // CoefficientList[Series[-(x + 1)^2 (x^2 + 1)^2/(2 x^4 - 1), {x, 0, 78}], x]
      return ((value <= UINT32_C(     32)) ? UINT32_C(     32) : ((value <=  UINT32_C(     40)) ?  UINT32_C(     40) : ((value <= UINT32_C(     48)) ? UINT32_C(     48) : ((value <= UINT32_C(     56)) ? UINT32_C(     56) :
             ((value <= UINT32_C(     64)) ? UINT32_C(     64) : ((value <=  UINT32_C(     80)) ?  UINT32_C(     80) : ((value <= UINT32_C(     96)) ? UINT32_C(     96) : ((value <= UINT32_C(    112)) ? UINT32_C(    112) :
             ((value <= UINT32_C(    128)) ? UINT32_C(    128) : ((value <=  UINT32_C(    160)) ?  UINT32_C(    160) : ((value <= UINT32_C(    192)) ? UINT32_C(    192) : ((value <= UINT32_C(    224)) ? UINT32_C(    224) :
             ((value <= UINT32_C(    256)) ? UINT32_C(    256) : ((value <=  UINT32_C(    320)) ?  UINT32_C(    320) : ((value <= UINT32_C(    384)) ? UINT32_C(    384) : ((value <= UINT32_C(    448)) ? UINT32_C(    448) :
             ((value <= UINT32_C(    512)) ? UINT32_C(    512) : ((value <=  UINT32_C(    640)) ?  UINT32_C(    640) : ((value <= UINT32_C(    768)) ? UINT32_C(    768) : ((value <= UINT32_C(    896)) ? UINT32_C(    896) :
             ((value <= UINT32_C(   1024)) ? UINT32_C(   1024) : ((value <=  UINT32_C(   1280)) ?  UINT32_C(   1280) : ((value <= UINT32_C(   1536)) ? UINT32_C(   1536) : ((value <= UINT32_C(   1792)) ? UINT32_C(   1792) :
             ((value <= UINT32_C(   2048)) ? UINT32_C(   2048) : ((value <=  UINT32_C(   2560)) ?  UINT32_C(   2560) : ((value <= UINT32_C(   3072)) ? UINT32_C(   3072) : ((value <= UINT32_C(   3584)) ? UINT32_C(   3584) :
             ((value <= UINT32_C(   4096)) ? UINT32_C(   4096) : ((value <=  UINT32_C(   5120)) ?  UINT32_C(   5120) : ((value <= UINT32_C(   6144)) ? UINT32_C(   6144) : ((value <= UINT32_C(   7168)) ? UINT32_C(   7168) :
             ((value <= UINT32_C(   8192)) ? UINT32_C(   8192) : ((value <=  UINT32_C(  10240)) ?  UINT32_C(  10240) : ((value <= UINT32_C(  12288)) ? UINT32_C(  12288) : ((value <= UINT32_C(  14336)) ? UINT32_C(  14336) :
             ((value <= UINT32_C(  16384)) ? UINT32_C(  16384) : ((value <=  UINT32_C(  20480)) ?  UINT32_C(  20480) : ((value <= UINT32_C(  24576)) ? UINT32_C(  24576) : ((value <= UINT32_C(  28672)) ? UINT32_C(  28672) :
             ((value <= UINT32_C(  32768)) ? UINT32_C(  32768) : ((value <=  UINT32_C(  40960)) ?  UINT32_C(  40960) : ((value <= UINT32_C(  49152)) ? UINT32_C(  49152) : ((value <= UINT32_C(  57344)) ? UINT32_C(  57344) :
             ((value <= UINT32_C(  65536)) ? UINT32_C(  65536) : ((value <=  UINT32_C(  81920)) ?  UINT32_C(  81920) : ((value <= UINT32_C(  98304)) ? UINT32_C(  98304) : ((value <= UINT32_C( 114688)) ? UINT32_C( 114688) :
             ((value <= UINT32_C( 131072)) ? UINT32_C( 131072) : ((value <=  UINT32_C( 163840)) ?  UINT32_C( 163840) : ((value <= UINT32_C( 196608)) ? UINT32_C( 196608) : ((value <= UINT32_C( 229376)) ? UINT32_C( 229376) :
             ((value <= UINT32_C( 262144)) ? UINT32_C( 262144) : ((value <=  UINT32_C( 327680)) ?  UINT32_C( 327680) : ((value <= UINT32_C( 393216)) ? UINT32_C( 393216) : ((value <= UINT32_C( 458752)) ? UINT32_C( 458752) :
             ((value <= UINT32_C( 524288)) ? UINT32_C( 524288) : ((value <=  UINT32_C( 655360)) ?  UINT32_C( 655360) : ((value <= UINT32_C( 786432)) ? UINT32_C( 786432) : ((value <= UINT32_C( 917504)) ? UINT32_C( 917504) :
             ((value <= UINT32_C(1048576)) ? UINT32_C(1048576) : ((value <=  UINT32_C(1310720)) ?  UINT32_C(1310720) : ((value <= UINT32_C(1572864)) ? UINT32_C(1572864) : ((value <= UINT32_C(1835008)) ? UINT32_C(1835008) : UINT32_C(0x7FFFFFFF)))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))));
   }

   static std::uint32_t a029750_as_runtime_value(const std::uint32_t value)
   {
      // Sloane's A029750 List of numbers of the form 2^k times 1, 3, 5 or 7.
      // CoefficientList[Series[-(x + 1)^2 (x^2 + 1)^2/(2 x^4 - 1), {x, 0, 78}], x]
      constexpr std::array<std::uint32_t, 65U> a029750_data =
      {{
         UINT32_C(        32), UINT32_C(     40), UINT32_C(     48), UINT32_C(     56),
         UINT32_C(        64), UINT32_C(     80), UINT32_C(     96), UINT32_C(    112),
         UINT32_C(       128), UINT32_C(    160), UINT32_C(    192), UINT32_C(    224),
         UINT32_C(       256), UINT32_C(    320), UINT32_C(    384), UINT32_C(    448),
         UINT32_C(       512), UINT32_C(    640), UINT32_C(    768), UINT32_C(    896),
         UINT32_C(      1024), UINT32_C(   1280), UINT32_C(   1536), UINT32_C(   1792),
         UINT32_C(      2048), UINT32_C(   2560), UINT32_C(   3072), UINT32_C(   3584),
         UINT32_C(      4096), UINT32_C(   5120), UINT32_C(   6144), UINT32_C(   7168),
         UINT32_C(      8192), UINT32_C(  10240), UINT32_C(  12288), UINT32_C(  14336),
         UINT32_C(     16384), UINT32_C(  20480), UINT32_C(  24576), UINT32_C(  28672),
         UINT32_C(     32768), UINT32_C(  40960), UINT32_C(  49152), UINT32_C(  57344),
         UINT32_C(     65536), UINT32_C(  81920), UINT32_C(  98304), UINT32_C( 114688),
         UINT32_C(    131072), UINT32_C( 163840), UINT32_C( 196608), UINT32_C( 229376),
         UINT32_C(    262144), UINT32_C( 327680), UINT32_C( 393216), UINT32_C( 458752),
         UINT32_C(    524288), UINT32_C( 655360), UINT32_C( 786432), UINT32_C( 917504),
         UINT32_C(   1048576), UINT32_C(1310720), UINT32_C(1572864), UINT32_C(1835008),
         UINT32_C(0x7FFFFFFF)
      }};

      const std::array<std::uint32_t, 65U>::const_iterator it =
         std::lower_bound(a029750_data.cbegin(), a029750_data.cend(), value);

      return ((it != a029750_data.cend()) ? *it : UINT32_C(0xFFFFFFFF));
   }
};

constexpr std::uint32_t pow10_maker(std::uint32_t n)
{
   // Make the constant power of 10^n.
   return ((n == UINT32_C(0)) ? UINT32_C(1) : pow10_maker(n - UINT32_C(1)) * UINT32_C(10));
}

}}}} // namespace boost::multiprecision::backends::detail

#endif // BOOST_MP_DETAIL_TABLES_HPP
