// Copyright (C) 2006 Douglas Gregor <doug.gregor -at- gmail.com>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file status.hpp
 *
 *  This header defines the class @c status, which reports on the
 *  results of point-to-point communication.
 */
#ifndef BOOST_MPI_STATUS_HPP
#define BOOST_MPI_STATUS_HPP

#include <boost/mpi/config.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/optional.hpp>
#include <boost/mpl/bool.hpp>

namespace boost { namespace mpi {

class request;
class communicator;

/** @brief Contains information about a message that has been or can
 *  be received.
 *
 *  This structure contains status information about messages that
 *  have been received (with @c communicator::recv) or can be received
 *  (returned from @c communicator::probe or @c
 *  communicator::iprobe). It permits access to the source of the
 *  message, message tag, error code (rarely used), or the number of
 *  elements that have been transmitted.
 */
class BOOST_MPI_DECL status
{
 public:
  status() : m_count(-1) { }
  
  status(MPI_Status const& s) : m_status(s), m_count(-1) {}

  /**
   * Retrieve the source of the message.
   */
  int source() const { return m_status.MPI_SOURCE; }

  /**
   * Retrieve the message tag.
   */
  int tag() const { return m_status.MPI_TAG; }

  /**
   * Retrieve the error code.
   */
  int error() const { return m_status.MPI_ERROR; }

  /**
   * Determine whether the communication associated with this object
   * has been successfully cancelled.
  */
  bool cancelled() const;

  /**
   * Determines the number of elements of type @c T contained in the
   * message. The type @c T must have an associated data type, i.e.,
   * @c is_mpi_datatype<T> must derive @c mpl::true_. In cases where
   * the type @c T does not match the transmitted type, this routine
   * will return an empty @c optional<int>.
   *
   * @returns the number of @c T elements in the message, if it can be
   * determined.
   */
  template<typename T> optional<int> count() const { return count_impl<T>(is_mpi_datatype<T>()); }

  /**
   * References the underlying @c MPI_Status
   */
  operator       MPI_Status&()       { return m_status; }

  /**
   * References the underlying @c MPI_Status
   */
  operator const MPI_Status&() const { return m_status; }

 private:
  /**
   * INTERNAL ONLY
   */
  template<typename T> optional<int> count_impl(mpl::true_) const;

  /**
   * INTERNAL ONLY
   */
  template<typename T> optional<int> count_impl(mpl::false_) const;

 public: // friend templates are not portable

  /// INTERNAL ONLY
  mutable MPI_Status m_status;
  mutable int m_count;

  friend class communicator;
  friend class request;
};

template<typename T> 
inline optional<int> status::count_impl(mpl::true_) const
{
  if (m_count != -1)
    return m_count;

  int return_value;
  BOOST_MPI_CHECK_RESULT(MPI_Get_count,
                         (&m_status, get_mpi_datatype<T>(T()), &return_value));
  if (return_value == MPI_UNDEFINED)
    return optional<int>();
  else
    /* Cache the result. */
    return m_count = return_value;
}

template<typename T> 
inline optional<int> status::count_impl(mpl::false_) const
{
  if (m_count == -1)
    return optional<int>();
  else
    return m_count;
}


} } // end namespace boost::mpi

#endif // BOOST_MPI_STATUS_HPP
