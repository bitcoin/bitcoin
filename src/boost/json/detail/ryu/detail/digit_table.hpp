// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

/*
    This is a derivative work
*/

#ifndef BOOST_JSON_DETAIL_RYU_DETAIL_DIGIT_TABLE_HPP
#define BOOST_JSON_DETAIL_RYU_DETAIL_DIGIT_TABLE_HPP

#include <boost/json/detail/config.hpp>

BOOST_JSON_NS_BEGIN
namespace detail {

namespace ryu {
namespace detail {

// A table of all two-digit numbers. This is used to speed up decimal digit
// generation by copying pairs of digits into the final output.
inline
char const
(&DIGIT_TABLE() noexcept)[200]
{
    static constexpr char arr[200] = {
      '0','0','0','1','0','2','0','3','0','4','0','5','0','6','0','7','0','8','0','9',
      '1','0','1','1','1','2','1','3','1','4','1','5','1','6','1','7','1','8','1','9',
      '2','0','2','1','2','2','2','3','2','4','2','5','2','6','2','7','2','8','2','9',
      '3','0','3','1','3','2','3','3','3','4','3','5','3','6','3','7','3','8','3','9',
      '4','0','4','1','4','2','4','3','4','4','4','5','4','6','4','7','4','8','4','9',
      '5','0','5','1','5','2','5','3','5','4','5','5','5','6','5','7','5','8','5','9',
      '6','0','6','1','6','2','6','3','6','4','6','5','6','6','6','7','6','8','6','9',
      '7','0','7','1','7','2','7','3','7','4','7','5','7','6','7','7','7','8','7','9',
      '8','0','8','1','8','2','8','3','8','4','8','5','8','6','8','7','8','8','8','9',
      '9','0','9','1','9','2','9','3','9','4','9','5','9','6','9','7','9','8','9','9' };
    return arr;
}

} // detail
} // ryu

} // detail
BOOST_JSON_NS_END

#endif
