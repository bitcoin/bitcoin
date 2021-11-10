// Copyright (C) 2004 The Trustees of Indiana University.
// Copyright (C) 2005-2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine

/** @file operations.hpp
 *
 *  This header provides a mapping from function objects to @c MPI_Op
 *  constants used in MPI collective operations. It also provides
 *  several new function object types not present in the standard @c
 *  <functional> header that have direct mappings to @c MPI_Op.
 */
#ifndef BOOST_MPI_IS_MPI_OP_HPP
#define BOOST_MPI_IS_MPI_OP_HPP

#include <boost/mpi/config.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpi/datatype.hpp>
#include <boost/core/enable_if.hpp>
#include <boost/core/uncaught_exceptions.hpp>
#include <functional>

namespace boost { namespace mpi {

template<typename Op, typename T> struct is_mpi_op;

/**
 * @brief Determine if a function object type is commutative.
 *
 * This trait determines if an operation @c Op is commutative when
 * applied to values of type @c T. Parallel operations such as @c
 * reduce and @c prefix_sum can be implemented more efficiently with
 * commutative operations. To mark an operation as commutative, users
 * should specialize @c is_commutative and derive from the class @c
 * mpl::true_.
 */
template<typename Op, typename T>
struct is_commutative : public mpl::false_ { };

/**************************************************************************
 * Function objects for MPI operations not in <functional> header         *
 **************************************************************************/

/**
 *  @brief Compute the maximum of two values.
 *
 *  This binary function object computes the maximum of the two values
 *  it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_MAX.
 */
template<typename T>
struct maximum
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns the maximum of x and y. */
  const T& operator()(const T& x, const T& y) const
  {
    return x < y? y : x;
  }
};

/**
 *  @brief Compute the minimum of two values.
 *
 *  This binary function object computes the minimum of the two values
 *  it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_MIN.
 */
template<typename T>
struct minimum
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns the minimum of x and y. */
  const T& operator()(const T& x, const T& y) const
  {
    return x < y? x : y;
  }
};


/**
 *  @brief Compute the bitwise AND of two integral values.
 *
 *  This binary function object computes the bitwise AND of the two
 *  values it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_BAND.
 */
template<typename T>
struct bitwise_and
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns @c x & y. */
  T operator()(const T& x, const T& y) const
  {
    return x & y;
  }
};

/**
 *  @brief Compute the bitwise OR of two integral values.
 *
 *  This binary function object computes the bitwise OR of the two
 *  values it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_BOR.
 */
template<typename T>
struct bitwise_or
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns the @c x | y. */
  T operator()(const T& x, const T& y) const
  {
    return x | y;
  }
};

/**
 *  @brief Compute the logical exclusive OR of two integral values.
 *
 *  This binary function object computes the logical exclusive of the
 *  two values it is given. When used with MPI and a type @c T that has
 *  an associated, built-in MPI data type, translates to @c MPI_LXOR.
 */
template<typename T>
struct logical_xor
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns the logical exclusive OR of x and y. */
  T operator()(const T& x, const T& y) const
  {
    return (x || y) && !(x && y);
  }
};

/**
 *  @brief Compute the bitwise exclusive OR of two integral values.
 *
 *  This binary function object computes the bitwise exclusive OR of
 *  the two values it is given. When used with MPI and a type @c T that
 *  has an associated, built-in MPI data type, translates to @c
 *  MPI_BXOR.
 */
template<typename T>
struct bitwise_xor
{
  typedef T first_argument_type;
  typedef T second_argument_type;
  typedef T result_type;
  /** @returns @c x ^ y. */
  T operator()(const T& x, const T& y) const
  {
    return x ^ y;
  }
};

/**************************************************************************
 * MPI_Op queries                                                         *
 **************************************************************************/

/**
 *  @brief Determine if a function object has an associated @c MPI_Op.
 *
 *  This trait determines if a function object type @c Op, when used
 *  with argument type @c T, has an associated @c MPI_Op. If so, @c
 *  is_mpi_op<Op,T> will derive from @c mpl::false_ and will
 *  contain a static member function @c op that takes no arguments but
 *  returns the associated @c MPI_Op value. For instance, @c
 *  is_mpi_op<std::plus<int>,int>::op() returns @c MPI_SUM.
 *
 *  Users may specialize @c is_mpi_op for any other class templates
 *  that map onto operations that have @c MPI_Op equivalences, such as
 *  bitwise OR, logical and, or maximum. However, users are encouraged
 *  to use the standard function objects in the @c functional and @c
 *  boost/mpi/operations.hpp headers whenever possible. For
 *  function objects that are class templates with a single template
 *  parameter, it may be easier to specialize @c is_builtin_mpi_op.
 */
template<typename Op, typename T>
struct is_mpi_op : public mpl::false_ { };

/// INTERNAL ONLY
template<typename T>
struct is_mpi_op<maximum<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_floating_point_datatype<T> >
{
  static MPI_Op op() { return MPI_MAX; }
};

/// INTERNAL ONLY
template<typename T>
struct is_mpi_op<minimum<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_floating_point_datatype<T> >
{
  static MPI_Op op() { return MPI_MIN; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<std::plus<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_floating_point_datatype<T>,
                           is_mpi_complex_datatype<T> >
{
  static MPI_Op op() { return MPI_SUM; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<std::multiplies<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_floating_point_datatype<T>,
                           is_mpi_complex_datatype<T> >
{
  static MPI_Op op() { return MPI_PROD; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<std::logical_and<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_logical_datatype<T> >
{
  static MPI_Op op() { return MPI_LAND; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<std::logical_or<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_logical_datatype<T> >
{
  static MPI_Op op() { return MPI_LOR; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<logical_xor<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_logical_datatype<T> >
{
  static MPI_Op op() { return MPI_LXOR; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<bitwise_and<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_byte_datatype<T> >
{
  static MPI_Op op() { return MPI_BAND; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<bitwise_or<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_byte_datatype<T> >
{
  static MPI_Op op() { return MPI_BOR; }
};

/// INTERNAL ONLY
template<typename T>
 struct is_mpi_op<bitwise_xor<T>, T>
  : public boost::mpl::or_<is_mpi_integer_datatype<T>,
                           is_mpi_byte_datatype<T> >
{
  static MPI_Op op() { return MPI_BXOR; }
};

namespace detail {
  // A helper class used to create user-defined MPI_Ops
  template<typename Op, typename T>
  class user_op
  {
  public:
    user_op()
    {
      BOOST_MPI_CHECK_RESULT(MPI_Op_create,
                             (&user_op<Op, T>::perform,
                              is_commutative<Op, T>::value,
                              &mpi_op));
    }

    ~user_op()
    {
      if (boost::core::uncaught_exceptions() > 0) {
        // Ignore failure cases: there are obviously other problems
        // already, and we don't want to cause program termination if
        // MPI_Op_free fails.
        MPI_Op_free(&mpi_op);
      } else {
        BOOST_MPI_CHECK_RESULT(MPI_Op_free, (&mpi_op));
      }
    }

    MPI_Op& get_mpi_op()
    {
      return mpi_op;
    }

  private:
    MPI_Op mpi_op;

    static void BOOST_MPI_CALLING_CONVENTION perform(void* vinvec, void* voutvec, int* plen, MPI_Datatype*)
    {
      T* invec = static_cast<T*>(vinvec);
      T* outvec = static_cast<T*>(voutvec);
      Op op;
      std::transform(invec, invec + *plen, outvec, outvec, op);
    }
  };

} // end namespace detail

} } // end namespace boost::mpi

#endif // BOOST_MPI_GET_MPI_OP_HPP
