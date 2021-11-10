// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_LOCK_ALGORITHMS_HPP
#define BOOST_THREAD_LOCK_ALGORITHMS_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/lockable_traits.hpp>

#include <algorithm>
#include <iterator>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail
  {
    template <typename MutexType1, typename MutexType2>
    unsigned try_lock_internal(MutexType1& m1, MutexType2& m2)
    {
      boost::unique_lock<MutexType1> l1(m1, boost::try_to_lock);
      if (!l1)
      {
        return 1;
      }
      if (!m2.try_lock())
      {
        return 2;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3>
    unsigned try_lock_internal(MutexType1& m1, MutexType2& m2, MutexType3& m3)
    {
      boost::unique_lock<MutexType1> l1(m1, boost::try_to_lock);
      if (!l1)
      {
        return 1;
      }
      if (unsigned const failed_lock=try_lock_internal(m2,m3))
      {
        return failed_lock + 1;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4>
    unsigned try_lock_internal(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4)
    {
      boost::unique_lock<MutexType1> l1(m1, boost::try_to_lock);
      if (!l1)
      {
        return 1;
      }
      if (unsigned const failed_lock=try_lock_internal(m2,m3,m4))
      {
        return failed_lock + 1;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4, typename MutexType5>
    unsigned try_lock_internal(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4, MutexType5& m5)
    {
      boost::unique_lock<MutexType1> l1(m1, boost::try_to_lock);
      if (!l1)
      {
        return 1;
      }
      if (unsigned const failed_lock=try_lock_internal(m2,m3,m4,m5))
      {
        return failed_lock + 1;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2>
    unsigned lock_helper(MutexType1& m1, MutexType2& m2)
    {
      boost::unique_lock<MutexType1> l1(m1);
      if (!m2.try_lock())
      {
        return 1;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3>
    unsigned lock_helper(MutexType1& m1, MutexType2& m2, MutexType3& m3)
    {
      boost::unique_lock<MutexType1> l1(m1);
      if (unsigned const failed_lock=try_lock_internal(m2,m3))
      {
        return failed_lock;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4>
    unsigned lock_helper(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4)
    {
      boost::unique_lock<MutexType1> l1(m1);
      if (unsigned const failed_lock=try_lock_internal(m2,m3,m4))
      {
        return failed_lock;
      }
      l1.release();
      return 0;
    }

    template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4, typename MutexType5>
    unsigned lock_helper(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4, MutexType5& m5)
    {
      boost::unique_lock<MutexType1> l1(m1);
      if (unsigned const failed_lock=try_lock_internal(m2,m3,m4,m5))
      {
        return failed_lock;
      }
      l1.release();
      return 0;
    }
  }

  namespace detail
  {
    template <bool x>
    struct is_mutex_type_wrapper
    {
    };

    template <typename MutexType1, typename MutexType2>
    void lock_impl(MutexType1& m1, MutexType2& m2, is_mutex_type_wrapper<true> )
    {
      unsigned const lock_count = 2;
      unsigned lock_first = 0;
      for (;;)
      {
        switch (lock_first)
        {
        case 0:
          lock_first = detail::lock_helper(m1, m2);
          if (!lock_first) return;
          break;
        case 1:
          lock_first = detail::lock_helper(m2, m1);
          if (!lock_first) return;
          lock_first = (lock_first + 1) % lock_count;
          break;
        }
      }
    }

    template <typename Iterator>
    void lock_impl(Iterator begin, Iterator end, is_mutex_type_wrapper<false> );
  }

  template <typename MutexType1, typename MutexType2>
  void lock(MutexType1& m1, MutexType2& m2)
  {
    detail::lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  void lock(const MutexType1& m1, MutexType2& m2)
  {
    detail::lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  void lock(MutexType1& m1, const MutexType2& m2)
  {
    detail::lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  void lock(const MutexType1& m1, const MutexType2& m2)
  {
    detail::lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3>
  void lock(MutexType1& m1, MutexType2& m2, MutexType3& m3)
  {
    unsigned const lock_count = 3;
    unsigned lock_first = 0;
    for (;;)
    {
      switch (lock_first)
      {
      case 0:
        lock_first = detail::lock_helper(m1, m2, m3);
        if (!lock_first) return;
        break;
      case 1:
        lock_first = detail::lock_helper(m2, m3, m1);
        if (!lock_first) return;
        lock_first = (lock_first + 1) % lock_count;
        break;
      case 2:
        lock_first = detail::lock_helper(m3, m1, m2);
        if (!lock_first) return;
        lock_first = (lock_first + 2) % lock_count;
        break;
      }
    }
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4>
  void lock(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4)
  {
    unsigned const lock_count = 4;
    unsigned lock_first = 0;
    for (;;)
    {
      switch (lock_first)
      {
      case 0:
        lock_first = detail::lock_helper(m1, m2, m3, m4);
        if (!lock_first) return;
        break;
      case 1:
        lock_first = detail::lock_helper(m2, m3, m4, m1);
        if (!lock_first) return;
        lock_first = (lock_first + 1) % lock_count;
        break;
      case 2:
        lock_first = detail::lock_helper(m3, m4, m1, m2);
        if (!lock_first) return;
        lock_first = (lock_first + 2) % lock_count;
        break;
      case 3:
        lock_first = detail::lock_helper(m4, m1, m2, m3);
        if (!lock_first) return;
        lock_first = (lock_first + 3) % lock_count;
        break;
      }
    }
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4, typename MutexType5>
  void lock(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4, MutexType5& m5)
  {
    unsigned const lock_count = 5;
    unsigned lock_first = 0;
    for (;;)
    {
      switch (lock_first)
      {
      case 0:
        lock_first = detail::lock_helper(m1, m2, m3, m4, m5);
        if (!lock_first) return;
        break;
      case 1:
        lock_first = detail::lock_helper(m2, m3, m4, m5, m1);
        if (!lock_first) return;
        lock_first = (lock_first + 1) % lock_count;
        break;
      case 2:
        lock_first = detail::lock_helper(m3, m4, m5, m1, m2);
        if (!lock_first) return;
        lock_first = (lock_first + 2) % lock_count;
        break;
      case 3:
        lock_first = detail::lock_helper(m4, m5, m1, m2, m3);
        if (!lock_first) return;
        lock_first = (lock_first + 3) % lock_count;
        break;
      case 4:
        lock_first = detail::lock_helper(m5, m1, m2, m3, m4);
        if (!lock_first) return;
        lock_first = (lock_first + 4) % lock_count;
        break;
      }
    }
  }

  namespace detail
  {
    template <typename Mutex, bool x = is_mutex_type<Mutex>::value>
    struct try_lock_impl_return
    {
      typedef int type;
    };

    template <typename Iterator>
    struct try_lock_impl_return<Iterator, false>
    {
      typedef Iterator type;
    };

    template <typename MutexType1, typename MutexType2>
    int try_lock_impl(MutexType1& m1, MutexType2& m2, is_mutex_type_wrapper<true> )
    {
      return ((int) detail::try_lock_internal(m1, m2)) - 1;
    }

    template <typename Iterator>
    Iterator try_lock_impl(Iterator begin, Iterator end, is_mutex_type_wrapper<false> );
  }

  template <typename MutexType1, typename MutexType2>
  typename detail::try_lock_impl_return<MutexType1>::type try_lock(MutexType1& m1, MutexType2& m2)
  {
    return detail::try_lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  typename detail::try_lock_impl_return<MutexType1>::type try_lock(const MutexType1& m1, MutexType2& m2)
  {
    return detail::try_lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  typename detail::try_lock_impl_return<MutexType1>::type try_lock(MutexType1& m1, const MutexType2& m2)
  {
    return detail::try_lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2>
  typename detail::try_lock_impl_return<MutexType1>::type try_lock(const MutexType1& m1, const MutexType2& m2)
  {
    return detail::try_lock_impl(m1, m2, detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3>
  int try_lock(MutexType1& m1, MutexType2& m2, MutexType3& m3)
  {
    return ((int) detail::try_lock_internal(m1, m2, m3)) - 1;
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4>
  int try_lock(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4)
  {
    return ((int) detail::try_lock_internal(m1, m2, m3, m4)) - 1;
  }

  template <typename MutexType1, typename MutexType2, typename MutexType3, typename MutexType4, typename MutexType5>
  int try_lock(MutexType1& m1, MutexType2& m2, MutexType3& m3, MutexType4& m4, MutexType5& m5)
  {
    return ((int) detail::try_lock_internal(m1, m2, m3, m4, m5)) - 1;
  }

  namespace detail
  {
    template <typename Iterator>
    struct range_lock_guard
    {
      Iterator begin;
      Iterator end;

      range_lock_guard(Iterator begin_, Iterator end_) :
        begin(begin_), end(end_)
      {
        boost::lock(begin, end);
      }

      void release()
      {
        begin = end;
      }

      ~range_lock_guard()
      {
        for (; begin != end; ++begin)
        {
          begin->unlock();
        }
      }
    };

    template <typename Iterator>
    Iterator try_lock_impl(Iterator begin, Iterator end, is_mutex_type_wrapper<false> )

    {
      if (begin == end)
      {
        return end;
      }
      typedef typename std::iterator_traits<Iterator>::value_type lock_type;
      unique_lock<lock_type> guard(*begin, try_to_lock);

      if (!guard.owns_lock())
      {
        return begin;
      }
      Iterator const failed = boost::try_lock(++begin, end);
      if (failed == end)
      {
        guard.release();
      }

      return failed;
    }
  }

  namespace detail
  {
    template <typename Iterator>
    void lock_impl(Iterator begin, Iterator end, is_mutex_type_wrapper<false> )
    {
      typedef typename std::iterator_traits<Iterator>::value_type lock_type;

      if (begin == end)
      {
        return;
      }
      bool start_with_begin = true;
      Iterator second = begin;
      ++second;
      Iterator next = second;

      for (;;)
      {
        unique_lock<lock_type> begin_lock(*begin, defer_lock);
        if (start_with_begin)
        {
          begin_lock.lock();
          Iterator const failed_lock = boost::try_lock(next, end);
          if (failed_lock == end)
          {
            begin_lock.release();
            return;
          }
          start_with_begin = false;
          next = failed_lock;
        }
        else
        {
          detail::range_lock_guard<Iterator> guard(next, end);
          if (begin_lock.try_lock())
          {
            Iterator const failed_lock = boost::try_lock(second, next);
            if (failed_lock == next)
            {
              begin_lock.release();
              guard.release();
              return;
            }
            start_with_begin = false;
            next = failed_lock;
          }
          else
          {
            start_with_begin = true;
            next = second;
          }
        }
      }
    }

  }

}
#include <boost/config/abi_suffix.hpp>

#endif
