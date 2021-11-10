// (C) Copyright 2005 Matthias Troyer
// (C) Copyright 2006 Douglas Gregor <doug.gregor -at- gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer
//           Douglas Gregor

/** @file packed_oarchive.hpp
 *
 *  This header provides the facilities for unpacking Serializable
 *  data types from a buffer using @c MPI_Unpack. The buffers are
 *  typically received via MPI and have been packed either by via the
 *  facilities in @c packed_iarchive.hpp or @c MPI_Pack.
 */
#ifndef BOOST_MPI_PACKED_OARCHIVE_HPP
#define BOOST_MPI_PACKED_OARCHIVE_HPP

#include <boost/mpi/datatype.hpp>
#include <boost/archive/basic_archive.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/mpi/detail/packed_oprimitive.hpp>
#include <boost/mpi/detail/binary_buffer_oprimitive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/item_version_type.hpp>

namespace boost { namespace mpi {

#ifdef BOOST_MPI_HOMOGENEOUS
  typedef binary_buffer_oprimitive oprimitive;
#else
  typedef packed_oprimitive oprimitive;
#endif

/** @brief An archive that packs binary data into an MPI buffer.
 *
 *  The @c packed_iarchive class is an Archiver (as in the
 *  Boost.Serialization library) that packs binary data into a buffer
 *  for transmission via MPI. It can operate on any Serializable data
 *  type and will use the @c MPI_Pack function of the underlying MPI
 *  implementation to perform serialization.
 */

class BOOST_MPI_DECL packed_oarchive
  : public oprimitive
  , public archive::detail::common_oarchive<packed_oarchive>
{
public:
  /**
   *  Construct a @c packed_oarchive for transmission over the given
   *  MPI communicator and with an initial buffer.
   *
   *  @param comm The communicator over which this archive will be
   *  sent.
   *
   *  @param b A user-defined buffer that will be filled with the
   *  binary representation of serialized objects.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   *
   *  @param position Set the offset into buffer @p b at which
   *  deserialization will begin.
   */
  packed_oarchive( MPI_Comm const & comm, buffer_type & b, unsigned int flags = boost::archive::no_header)
         : oprimitive(b,comm),
           archive::detail::common_oarchive<packed_oarchive>(flags)
        {}

  /**
   *  Construct a @c packed_oarchive for transmission over the given
   *  MPI communicator.
   *
   *  @param comm The communicator over which this archive will be
   *  sent.
   *
   *  @param s The size of the buffer to be received.
   *
   *  @param flags Control the serialization of the data types. Refer
   *  to the Boost.Serialization documentation before changing the
   *  default flags.
   */
  packed_oarchive ( MPI_Comm const & comm, unsigned int flags =  boost::archive::no_header)
         : oprimitive(internal_buffer_,comm),
           archive::detail::common_oarchive<packed_oarchive>(flags)
        {}

  // Save everything else in the usual way, forwarding on to the Base class
  template<class T>
  void save_override(T const& x, mpl::false_)
  {
    archive::detail::common_oarchive<packed_oarchive>::save_override(x);
  }

  // Save it directly using the primitives
  template<class T>
  void save_override(T const& x, mpl::true_)
  {
    oprimitive::save(x);
  }

  // Save all supported datatypes directly
  template<class T>
  void save_override(T const& x)
  {
    typedef typename mpl::apply1<use_array_optimization,T>::type use_optimized;
    save_override(x, use_optimized());
  }

  // output archives need to ignore  the optional information
  void save_override(const archive::class_id_optional_type & ){}

  // explicitly convert to char * to avoid compile ambiguities
  void save_override(const archive::class_name_type & t){
      const std::string s(t);
      * this->This() << s;
  }

  void save_override(const archive::class_id_type & t){
    const boost::int_least16_t x = t;
    * this->This() << x;
  }

  void save_override(const archive::version_type & t){
    const boost::int_least8_t x = t;
    * this->This() << x;
  }
private:
  /// An internal buffer to be used when the user does not supply his
  /// own buffer.
  buffer_type internal_buffer_;
};

} } // end namespace boost::mpi

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(boost::mpi::packed_oarchive)
BOOST_SERIALIZATION_USE_ARRAY_OPTIMIZATION(boost::mpi::packed_oarchive)



#endif // BOOST_MPI_PACKED_OARCHIVE_HPP
