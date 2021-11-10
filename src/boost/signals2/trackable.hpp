// Boost.Signals2 library

// Copyright Frank Mori Hess 2007,2009.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Compatibility class to ease porting from the original
// Boost.Signals library.  However,
// boost::signals2::trackable is NOT thread-safe.

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_TRACKABLE_HPP
#define BOOST_SIGNALS2_TRACKABLE_HPP

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace boost {
  namespace signals2 {
    namespace detail
    {
        class tracked_objects_visitor;
        
        // trackable_pointee is used to identify the tracked shared_ptr 
        // originating from the signals2::trackable class.  These tracked
        // shared_ptr are special in that we shouldn't bother to
        // increment their use count during signal invocation, since
        // they don't actually control the lifetime of the
        // signals2::trackable object they are associated with.
        class trackable_pointee
        {};
    }
    class trackable {
    protected:
      trackable(): _tracked_ptr(static_cast<detail::trackable_pointee*>(0)) {}
      trackable(const trackable &): _tracked_ptr(static_cast<detail::trackable_pointee*>(0)) {}
      trackable& operator=(const trackable &)
      {
          return *this;
      }
      ~trackable() {}
    private:
      friend class detail::tracked_objects_visitor;
      weak_ptr<detail::trackable_pointee> get_weak_ptr() const
      {
          return _tracked_ptr;
      }

      shared_ptr<detail::trackable_pointee> _tracked_ptr;
    };
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_TRACKABLE_HPP
