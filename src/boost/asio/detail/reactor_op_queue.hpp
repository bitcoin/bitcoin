//
// detail/reactor_op_queue.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_REACTOR_OP_QUEUE_HPP
#define BOOST_ASIO_DETAIL_REACTOR_OP_QUEUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/hash_map.hpp>
#include <boost/asio/detail/noncopyable.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Descriptor>
class reactor_op_queue
  : private noncopyable
{
public:
  typedef Descriptor key_type;

  struct mapped_type : op_queue<reactor_op>
  {
    mapped_type() {}
    mapped_type(const mapped_type&) {}
    void operator=(const mapped_type&) {}
  };

  typedef typename hash_map<key_type, mapped_type>::value_type value_type;
  typedef typename hash_map<key_type, mapped_type>::iterator iterator;

  // Constructor.
  reactor_op_queue()
    : operations_()
  {
  }

  // Obtain iterators to all registered descriptors.
  iterator begin() { return operations_.begin(); }
  iterator end() { return operations_.end(); }

  // Add a new operation to the queue. Returns true if this is the only
  // operation for the given descriptor, in which case the reactor's event
  // demultiplexing function call may need to be interrupted and restarted.
  bool enqueue_operation(Descriptor descriptor, reactor_op* op)
  {
    std::pair<iterator, bool> entry =
      operations_.insert(value_type(descriptor, mapped_type()));
    entry.first->second.push(op);
    return entry.second;
  }

  // Cancel all operations associated with the descriptor identified by the
  // supplied iterator. Any operations pending for the descriptor will be
  // cancelled. Returns true if any operations were cancelled, in which case
  // the reactor's event demultiplexing function may need to be interrupted and
  // restarted.
  bool cancel_operations(iterator i, op_queue<operation>& ops,
      const boost::system::error_code& ec =
        boost::asio::error::operation_aborted)
  {
    if (i != operations_.end())
    {
      while (reactor_op* op = i->second.front())
      {
        op->ec_ = ec;
        i->second.pop();
        ops.push(op);
      }
      operations_.erase(i);
      return true;
    }

    return false;
  }

  // Cancel all operations associated with the descriptor. Any operations
  // pending for the descriptor will be cancelled. Returns true if any
  // operations were cancelled, in which case the reactor's event
  // demultiplexing function may need to be interrupted and restarted.
  bool cancel_operations(Descriptor descriptor, op_queue<operation>& ops,
      const boost::system::error_code& ec =
        boost::asio::error::operation_aborted)
  {
    return this->cancel_operations(operations_.find(descriptor), ops, ec);
  }

  // Cancel operations associated with the descriptor identified by the
  // supplied iterator, and the specified cancellation key. Any operations
  // pending for the descriptor with the key will be cancelled. Returns true if
  // any operations were cancelled, in which case the reactor's event
  // demultiplexing function may need to be interrupted and restarted.
  bool cancel_operations_by_key(iterator i, op_queue<operation>& ops,
      void* cancellation_key, const boost::system::error_code& ec =
        boost::asio::error::operation_aborted)
  {
    bool result = false;
    if (i != operations_.end())
    {
      op_queue<reactor_op> other_ops;
      while (reactor_op* op = i->second.front())
      {
        i->second.pop();
        if (op->cancellation_key_ == cancellation_key)
        {
          op->ec_ = ec;
          ops.push(op);
          result = true;
        }
        else
          other_ops.push(op);
      }
      i->second.push(other_ops);
      if (i->second.empty())
        operations_.erase(i);
    }
    return result;
  }

  // Cancel all operations associated with the descriptor. Any operations
  // pending for the descriptor will be cancelled. Returns true if any
  // operations were cancelled, in which case the reactor's event
  // demultiplexing function may need to be interrupted and restarted.
  bool cancel_operations_by_key(Descriptor descriptor, op_queue<operation>& ops,
      void* cancellation_key, const boost::system::error_code& ec =
        boost::asio::error::operation_aborted)
  {
    return this->cancel_operations_by_key(
        operations_.find(descriptor), ops, cancellation_key, ec);
  }

  // Whether there are no operations in the queue.
  bool empty() const
  {
    return operations_.empty();
  }

  // Determine whether there are any operations associated with the descriptor.
  bool has_operation(Descriptor descriptor) const
  {
    return operations_.find(descriptor) != operations_.end();
  }

  // Perform the operations corresponding to the descriptor identified by the
  // supplied iterator. Returns true if there are still unfinished operations
  // queued for the descriptor.
  bool perform_operations(iterator i, op_queue<operation>& ops)
  {
    if (i != operations_.end())
    {
      while (reactor_op* op = i->second.front())
      {
        if (op->perform())
        {
          i->second.pop();
          ops.push(op);
        }
        else
        {
          return true;
        }
      }
      operations_.erase(i);
    }
    return false;
  }

  // Perform the operations corresponding to the descriptor. Returns true if
  // there are still unfinished operations queued for the descriptor.
  bool perform_operations(Descriptor descriptor, op_queue<operation>& ops)
  {
    return this->perform_operations(operations_.find(descriptor), ops);
  }

  // Get all operations owned by the queue.
  void get_all_operations(op_queue<operation>& ops)
  {
    iterator i = operations_.begin();
    while (i != operations_.end())
    {
      iterator op_iter = i++;
      ops.push(op_iter->second);
      operations_.erase(op_iter);
    }
  }

private:
  // The operations that are currently executing asynchronously.
  hash_map<key_type, mapped_type> operations_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_REACTOR_OP_QUEUE_HPP
