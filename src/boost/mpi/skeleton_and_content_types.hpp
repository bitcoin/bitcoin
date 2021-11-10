// (C) Copyright 2005 Matthias Troyer
// (C) Copyright 2006 Douglas Gregor <doug.gregor -at gmail.com>

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Matthias Troyer
//           Douglas Gregor

/** @file skeleton_and_content.hpp
 *
 *  This header provides facilities that allow the structure of data
 *  types (called the "skeleton") to be transmitted and received
 *  separately from the content stored in those data types. These
 *  facilities are useful when the data in a stable data structure
 *  (e.g., a mesh or a graph) will need to be transmitted
 *  repeatedly. In this case, transmitting the skeleton only once
 *  saves both communication effort (it need not be sent again) and
 *  local computation (serialization need only be performed once for
 *  the content).
 */
#ifndef BOOST_MPI_SKELETON_AND_CONTENT_TYPES_HPP
#define BOOST_MPI_SKELETON_AND_CONTENT_TYPES_HPP

#include <boost/mpi/config.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>
#include <boost/mpi/detail/forward_skeleton_iarchive.hpp>
#include <boost/mpi/detail/forward_skeleton_oarchive.hpp>
#include <boost/mpi/detail/ignore_iprimitive.hpp>
#include <boost/mpi/detail/ignore_oprimitive.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/archive/detail/register_archive.hpp>

namespace boost { namespace mpi {

/**
 *  @brief A proxy that requests that the skeleton of an object be
 *  transmitted.
 *
 *  The @c skeleton_proxy is a lightweight proxy object used to
 *  indicate that the skeleton of an object, not the object itself,
 *  should be transmitted. It can be used with the @c send and @c recv
 *  operations of communicators or the @c broadcast collective. When a
 *  @c skeleton_proxy is sent, Boost.MPI generates a description
 *  containing the structure of the stored object. When that skeleton
 *  is received, the receiving object is reshaped to match the
 *  structure. Once the skeleton of an object as been transmitted, its
 *  @c content can be transmitted separately (often several times)
 *  without changing the structure of the object.
 */
template <class T>
struct BOOST_MPI_DECL skeleton_proxy
{
  /**
   *  Constructs a @c skeleton_proxy that references object @p x.
   *
   *  @param x the object whose structure will be transmitted or
   *  altered.
   */
  skeleton_proxy(T& x)
   : object(x)
  {}

  T& object;
};

/**
 *  @brief Create a skeleton proxy object.
 *
 *  This routine creates an instance of the skeleton_proxy class. It
 *  will typically be used when calling @c send, @c recv, or @c
 *  broadcast, to indicate that only the skeleton (structure) of an
 *  object should be transmitted and not its contents.
 *
 *  @param x the object whose structure will be transmitted.
 *
 *  @returns a skeleton_proxy object referencing @p x
 */
template <class T>
inline const skeleton_proxy<T> skeleton(T& x)
{
  return skeleton_proxy<T>(x);
}

namespace detail {
  /// @brief a class holding an MPI datatype
  /// INTERNAL ONLY
  /// the type is freed upon destruction
  class BOOST_MPI_DECL mpi_datatype_holder : public boost::noncopyable
  {
  public:
    mpi_datatype_holder()
     : is_committed(false)
    {}

    mpi_datatype_holder(MPI_Datatype t, bool committed = true)
     : d(t)
     , is_committed(committed)
    {}

    void commit()
    {
      BOOST_MPI_CHECK_RESULT(MPI_Type_commit,(&d));
      is_committed=true;
    }

    MPI_Datatype get_mpi_datatype() const
    {
      return d;
    }

    ~mpi_datatype_holder()
    {
      int finalized=0;
      BOOST_MPI_CHECK_RESULT(MPI_Finalized,(&finalized));
      if (!finalized && is_committed)
        BOOST_MPI_CHECK_RESULT(MPI_Type_free,(&d));
    }

  private:
    MPI_Datatype d;
    bool is_committed;
  };
} // end namespace detail

/** @brief A proxy object that transfers the content of an object
 *  without its structure.
 *
 *  The @c content class indicates that Boost.MPI should transmit or
 *  receive the content of an object, but without any information
 *  about the structure of the object. It is only meaningful to
 *  transmit the content of an object after the receiver has already
 *  received the skeleton for the same object.
 *
 *  Most users will not use @c content objects directly. Rather, they
 *  will invoke @c send, @c recv, or @c broadcast operations using @c
 *  get_content().
 */
class BOOST_MPI_DECL content
{
public:
  /**
   *  Constructs an empty @c content object. This object will not be
   *  useful for any Boost.MPI operations until it is reassigned.
   */
  content() {}

  /**
   *  This routine initializes the @c content object with an MPI data
   *  type that refers to the content of an object without its structure.
   *
   *  @param d the MPI data type referring to the content of the object.
   *
   *  @param committed @c true indicates that @c MPI_Type_commit has
   *  already been excuted for the data type @p d.
   */
  content(MPI_Datatype d, bool committed=true)
   : holder(new detail::mpi_datatype_holder(d,committed))
  {}

  /**
   *  Replace the MPI data type referencing the content of an object.
   *
   *  @param d the new MPI data type referring to the content of the
   *  object.
   *
   *  @returns *this
   */
  const content& operator=(MPI_Datatype d)
  {
    holder.reset(new detail::mpi_datatype_holder(d));
    return *this;
  }

  /**
   * Retrieve the MPI data type that refers to the content of the
   * object.
   *
   * @returns the MPI data type, which should only be transmitted or
   * received using @c MPI_BOTTOM as the address.
   */
  MPI_Datatype get_mpi_datatype() const
  {
    return holder->get_mpi_datatype();
  }

  /**
   *  Commit the MPI data type referring to the content of the
   *  object.
   */
  void commit()
  {
    holder->commit();
  }

private:
  boost::shared_ptr<detail::mpi_datatype_holder> holder;
};

/** @brief Returns the content of an object, suitable for transmission
 *   via Boost.MPI.
 *
 *  The function creates an absolute MPI datatype for the object,
 *  where all offsets are counted from the address 0 (a.k.a. @c
 *  MPI_BOTTOM) instead of the address @c &x of the object. This
 *  allows the creation of MPI data types for complex data structures
 *  containing pointers, such as linked lists or trees.
 *
 *  The disadvantage, compared to relative MPI data types is that for
 *  each object a new MPI data type has to be created.
 *
 *  The contents of an object can only be transmitted when the
 *  receiver already has an object with the same structure or shape as
 *  the sender. To accomplish this, first transmit the skeleton of the
 *  object using, e.g., @c skeleton() or @c skeleton_proxy.
 *
 *  The type @c T has to allow creation of an absolute MPI data type
 *  (content).
 *
 *  @param x the object for which the content will be transmitted.
 *
 *  @returns the content of the object @p x, which can be used for
 *  transmission via @c send, @c recv, or @c broadcast.
 */
template <class T> const content get_content(const T& x);

/** @brief An archiver that reconstructs a data structure based on the
 *  binary skeleton stored in a buffer.
 *
 *  The @c packed_skeleton_iarchive class is an Archiver (as in the
 *  Boost.Serialization library) that can construct the the shape of a
 *  data structure based on a binary skeleton stored in a buffer. The
 *  @c packed_skeleton_iarchive is typically used by the receiver of a
 *  skeleton, to prepare a data structure that will eventually receive
 *  content separately.
 *
 *  Users will not generally need to use @c packed_skeleton_iarchive
 *  directly. Instead, use @c skeleton or @c get_skeleton.
 */
class BOOST_MPI_DECL packed_skeleton_iarchive
  : public detail::ignore_iprimitive,
    public detail::forward_skeleton_iarchive<packed_skeleton_iarchive,packed_iarchive>
{
public:
  /**
   *  Construct a @c packed_skeleton_iarchive for the given
   *  communicator.
   *
   *  @param comm The communicator over which this archive will be
   *  transmitted.
   *
   *  @param flags Control the serialization of the skeleton. Refer to
   *  the Boost.Serialization documentation before changing the
   *  default flags.
   */
  packed_skeleton_iarchive(MPI_Comm const & comm,
                           unsigned int flags =  boost::archive::no_header)
         : detail::forward_skeleton_iarchive<packed_skeleton_iarchive,packed_iarchive>(skeleton_archive_)
         , skeleton_archive_(comm,flags)
        {}

  /**
   *  Construct a @c packed_skeleton_iarchive that unpacks a skeleton
   *  from the given @p archive.
   *
   *  @param archive the archive from which the skeleton will be
   *  unpacked.
   *
   */
  explicit packed_skeleton_iarchive(packed_iarchive & archive)
         : detail::forward_skeleton_iarchive<packed_skeleton_iarchive,packed_iarchive>(archive)
         , skeleton_archive_(MPI_COMM_WORLD, boost::archive::no_header)
        {}

  /**
   *  Retrieve the archive corresponding to this skeleton.
   */
  const packed_iarchive& get_skeleton() const
  {
    return this->implementation_archive;
  }

  /**
   *  Retrieve the archive corresponding to this skeleton.
   */
  packed_iarchive& get_skeleton()
  {
    return this->implementation_archive;
  }

private:
  /// Store the actual archive that holds the structure, unless the
  /// user overrides this with their own archive.
  packed_iarchive skeleton_archive_;
};

/** @brief An archiver that records the binary skeleton of a data
 * structure into a buffer.
 *
 *  The @c packed_skeleton_oarchive class is an Archiver (as in the
 *  Boost.Serialization library) that can record the shape of a data
 *  structure (called the "skeleton") into a binary representation
 *  stored in a buffer. The @c packed_skeleton_oarchive is typically
 *  used by the send of a skeleton, to pack the skeleton of a data
 *  structure for transmission separately from the content.
 *
 *  Users will not generally need to use @c packed_skeleton_oarchive
 *  directly. Instead, use @c skeleton or @c get_skeleton.
 */
class BOOST_MPI_DECL packed_skeleton_oarchive
  : public detail::ignore_oprimitive,
    public detail::forward_skeleton_oarchive<packed_skeleton_oarchive,packed_oarchive>
{
public:
  /**
   *  Construct a @c packed_skeleton_oarchive for the given
   *  communicator.
   *
   *  @param comm The communicator over which this archive will be
   *  transmitted.
   *
   *  @param flags Control the serialization of the skeleton. Refer to
   *  the Boost.Serialization documentation before changing the
   *  default flags.
   */
  packed_skeleton_oarchive(MPI_Comm const & comm,
                           unsigned int flags =  boost::archive::no_header)
         : detail::forward_skeleton_oarchive<packed_skeleton_oarchive,packed_oarchive>(skeleton_archive_)
         , skeleton_archive_(comm,flags)
        {}

  /**
   *  Construct a @c packed_skeleton_oarchive that packs a skeleton
   *  into the given @p archive.
   *
   *  @param archive the archive to which the skeleton will be packed.
   *
   */
  explicit packed_skeleton_oarchive(packed_oarchive & archive)
         : detail::forward_skeleton_oarchive<packed_skeleton_oarchive,packed_oarchive>(archive)
         , skeleton_archive_(MPI_COMM_WORLD, boost::archive::no_header)
        {}

  /**
   *  Retrieve the archive corresponding to this skeleton.
   */
  const packed_oarchive& get_skeleton() const
  {
    return this->implementation_archive;
  }

private:
  /// Store the actual archive that holds the structure.
  packed_oarchive skeleton_archive_;
};


} } // end namespace boost::mpi

#endif // BOOST_MPI_SKELETON_AND_CONTENT_TYPES_HPP
