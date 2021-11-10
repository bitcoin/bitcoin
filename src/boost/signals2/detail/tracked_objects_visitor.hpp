// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Copyright Timmo Stange 2007.
// Copyright Douglas Gregor 2001-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_TRACKED_OBJECTS_VISITOR_HPP
#define BOOST_SIGNALS2_TRACKED_OBJECTS_VISITOR_HPP

#include <boost/mpl/bool.hpp>
#include <boost/ref.hpp>
#include <boost/signals2/detail/signals_common.hpp>
#include <boost/signals2/slot_base.hpp>
#include <boost/signals2/trackable.hpp>
#include <boost/type_traits/is_function.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/addressof.hpp>

namespace boost
{
  namespace signals2
  {
    namespace detail
    {
      // Visitor to collect tracked objects from a bound function.
      class tracked_objects_visitor
      {
      public:
        tracked_objects_visitor(slot_base *slot) : slot_(slot)
        {}
        template<typename T>
        void operator()(const T& t) const
        {
            m_visit_reference_wrapper(t, mpl::bool_<is_reference_wrapper<T>::value>());
        }
      private:
        template<typename T>
        void m_visit_reference_wrapper(const reference_wrapper<T> &t, const mpl::bool_<true> &) const
        {
            m_visit_pointer(t.get_pointer(), mpl::bool_<true>());
        }
        template<typename T>
        void m_visit_reference_wrapper(const T &t, const mpl::bool_<false> &) const
        {
            m_visit_pointer(t, mpl::bool_<is_pointer<T>::value>());
        }
        template<typename T>
        void m_visit_pointer(const T &t, const mpl::bool_<true> &) const
        {
            m_visit_not_function_pointer(t, mpl::bool_<!is_function<typename remove_pointer<T>::type>::value>());
        }
        template<typename T>
        void m_visit_pointer(const T &t, const mpl::bool_<false> &) const
        {
            m_visit_pointer(boost::addressof(t), mpl::bool_<true>());
        }
        template<typename T>
        void m_visit_not_function_pointer(const T *t, const mpl::bool_<true> &) const
        {
            m_visit_signal(t, mpl::bool_<is_signal<T>::value>());
        }
        template<typename T>
        void m_visit_not_function_pointer(const T &, const mpl::bool_<false> &) const
        {}
        template<typename T>
        void m_visit_signal(const T *signal, const mpl::bool_<true> &) const
        {
          if(signal)
            slot_->track_signal(*signal);
        }
        template<typename T>
        void m_visit_signal(const T &t, const mpl::bool_<false> &) const
        {
            add_if_trackable(t);
        }
        void add_if_trackable(const trackable *trackable) const
        {
          if(trackable)
            slot_->_tracked_objects.push_back(trackable->get_weak_ptr());
        }
        void add_if_trackable(const void *) const {}

        mutable slot_base * slot_;
      };


    } // end namespace detail
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_TRACKED_OBJECTS_VISITOR_HPP

