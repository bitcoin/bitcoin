// Copyright Frank Mori Hess 2007-2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_DETAIL_REPLACE_SLOT_FUNCTION_HPP
#define BOOST_SIGNALS2_DETAIL_REPLACE_SLOT_FUNCTION_HPP

#include <boost/signals2/slot_base.hpp>

namespace boost
{
  namespace signals2
  {
    namespace detail
    {
      template<typename ResultSlot, typename SlotIn, typename SlotFunction>
        ResultSlot replace_slot_function(const SlotIn &slot_in, const SlotFunction &fun)
      {
        ResultSlot slot(fun);
        slot.track(slot_in);
        return slot;
      }
    } // namespace detail
  } // namespace signals2
} // namespace boost

#endif // BOOST_SIGNALS2_DETAIL_REPLACE_SLOT_FUNCTION_HPP
