// Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file nonblocking.hpp
 *
 *  This header defines operations for completing non-blocking
 *  communication requests.
 */
#ifndef BOOST_MPI_NONBLOCKING_HPP
#define BOOST_MPI_NONBLOCKING_HPP

#include <boost/mpi/config.hpp>
#include <vector>
#include <iterator> // for std::iterator_traits
#include <boost/optional.hpp>
#include <utility> // for std::pair
#include <algorithm> // for iter_swap, reverse
#include <boost/static_assert.hpp>
#include <boost/mpi/request.hpp>
#include <boost/mpi/status.hpp>
#include <boost/mpi/exception.hpp>

namespace boost { namespace mpi {

/** 
 *  @brief Wait until any non-blocking request has completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and waits until any of these requests has
 *  been completed. It provides functionality equivalent to 
 *  @c MPI_Waitany.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. This may not be equal to @c first.
 *
 *  @returns A pair containing the status object that corresponds to
 *  the completed operation and the iterator referencing the completed
 *  request.
 */
template<typename ForwardIterator>
std::pair<status, ForwardIterator> 
wait_any(ForwardIterator first, ForwardIterator last)
{
  using std::advance;

  BOOST_ASSERT(first != last);
  
  typedef typename std::iterator_traits<ForwardIterator>::difference_type
    difference_type;

  bool all_trivial_requests = true;
  difference_type n = 0;
  ForwardIterator current = first;
  while (true) {
    // Check if we have found a completed request. If so, return it.
    if (current->active()) {
      optional<status> result = current->test();
      if (bool(result)) {
        return std::make_pair(*result, current);
      }
    }
    
    // Check if this request (and all others before it) are "trivial"
    // requests, e.g., they can be represented with a single
    // MPI_Request.
    // We could probably ignore non trivial request that are inactive,
    // but we can assume that a mix of trivial and non trivial requests
    // is unlikely enough not to care.
    all_trivial_requests = all_trivial_requests && current->trivial();

    // Move to the next request.
    ++n;
    if (++current == last) {
      // We have reached the end of the list. If all requests thus far
      // have been trivial, we can call MPI_Waitany directly, because
      // it may be more efficient than our busy-wait semantics.
      if (all_trivial_requests) {
        std::vector<MPI_Request> requests;
        requests.reserve(n);
        for (current = first; current != last; ++current) {
          requests.push_back(*current->trivial());
        }

        // Let MPI wait until one of these operations completes.
        int index;
        status stat;
        BOOST_MPI_CHECK_RESULT(MPI_Waitany, 
                               (n, detail::c_data(requests), &index, &stat.m_status));

        // We don't have a notion of empty requests or status objects,
        // so this is an error.
        if (index == MPI_UNDEFINED)
          boost::throw_exception(exception("MPI_Waitany", MPI_ERR_REQUEST));

        // Find the iterator corresponding to the completed request.
        current = first;
        advance(current, index);
        *current->trivial() = requests[index];
        return std::make_pair(stat, current);
      }

      // There are some nontrivial requests, so we must continue our
      // busy waiting loop.
      n = 0;
      current = first;
      all_trivial_requests = true;
    }
  }

  // We cannot ever get here
  BOOST_ASSERT(false);
}

/** 
 *  @brief Test whether any non-blocking request has completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and tests whether any of these requests has
 *  been completed. This routine is similar to @c wait_any, but will
 *  not block waiting for requests to completed. It provides
 *  functionality equivalent to @c MPI_Testany.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. 
 *
 *  @returns If any outstanding requests have completed, a pair
 *  containing the status object that corresponds to the completed
 *  operation and the iterator referencing the completed
 *  request. Otherwise, an empty @c optional<>.
 */
template<typename ForwardIterator>
optional<std::pair<status, ForwardIterator> >
test_any(ForwardIterator first, ForwardIterator last)
{
  while (first != last) {
    // Check if we have found a completed request. If so, return it.
    if (optional<status> result = first->test()) {
      return std::make_pair(*result, first);
    }
    ++first;
  }

  // We found nothing
  return optional<std::pair<status, ForwardIterator> >();
}

/** 
 *  @brief Wait until all non-blocking requests have completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and waits until all of these requests have
 *  been completed. It provides functionality equivalent to 
 *  @c MPI_Waitall.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. 
 *
 *  @param out If provided, an output iterator through which the
 *  status of each request will be emitted. The @c status objects are
 *  emitted in the same order as the requests are retrieved from 
 *  @c [first,last).
 *
 *  @returns If an @p out parameter was provided, the value @c out
 *  after all of the @c status objects have been emitted.
 */
template<typename ForwardIterator, typename OutputIterator>
OutputIterator 
wait_all(ForwardIterator first, ForwardIterator last, OutputIterator out)
{
  typedef typename std::iterator_traits<ForwardIterator>::difference_type
    difference_type;

  using std::distance;

  difference_type num_outstanding_requests = distance(first, last);

  std::vector<status> results(num_outstanding_requests);
  std::vector<bool> completed(num_outstanding_requests);

  while (num_outstanding_requests > 0) {
    bool all_trivial_requests = true;
    difference_type idx = 0;
    for (ForwardIterator current = first; current != last; ++current, ++idx) {
      if (!completed[idx]) {
        if (!current->active()) {
          completed[idx] = true;
          --num_outstanding_requests;
        } else if (optional<status> stat = current->test()) {
          // This outstanding request has been completed. We're done.
          results[idx] = *stat;
          completed[idx] = true;
          --num_outstanding_requests;
          all_trivial_requests = false;
        } else {
          // Check if this request (and all others before it) are "trivial"
          // requests, e.g., they can be represented with a single
          // MPI_Request.
          all_trivial_requests = all_trivial_requests && current->trivial();
        }
      }
    }

    // If we have yet to fulfill any requests and all of the requests
    // are trivial (i.e., require only a single MPI_Request to be
    // fulfilled), call MPI_Waitall directly.
    if (all_trivial_requests 
        && num_outstanding_requests == (difference_type)results.size()) {
      std::vector<MPI_Request> requests;
      requests.reserve(num_outstanding_requests);
      for (ForwardIterator current = first; current != last; ++current)
        requests.push_back(*current->trivial());

      // Let MPI wait until all of these operations completes.
      std::vector<MPI_Status> stats(num_outstanding_requests);
      BOOST_MPI_CHECK_RESULT(MPI_Waitall, 
                             (num_outstanding_requests, detail::c_data(requests), 
                              detail::c_data(stats)));

      for (std::vector<MPI_Status>::iterator i = stats.begin(); 
           i != stats.end(); ++i, ++out) {
        status stat;
        stat.m_status = *i;
        *out = stat;
      }

      return out;
    }

    all_trivial_requests = false;
  }

  return std::copy(results.begin(), results.end(), out);
}

/**
 * \overload
 */
template<typename ForwardIterator>
void
wait_all(ForwardIterator first, ForwardIterator last)
{
  typedef typename std::iterator_traits<ForwardIterator>::difference_type
    difference_type;

  using std::distance;

  difference_type num_outstanding_requests = distance(first, last);

  std::vector<bool> completed(num_outstanding_requests, false);

  while (num_outstanding_requests > 0) {
    bool all_trivial_requests = true;

    difference_type idx = 0;
    for (ForwardIterator current = first; current != last; ++current, ++idx) {
      if (!completed[idx]) {
        if (!current->active()) {
          completed[idx] = true;
          --num_outstanding_requests;
        } else if (optional<status> stat = current->test()) {
          // This outstanding request has been completed.
          completed[idx] = true;
          --num_outstanding_requests;
          all_trivial_requests = false;
        } else {
          // Check if this request (and all others before it) are "trivial"
          // requests, e.g., they can be represented with a single
          // MPI_Request.
          all_trivial_requests = all_trivial_requests && current->trivial();
        }
      }
    }

    // If we have yet to fulfill any requests and all of the requests
    // are trivial (i.e., require only a single MPI_Request to be
    // fulfilled), call MPI_Waitall directly.
    if (all_trivial_requests 
        && num_outstanding_requests == (difference_type)completed.size()) {
      std::vector<MPI_Request> requests;
      requests.reserve(num_outstanding_requests);
      for (ForwardIterator current = first; current != last; ++current)
        requests.push_back(*current->trivial());

      // Let MPI wait until all of these operations completes.
      BOOST_MPI_CHECK_RESULT(MPI_Waitall, 
                             (num_outstanding_requests, detail::c_data(requests), 
                              MPI_STATUSES_IGNORE));

      // Signal completion
      num_outstanding_requests = 0;
    }
  }
}

/** 
 *  @brief Tests whether all non-blocking requests have completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and determines whether all of these requests
 *  have been completed. However, due to limitations of the underlying
 *  MPI implementation, if any of the requests refers to a
 *  non-blocking send or receive of a serialized data type, @c
 *  test_all will always return the equivalent of @c false (i.e., the
 *  requests cannot all be finished at this time). This routine
 *  performs the same functionality as @c wait_all, except that this
 *  routine will not block. This routine provides functionality
 *  equivalent to @c MPI_Testall.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. 
 *
 *  @param out If provided and all requests hav been completed, an
 *  output iterator through which the status of each request will be
 *  emitted. The @c status objects are emitted in the same order as
 *  the requests are retrieved from @c [first,last).
 *
 *  @returns If an @p out parameter was provided, the value @c out
 *  after all of the @c status objects have been emitted (if all
 *  requests were completed) or an empty @c optional<>. If no @p out
 *  parameter was provided, returns @c true if all requests have
 *  completed or @c false otherwise.
 */
template<typename ForwardIterator, typename OutputIterator>
optional<OutputIterator>
test_all(ForwardIterator first, ForwardIterator last, OutputIterator out)
{
  std::vector<MPI_Request> requests;
  for (; first != last; ++first) {
    // If we have a non-trivial request, then no requests can be
    // completed.
    if (!first->trivial()) {
      return optional<OutputIterator>();
    }
    requests.push_back(*first->trivial());
  }

  int flag = 0;
  int n = requests.size();
  std::vector<MPI_Status> stats(n);
  BOOST_MPI_CHECK_RESULT(MPI_Testall, (n, detail::c_data(requests), &flag, detail::c_data(stats)));
  if (flag) {
    for (int i = 0; i < n; ++i, ++out) {
      status stat;
      stat.m_status = stats[i];
      *out = stat;
    }
    return out;
  } else {
    return optional<OutputIterator>();
  }
}

/**
 *  \overload
 */
template<typename ForwardIterator>
bool
test_all(ForwardIterator first, ForwardIterator last)
{
  std::vector<MPI_Request> requests;
  for (; first != last; ++first) {
    // If we have a non-trivial request, then no requests can be
    // completed.
    if (!first->trivial()) {
      return false;
    }
    requests.push_back(*first->trivial());
  }

  int flag = 0;
  int n = requests.size();
  BOOST_MPI_CHECK_RESULT(MPI_Testall, 
                         (n, detail::c_data(requests), &flag, MPI_STATUSES_IGNORE));
  return flag != 0;
}

/** 
 *  @brief Wait until some non-blocking requests have completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and waits until at least one of the requests
 *  has completed. It then completes all of the requests it can,
 *  partitioning the input sequence into pending requests followed by
 *  completed requests. If an output iterator is provided, @c status
 *  objects will be emitted for each of the completed requests. This
 *  routine provides functionality equivalent to @c MPI_Waitsome.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. This may not be equal to @c first.
 *
 *  @param out If provided, the @c status objects corresponding to
 *  completed requests will be emitted through this output iterator.

 *  @returns If the @p out parameter was provided, a pair containing
 *  the output iterator @p out after all of the @c status objects have
 *  been written through it and an iterator referencing the first
 *  completed request. If no @p out parameter was provided, only the
 *  iterator referencing the first completed request will be emitted.
 */
template<typename BidirectionalIterator, typename OutputIterator>
std::pair<OutputIterator, BidirectionalIterator> 
wait_some(BidirectionalIterator first, BidirectionalIterator last,
          OutputIterator out)
{
  using std::advance;

  if (first == last)
    return std::make_pair(out, first);
  
  typedef typename std::iterator_traits<BidirectionalIterator>::difference_type
    difference_type;

  bool all_trivial_requests = true;
  difference_type n = 0;
  BidirectionalIterator current = first;
  BidirectionalIterator start_of_completed = last;
  while (true) {
    // Check if we have found a completed request. 
    if (optional<status> result = current->test()) {
      using std::iter_swap;

      // Emit the resulting status object
      *out++ = *result;

      // We're expanding the set of completed requests
      --start_of_completed;

      if (current == start_of_completed) {
        // If we have hit the end of the list of pending
        // requests. Finish up by fixing the order of the completed
        // set to match the order in which we emitted status objects,
        // then return.
        std::reverse(start_of_completed, last);
        return std::make_pair(out, start_of_completed);
      }

      // Swap the request we just completed with the last request that
      // has not yet been tested.
      iter_swap(current, start_of_completed);

      continue;
    }

    // Check if this request (and all others before it) are "trivial"
    // requests, e.g., they can be represented with a single
    // MPI_Request.
    all_trivial_requests = all_trivial_requests && current->trivial();

    // Move to the next request.
    ++n;
    if (++current == start_of_completed) {
      if (start_of_completed != last) {
        // We have satisfied some requests. Make the order of the
        // completed requests match that of the status objects we've
        // already emitted and we're done.
        std::reverse(start_of_completed, last);
        return std::make_pair(out, start_of_completed);
      }

      // We have reached the end of the list. If all requests thus far
      // have been trivial, we can call MPI_Waitsome directly, because
      // it may be more efficient than our busy-wait semantics.
      if (all_trivial_requests) {
        std::vector<MPI_Request> requests;
        std::vector<int> indices(n);
        std::vector<MPI_Status> stats(n);
        requests.reserve(n);
        for (current = first; current != last; ++current)
          requests.push_back(*current->trivial());

        // Let MPI wait until some of these operations complete.
        int num_completed;
        BOOST_MPI_CHECK_RESULT(MPI_Waitsome, 
                               (n, detail::c_data(requests), &num_completed, detail::c_data(indices),
                                detail::c_data(stats)));

        // Translate the index-based result of MPI_Waitsome into a
        // partitioning on the requests.
        int current_offset = 0;
        current = first;
        for (int index = 0; index < num_completed; ++index, ++out) {
          using std::iter_swap;

          // Move "current" to the request object at this index
          advance(current, indices[index] - current_offset);
          current_offset = indices[index];

          // Emit the status object
          status stat;
          stat.m_status = stats[index];
          *out = stat;

          // Finish up the request and swap it into the "completed
          // requests" partition.
          *current->trivial() = requests[indices[index]];
          --start_of_completed;
          iter_swap(current, start_of_completed);
        }

        // We have satisfied some requests. Make the order of the
        // completed requests match that of the status objects we've
        // already emitted and we're done.
        std::reverse(start_of_completed, last);
        return std::make_pair(out, start_of_completed);
      }

      // There are some nontrivial requests, so we must continue our
      // busy waiting loop.
      n = 0;
      current = first;
    }
  }

  // We cannot ever get here
  BOOST_ASSERT(false);
}

/**
 *  \overload
 */
template<typename BidirectionalIterator>
BidirectionalIterator
wait_some(BidirectionalIterator first, BidirectionalIterator last)
{
  using std::advance;

  if (first == last)
    return first;
  
  typedef typename std::iterator_traits<BidirectionalIterator>::difference_type
    difference_type;

  bool all_trivial_requests = true;
  difference_type n = 0;
  BidirectionalIterator current = first;
  BidirectionalIterator start_of_completed = last;
  while (true) {
    // Check if we have found a completed request. 
    if (optional<status> result = current->test()) {
      using std::iter_swap;

      // We're expanding the set of completed requests
      --start_of_completed;

      // If we have hit the end of the list of pending requests, we're
      // done.
      if (current == start_of_completed)
        return start_of_completed;

      // Swap the request we just completed with the last request that
      // has not yet been tested.
      iter_swap(current, start_of_completed);

      continue;
    }

    // Check if this request (and all others before it) are "trivial"
    // requests, e.g., they can be represented with a single
    // MPI_Request.
    all_trivial_requests = all_trivial_requests && current->trivial();

    // Move to the next request.
    ++n;
    if (++current == start_of_completed) {
        // If we have satisfied some requests, we're done.
      if (start_of_completed != last)
        return start_of_completed;

      // We have reached the end of the list. If all requests thus far
      // have been trivial, we can call MPI_Waitsome directly, because
      // it may be more efficient than our busy-wait semantics.
      if (all_trivial_requests) {
        std::vector<MPI_Request> requests;
        std::vector<int> indices(n);
        requests.reserve(n);
        for (current = first; current != last; ++current)
          requests.push_back(*current->trivial());

        // Let MPI wait until some of these operations complete.
        int num_completed;
        BOOST_MPI_CHECK_RESULT(MPI_Waitsome, 
                               (n, detail::c_data(requests), &num_completed, detail::c_data(indices),
                                MPI_STATUSES_IGNORE));

        // Translate the index-based result of MPI_Waitsome into a
        // partitioning on the requests.
        int current_offset = 0;
        current = first;
        for (int index = 0; index < num_completed; ++index) {
          using std::iter_swap;

          // Move "current" to the request object at this index
          advance(current, indices[index] - current_offset);
          current_offset = indices[index];

          // Finish up the request and swap it into the "completed
          // requests" partition.
          *current->trivial() = requests[indices[index]];
          --start_of_completed;
          iter_swap(current, start_of_completed);
        }

        // We have satisfied some requests, so we are done.
        return start_of_completed;
      }

      // There are some nontrivial requests, so we must continue our
      // busy waiting loop.
      n = 0;
      current = first;
    }
  }

  // We cannot ever get here
  BOOST_ASSERT(false);
}

/** 
 *  @brief Test whether some non-blocking requests have completed.
 *
 *  This routine takes in a set of requests stored in the iterator
 *  range @c [first,last) and tests to see if any of the requests has
 *  completed. It completes all of the requests it can, partitioning
 *  the input sequence into pending requests followed by completed
 *  requests. If an output iterator is provided, @c status objects
 *  will be emitted for each of the completed requests. This routine
 *  is similar to @c wait_some, but does not wait until any requests
 *  have completed. This routine provides functionality equivalent to
 *  @c MPI_Testsome.
 *
 *  @param first The iterator that denotes the beginning of the
 *  sequence of request objects.
 *
 *  @param last The iterator that denotes the end of the sequence of
 *  request objects. This may not be equal to @c first.
 *
 *  @param out If provided, the @c status objects corresponding to
 *  completed requests will be emitted through this output iterator.

 *  @returns If the @p out parameter was provided, a pair containing
 *  the output iterator @p out after all of the @c status objects have
 *  been written through it and an iterator referencing the first
 *  completed request. If no @p out parameter was provided, only the
 *  iterator referencing the first completed request will be emitted.
 */
template<typename BidirectionalIterator, typename OutputIterator>
std::pair<OutputIterator, BidirectionalIterator> 
test_some(BidirectionalIterator first, BidirectionalIterator last,
          OutputIterator out)
{
  BidirectionalIterator current = first;
  BidirectionalIterator start_of_completed = last;
  while (current != start_of_completed) {
    // Check if we have found a completed request. 
    if (optional<status> result = current->test()) {
      using std::iter_swap;

      // Emit the resulting status object
      *out++ = *result;

      // We're expanding the set of completed requests
      --start_of_completed;

      // Swap the request we just completed with the last request that
      // has not yet been tested.
      iter_swap(current, start_of_completed);

      continue;
    }

    // Move to the next request.
    ++current;
  }

  // Finish up by fixing the order of the completed set to match the
  // order in which we emitted status objects, then return.
  std::reverse(start_of_completed, last);
  return std::make_pair(out, start_of_completed);
}

/**
 *  \overload
 */
template<typename BidirectionalIterator>
BidirectionalIterator
test_some(BidirectionalIterator first, BidirectionalIterator last)
{
  BidirectionalIterator current = first;
  BidirectionalIterator start_of_completed = last;
  while (current != start_of_completed) {
    // Check if we have found a completed request. 
    if (optional<status> result = current->test()) {
      using std::iter_swap;

      // We're expanding the set of completed requests
      --start_of_completed;

      // Swap the request we just completed with the last request that
      // has not yet been tested.
      iter_swap(current, start_of_completed);

      continue;
    }

    // Move to the next request.
    ++current;
  }

  return start_of_completed;
}

} } // end namespace boost::mpi


#endif // BOOST_MPI_NONBLOCKING_HPP
