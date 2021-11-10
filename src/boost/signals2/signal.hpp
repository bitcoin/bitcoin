//  A thread-safe version of Boost.Signals.

// Copyright Frank Mori Hess 2007-2009
//
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SIGNAL_HPP
#define BOOST_SIGNALS2_SIGNAL_HPP

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
#include <boost/signals2/detail/replace_slot_function.hpp>
#include <boost/signals2/detail/result_type_wrapper.hpp>
#include <boost/signals2/detail/signals_common.hpp>
#include <boost/signals2/detail/signals_common_macros.hpp>
#include <boost/signals2/detail/slot_groups.hpp>
#include <boost/signals2/detail/slot_call_iterator.hpp>
#include <boost/signals2/optional_last_value.hpp>
#include <boost/signals2/mutex.hpp>
#include <boost/signals2/slot.hpp>
#include <functional>

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
#include <boost/signals2/preprocessed_signal.hpp>
#else
#include <boost/signals2/variadic_signal.hpp>
#endif

namespace boost
{
  namespace signals2
  {
    // free swap function, findable by ADL
    template<typename Signature,
      typename Combiner,
      typename Group,
      typename GroupCompare,
      typename SlotFunction,
      typename ExtendedSlotFunction,
      typename Mutex>
      void swap(
        signal<Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &sig1,
        signal<Signature, Combiner, Group, GroupCompare, SlotFunction, ExtendedSlotFunction, Mutex> &sig2) BOOST_NOEXCEPT
    {
      sig1.swap(sig2);
    }
  }
}

#endif // BOOST_SIGNALS2_SIGNAL_HPP
