// (C) Copyright 2005 Matthias Troyer
// (C) Copyright 2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer
//           Douglas Gregor

/** @file packed_iarchive.hpp
 *
 *  This header provides the facilities for packing Serializable data
 *  types into a buffer using @c MPI_Pack. The buffers can then be
 *  transmitted via MPI and then be unpacked either via the facilities
 *  in @c packed_oarchive.hpp or @c MPI_Unpack.
 */
#ifndef BOOST_MPI_PACKED_IARCHIVE_HPP
#define BOOST_MPI_PACKED_IARCHIVE_HPP

#include <boost/mpi/datatype.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/archive/basic_archive.hpp>
#include <boost/mpi/detail/packed_iprimitive.hpp>
#include <boost/mpi/detail/binary_buffer_iprimitive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/assert.hpp>

namespace boost { namespace mpi {

#ifdef BOOST_MPI_HOMOGENEOUS
  typedef binary_buffer_iprimitive iprimitive;
#else
  typedef packed_iprimitive iprimitive;
#endif


/** @brief An archive that unpacks binary data from an MPI buffer.
 *
 *  The @c packed_oarchive class is an Archiver (as in the
 *  Boost.Serialization library) that unpacks binary data from a
 *  buffer received via MPI. It can operate on any Serializable data
 *  type and will use the @c MPI_Unpack function of the underlying MPI
 *  implementation to perform deserialization.
 */

class BOOST_MPI_DECL packed_iarchive
  : public iprimitive
  , public archive::detail::common_iarchive<packed_iarchive>
{
public:
  /**
   *  Construct a @c packed_iarchive to receive data over the given
   *  MPI communicator and with an initial buffer.
   *
   *  @param comm The communicator over which this archive will be
   *  received.
   *
   *  @param b A user-defined buffer that contains the binary
   *  representation of serialized objects.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   */

  packed_iarchive(MPI_Comm const & comm, buffer_type & b, unsigned int flags = boost::archive::no_header, int position = 0)
        : iprimitive(b,comm,position),
          archive::detail::common_iarchive<packed_iarchive>(flags)
        {}

  /**
   *  Construct a @c packed_iarchive to receive data over the given
   *  MPI communicator.
   *
   *  @param comm The communicator over which this archive will be
   *  received.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   */

  packed_iarchive
          ( MPI_Comm const & comm , std::size_t s=0,
           unsigned int flags = boost::archive::no_header)
         : iprimitive(internal_buffer_,comm)
         , archive::detail::common_iarchive<packed_iarchive>(flags)
         , internal_buffer_(s)
        {}

  // Load everything else in the usual way, forwarding on to the Base class
  template<class T>
  void load_override(T& x, mpl::false_)
  {
    archive::detail::common_iarchive<packed_iarchive>::load_override(x);
  }

  // Load it directly using the primnivites
  template<class T>
  void load_override(T& x, mpl::true_)
  {
    iprimitive::load(x);
  }

  // Load all supported datatypes directly
  template<class T>
  void load_override(T& x)
  {
    typedef typename mpl::apply1<use_array_optimization
      , BOOST_DEDUCED_TYPENAME remove_const<T>::type
    >::type use_optimized;
    load_override(x, use_optimized());
  }

  // input archives need to ignore  the optional information
  void load_override(archive::class_id_optional_type & /*t*/){}

  void load_override(archive::class_id_type & t){
    int_least16_t x=0;
    * this->This() >> x;
    t = boost::archive::class_id_type(x);
  }

  void load_override(archive::version_type & t){
    int_least8_t x=0;
    * this->This() >> x;
    t = boost::archive::version_type(x);
  }

  void load_override(archive::class_id_reference_type & t){
    load_override(static_cast<archive::class_id_type &>(t));
  }

  void load_override(archive::class_name_type & t)
  {
    std::string cn;
    cn.reserve(BOOST_SERIALIZATION_MAX_KEY_SIZE);
    * this->This() >> cn;
    std::memcpy(t, cn.data(), cn.size());
    // borland tweak
    t.t[cn.size()] = '\0';
  }

private:
  /// An internal buffer to be used when the user does not supply his
  /// own buffer.
  buffer_type internal_buffer_;
};

} } // end namespace boost::mpi

BOOST_SERIALIZATION_REGISTER_ARCHIVE(boost::mpi::packed_iarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(boost::mpi::packed_iarchive)

#endif // BOOST_MPI_PACKED_IARCHIVE_HPP
