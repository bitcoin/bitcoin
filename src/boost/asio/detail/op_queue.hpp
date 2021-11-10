//
// detail/op_queue.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_OP_QUEUE_HPP
#define BOOST_ASIO_DETAIL_OP_QUEUE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/noncopyable.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Operation>
class op_queue;

class op_queue_access
{
public:
  template <typename Operation>
  static Operation* next(Operation* o)
  {
    return static_cast<Operation*>(o->next_);
  }

  template <typename Operation1, typename Operation2>
  static void next(Operation1*& o1, Operation2* o2)
  {
    o1->next_ = o2;
  }

  template <typename Operation>
  static void destroy(Operation* o)
  {
    o->destroy();
  }

  template <typename Operation>
  static Operation*& front(op_queue<Operation>& q)
  {
    return q.front_;
  }

  template <typename Operation>
  static Operation*& back(op_queue<Operation>& q)
  {
    return q.back_;
  }
};

template <typename Operation>
class op_queue
  : private noncopyable
{
public:
  // Constructor.
  op_queue()
    : front_(0),
      back_(0)
  {
  }

  // Destructor destroys all operations.
  ~op_queue()
  {
    while (Operation* op = front_)
    {
      pop();
      op_queue_access::destroy(op);
    }
  }

  // Get the operation at the front of the queue.
  Operation* front()
  {
    return front_;
  }

  // Pop an operation from the front of the queue.
  void pop()
  {
    if (front_)
    {
      Operation* tmp = front_;
      front_ = op_queue_access::next(front_);
      if (front_ == 0)
        back_ = 0;
      op_queue_access::next(tmp, static_cast<Operation*>(0));
    }
  }

  // Push an operation on to the back of the queue.
  void push(Operation* h)
  {
    op_queue_access::next(h, static_cast<Operation*>(0));
    if (back_)
    {
      op_queue_access::next(back_, h);
      back_ = h;
    }
    else
    {
      front_ = back_ = h;
    }
  }

  // Push all operations from another queue on to the back of the queue. The
  // source queue may contain operations of a derived type.
  template <typename OtherOperation>
  void push(op_queue<OtherOperation>& q)
  {
    if (Operation* other_front = op_queue_access::front(q))
    {
      if (back_)
        op_queue_access::next(back_, other_front);
      else
        front_ = other_front;
      back_ = op_queue_access::back(q);
      op_queue_access::front(q) = 0;
      op_queue_access::back(q) = 0;
    }
  }

  // Whether the queue is empty.
  bool empty() const
  {
    return front_ == 0;
  }

  // Test whether an operation is already enqueued.
  bool is_enqueued(Operation* o) const
  {
    return op_queue_access::next(o) != 0 || back_ == o;
  }

private:
  friend class op_queue_access;

  // The front of the queue.
  Operation* front_;

  // The back of the queue.
  Operation* back_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_OP_QUEUE_HPP
