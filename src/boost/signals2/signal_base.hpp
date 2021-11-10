// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SIGNAL_BASE_HPP
#define BOOST_SIGNALS2_SIGNAL_BASE_HPP

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace boost {
  namespace signals2 {
    class slot_base;

    class signal_base : public noncopyable
    {
    public:
      friend class slot_base;

      virtual ~signal_base() {}
    protected:
      virtual shared_ptr<void> lock_pimpl() const = 0;
    };
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_SIGNAL_BASE_HPP
