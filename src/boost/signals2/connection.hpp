/*
  boost::signals2::connection provides a handle to a signal/slot connection.

  Author: Frank Mori Hess <fmhess@users.sourceforge.net>
  Begin: 2007-01-23
*/
// Copyright Frank Mori Hess 2007-2008.
// Distributed under the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org/libs/signals2 for library home page.

#ifndef BOOST_SIGNALS2_CONNECTION_HPP
#define BOOST_SIGNALS2_CONNECTION_HPP

#include <boost/config.hpp>
#include <boost/function.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/detail/auto_buffer.hpp>
#include <boost/signals2/detail/null_output_iterator.hpp>
#include <boost/signals2/detail/unique_lock.hpp>
#include <boost/signals2/slot.hpp>
#include <boost/weak_ptr.hpp>

namespace boost
{
  namespace signals2
  {
    inline void null_deleter(const void*) {}
    namespace detail
    {
      // This lock maintains a list of shared_ptr<void>
      // which will be destroyed only after the lock
      // has released its mutex.  Used to garbage
      // collect disconnected slots
      template<typename Mutex>
      class garbage_collecting_lock: public noncopyable
      {
      public:
        garbage_collecting_lock(Mutex &m):
          lock(m)
        {}
        void add_trash(const shared_ptr<void> &piece_of_trash)
        {
          garbage.push_back(piece_of_trash);
        }
      private:
        // garbage must be declared before lock
        // to insure it is destroyed after lock is
        // destroyed.
        auto_buffer<shared_ptr<void>, store_n_objects<10> > garbage;
        unique_lock<Mutex> lock;
      };
      
      class connection_body_base
      {
      public:
        connection_body_base():
          _connected(true), m_slot_refcount(1)
        {
        }
        virtual ~connection_body_base() {}
        void disconnect()
        {
          garbage_collecting_lock<connection_body_base> local_lock(*this);
          nolock_disconnect(local_lock);
        }
        template<typename Mutex>
        void nolock_disconnect(garbage_collecting_lock<Mutex> &lock_arg) const
        {
          if(_connected)
          {
            _connected = false;
            dec_slot_refcount(lock_arg);
          }
        }
        virtual bool connected() const = 0;
        shared_ptr<void> get_blocker()
        {
          unique_lock<connection_body_base> local_lock(*this);
          shared_ptr<void> blocker = _weak_blocker.lock();
          if(blocker == shared_ptr<void>())
          {
            blocker.reset(this, &null_deleter);
            _weak_blocker = blocker;
          }
          return blocker;
        }
        bool blocked() const
        {
          return !_weak_blocker.expired();
        }
        bool nolock_nograb_blocked() const
        {
          return nolock_nograb_connected() == false || blocked();
        }
        bool nolock_nograb_connected() const {return _connected;}
        // expose part of Lockable concept of mutex
        virtual void lock() = 0;
        virtual void unlock() = 0;

        // Slot refcount should be incremented while
        // a signal invocation is using the slot, in order
        // to prevent slot from being destroyed mid-invocation.
        // garbage_collecting_lock parameter enforces 
        // the existance of a lock before this
        // method is called
        template<typename Mutex>
        void inc_slot_refcount(const garbage_collecting_lock<Mutex> &)
        {
          BOOST_ASSERT(m_slot_refcount != 0);
          ++m_slot_refcount;
        }
        // if slot refcount decrements to zero due to this call, 
        // it puts a
        // shared_ptr to the slot in the garbage collecting lock,
        // which will destroy the slot only after it unlocks.
        template<typename Mutex>
        void dec_slot_refcount(garbage_collecting_lock<Mutex> &lock_arg) const
        {
          BOOST_ASSERT(m_slot_refcount != 0);
          if(--m_slot_refcount == 0)
          {
            lock_arg.add_trash(release_slot());
          }
        }

      protected:
        virtual shared_ptr<void> release_slot() const = 0;

        weak_ptr<void> _weak_blocker;
      private:
        mutable bool _connected;
        mutable unsigned m_slot_refcount;
      };

      template<typename GroupKey, typename SlotType, typename Mutex>
      class connection_body: public connection_body_base
      {
      public:
        typedef Mutex mutex_type;
        connection_body(const SlotType &slot_in, const boost::shared_ptr<mutex_type> &signal_mutex):
          m_slot(new SlotType(slot_in)), _mutex(signal_mutex)
        {
        }
        virtual ~connection_body() {}
        virtual bool connected() const
        {
          garbage_collecting_lock<mutex_type> local_lock(*_mutex);
          nolock_grab_tracked_objects(local_lock, detail::null_output_iterator());
          return nolock_nograb_connected();
        }
        const GroupKey& group_key() const {return _group_key;}
        void set_group_key(const GroupKey &key) {_group_key = key;}
        template<typename M>
        void disconnect_expired_slot(garbage_collecting_lock<M> &lock_arg)
        {
          if(!m_slot) return;
          bool expired = slot().expired();
          if(expired == true)
          {
            nolock_disconnect(lock_arg);
          }
        }
        template<typename M, typename OutputIterator>
        void nolock_grab_tracked_objects(garbage_collecting_lock<M> &lock_arg,
          OutputIterator inserter) const
        {
          if(!m_slot) return;
          slot_base::tracked_container_type::const_iterator it;
          for(it = slot().tracked_objects().begin();
            it != slot().tracked_objects().end();
            ++it)
          {
            void_shared_ptr_variant locked_object
            (
              apply_visitor
              (
                detail::lock_weak_ptr_visitor(),
                *it
              )
            );
            if(apply_visitor(detail::expired_weak_ptr_visitor(), *it))
            {
              nolock_disconnect(lock_arg);
              return;
            }
            *inserter++ = locked_object;
          }
        }
        // expose Lockable concept of mutex
        virtual void lock()
        {
          _mutex->lock();
        }
        virtual void unlock()
        {
          _mutex->unlock();
        }
        SlotType &slot()
        {
          return *m_slot;
        }
        const SlotType &slot() const
        {
          return *m_slot;
        }
      protected:
        virtual shared_ptr<void> release_slot() const
        {
          
          shared_ptr<void> released_slot = m_slot;
          m_slot.reset();
          return released_slot;
        }
      private:
        mutable boost::shared_ptr<SlotType> m_slot;
        const boost::shared_ptr<mutex_type> _mutex;
        GroupKey _group_key;
      };
    }

    class shared_connection_block;

    class connection
    {
    public:
      friend class shared_connection_block;

      connection() BOOST_NOEXCEPT {}
      connection(const connection &other) BOOST_NOEXCEPT: _weak_connection_body(other._weak_connection_body)
      {}
      connection(const boost::weak_ptr<detail::connection_body_base> &connectionBody) BOOST_NOEXCEPT:
        _weak_connection_body(connectionBody)
      {}
      
      // move support
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      connection(connection && other) BOOST_NOEXCEPT: _weak_connection_body(std::move(other._weak_connection_body))
      {
        // make sure other is reset, in case it is a scoped_connection (so it
        // won't disconnect on destruction after being moved away from).
        other._weak_connection_body.reset();
      }
      connection & operator=(connection && other) BOOST_NOEXCEPT
      {
        if(&other == this) return *this;
        _weak_connection_body = std::move(other._weak_connection_body);
        // make sure other is reset, in case it is a scoped_connection (so it
        // won't disconnect on destruction after being moved away from).
        other._weak_connection_body.reset();
        return *this;
      }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      connection & operator=(const connection & other) BOOST_NOEXCEPT
      {
        if(&other == this) return *this;
        _weak_connection_body = other._weak_connection_body;
        return *this;
      }

      ~connection() {}
      void disconnect() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return;
        connectionBody->disconnect();
      }
      bool connected() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return false;
        return connectionBody->connected();
      }
      bool blocked() const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        if(connectionBody == 0) return true;
        return connectionBody->blocked();
      }
      bool operator==(const connection& other) const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
        return connectionBody == otherConnectionBody;
      }
      bool operator!=(const connection& other) const
      {
        return !(*this == other);
      }
      bool operator<(const connection& other) const
      {
        boost::shared_ptr<detail::connection_body_base> connectionBody(_weak_connection_body.lock());
        boost::shared_ptr<detail::connection_body_base> otherConnectionBody(other._weak_connection_body.lock());
        return connectionBody < otherConnectionBody;
      }
      void swap(connection &other) BOOST_NOEXCEPT
      {
        using std::swap;
        swap(_weak_connection_body, other._weak_connection_body);
      }
    protected:

      boost::weak_ptr<detail::connection_body_base> _weak_connection_body;
    };
    inline void swap(connection &conn1, connection &conn2) BOOST_NOEXCEPT
    {
      conn1.swap(conn2);
    }

    class scoped_connection: public connection
    {
    public:
      scoped_connection() BOOST_NOEXCEPT {}
      scoped_connection(const connection &other) BOOST_NOEXCEPT:
        connection(other)
      {}
      ~scoped_connection()
      {
        disconnect();
      }
      scoped_connection& operator=(const connection &rhs) BOOST_NOEXCEPT
      {
        disconnect();
        connection::operator=(rhs);
        return *this;
      }

      // move support
#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      scoped_connection(scoped_connection && other) BOOST_NOEXCEPT: connection(std::move(other))
      {
      }
      scoped_connection(connection && other) BOOST_NOEXCEPT: connection(std::move(other))
      {
      }
      scoped_connection & operator=(scoped_connection && other) BOOST_NOEXCEPT
      {
        if(&other == this) return *this;
        disconnect();
        connection::operator=(std::move(other));
        return *this;
      }
      scoped_connection & operator=(connection && other) BOOST_NOEXCEPT
      {
        if(&other == this) return *this;
        disconnect();
        connection::operator=(std::move(other));
        return *this;
      }
#endif // !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

      connection release()
      {
        connection conn(_weak_connection_body);
        _weak_connection_body.reset();
        return conn;
      }
    private:
      scoped_connection(const scoped_connection &other);
      scoped_connection& operator=(const scoped_connection &rhs);
    };
    // Sun 5.9 compiler doesn't find the swap for base connection class when
    // arguments are scoped_connection, so we provide this explicitly.
    inline void swap(scoped_connection &conn1, scoped_connection &conn2) BOOST_NOEXCEPT
    {
      conn1.swap(conn2);
    }
  }
}

#endif  // BOOST_SIGNALS2_CONNECTION_HPP
