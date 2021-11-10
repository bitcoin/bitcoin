// (C) Copyright 2012 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_EXTERNALLY_LOCKED_STREAM_HPP
#define BOOST_THREAD_EXTERNALLY_LOCKED_STREAM_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/delete.hpp>

#include <boost/thread/externally_locked.hpp>
#include <boost/thread/lock_traits.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/strict_lock.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  template <typename Stream, typename RecursiveMutex=recursive_mutex>
  class externally_locked_stream;

  template <class Stream, typename RecursiveMutex=recursive_mutex>
  class stream_guard
  {

    friend class externally_locked_stream<Stream, RecursiveMutex> ;
  public:
    typedef typename externally_locked_stream<Stream, RecursiveMutex>::mutex_type mutex_type;

    BOOST_THREAD_MOVABLE_ONLY( stream_guard)

    stream_guard(externally_locked_stream<Stream, RecursiveMutex>& mtx) :
      mtx_(&mtx)
    {
      mtx.lock();
    }

    stream_guard(externally_locked_stream<Stream, RecursiveMutex>& mtx, adopt_lock_t) :
      mtx_(&mtx)
    {
    }

    stream_guard(BOOST_THREAD_RV_REF(stream_guard) rhs) BOOST_NOEXCEPT
    : mtx_(rhs.mtx_)
    {
      rhs.mtx_= 0;
    }

    ~stream_guard()
    {
      if (mtx_ != 0) mtx_->unlock();
    }

    bool owns_lock(const mutex_type * l) const BOOST_NOEXCEPT
    {
      return l == mtx_->mutex();
    }

    /**
     * @Requires mtx_
     */
    Stream& get() const
    {
      BOOST_THREAD_ASSERT_PRECONDITION(  mtx_, lock_error() );
      return mtx_->get(*this);
    }
    Stream& bypass() const
    {
      return get();
    }


  private:
    externally_locked_stream<Stream, RecursiveMutex>* mtx_;
  };

  template <typename Stream, typename RecursiveMutex>
  struct is_strict_lock_sur_parole<stream_guard<Stream, RecursiveMutex> > : true_type
  {
  };

  /**
   * externally_locked_stream cloaks a reference to an stream of type Stream, and actually
   * provides full access to that object through the get and set member functions, provided you
   * pass a reference to a strict lock object.
   */

  //[externally_locked_stream
  template <typename Stream, typename RecursiveMutex>
  class externally_locked_stream: public externally_locked<Stream&, RecursiveMutex>
  {
    typedef externally_locked<Stream&, RecursiveMutex> base_type;
  public:
    BOOST_THREAD_NO_COPYABLE( externally_locked_stream)

    /**
     * Effects: Constructs an externally locked object storing the cloaked reference object.
     */
    externally_locked_stream(Stream& stream, RecursiveMutex& mtx) BOOST_NOEXCEPT :
      base_type(stream, mtx)
    {
    }

    stream_guard<Stream, RecursiveMutex> hold() BOOST_NOEXCEPT
    {
      return stream_guard<Stream, RecursiveMutex> (*this);
    }
    Stream& bypass() const
    {
      stream_guard<Stream, RecursiveMutex> lk(*this);
      return get(lk);
    }
  };
  //]

  template <typename Stream, typename RecursiveMutex, typename T>
  inline const stream_guard<Stream, RecursiveMutex>& operator<<(const stream_guard<Stream, RecursiveMutex>& lck, T arg)
  {
    lck.get() << arg;
    return lck;
  }

  template <typename Stream, typename RecursiveMutex>
  inline const stream_guard<Stream, RecursiveMutex>& operator<<(const stream_guard<Stream, RecursiveMutex>& lck, Stream& (*arg)(Stream&))
  {
    lck.get() << arg;
    return lck;
  }

  template <typename Stream, typename RecursiveMutex, typename T>
  inline const stream_guard<Stream, RecursiveMutex>& operator>>(const stream_guard<Stream, RecursiveMutex>& lck, T& arg)
  {
    lck.get() >> arg;
    return lck;
  }

  template <typename Stream, typename RecursiveMutex, typename T>
  inline stream_guard<Stream, RecursiveMutex> operator<<(externally_locked_stream<Stream, RecursiveMutex>& mtx, T arg)
  {
    stream_guard<Stream, RecursiveMutex> lk(mtx);
    mtx.get(lk) << arg;
    return boost::move(lk);
  }

  template <typename Stream, typename RecursiveMutex>
  inline stream_guard<Stream, RecursiveMutex> operator<<(externally_locked_stream<Stream, RecursiveMutex>& mtx, Stream& (*arg)(Stream&))
  {
    stream_guard<Stream, RecursiveMutex> lk(mtx);
    mtx.get(lk) << arg;
    return boost::move(lk);
  }

  template <typename Stream, typename RecursiveMutex, typename T>
  inline stream_guard<Stream, RecursiveMutex> operator>>(externally_locked_stream<Stream, RecursiveMutex>& mtx, T& arg)
  {
    stream_guard<Stream, RecursiveMutex> lk(mtx);
    mtx.get(lk) >> arg;
    return boost::move(lk);
  }

}

#include <boost/config/abi_suffix.hpp>

#endif // header
