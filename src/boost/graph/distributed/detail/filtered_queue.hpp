// Copyright (C) 2004-2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_FILTERED_QUEUE_HPP
#define BOOST_FILTERED_QUEUE_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <algorithm>

namespace boost {

/** Queue adaptor that filters elements pushed into the queue
 * according to some predicate.
 */
template<typename Buffer, typename Predicate>
class filtered_queue
{
 public:
  typedef Buffer                      buffer_type;
  typedef Predicate                   predicate_type;
  typedef typename Buffer::value_type value_type;
  typedef typename Buffer::size_type  size_type;

  /**
   * Constructs a new filtered queue with an initial buffer and a
   * predicate.
   *
   * @param buffer the initial buffer
   * @param pred the predicate
   */
  explicit
  filtered_queue(const buffer_type& buffer = buffer_type(),
                 const predicate_type& pred = predicate_type())
    : buffer(buffer), pred(pred) {}

  /** Push a value into the queue.
   *
   *  If the predicate returns @c true for @p x, pushes @p x into the
   *  buffer.
   */
  void push(const value_type& x)  { if (pred(x)) buffer.push(x); }

  /** Pop the front element off the buffer.
   *
   * @pre @c !empty()
   */
  void pop()                      { buffer.pop(); }

  /** Retrieve the front (top) element in the buffer.
   *
   * @pre @c !empty()
   */
  value_type& top()               { return buffer.top(); }

  /**
   * \overload
   */
  const value_type& top() const   { return buffer.top(); }

  /** Determine the number of elements in the buffer. */
  size_type size() const          { return buffer.size(); }

  /** Determine if the buffer is empty. */
  bool empty() const              { return buffer.empty(); }

  /** Get a reference to the underlying buffer. */
  buffer_type& base()             { return buffer; }
  const buffer_type& base() const { return buffer; }

  /** Swap the contents of this with @p other. */
  void swap(filtered_queue& other)
  {
    using std::swap;
    swap(buffer, other.buffer);
    swap(pred, other.pred);
  }

 private:
  buffer_type buffer;
  predicate_type pred;
};

/** Create a filtered queue. */
template<typename Buffer, typename Predicate>
inline filtered_queue<Buffer, Predicate>
make_filtered_queue(const Buffer& buffer, const Predicate& pred)
{ return filtered_queue<Buffer, Predicate>(buffer, pred); }

/** Swap a filtered_queue. */
template<typename Buffer, typename Predicate>
inline void
swap(filtered_queue<Buffer, Predicate>& x,
     filtered_queue<Buffer, Predicate>& y)
{
  x.swap(y);
}

} // end namespace boost

#endif // BOOST_FILTERED_QUEUE_HPP
