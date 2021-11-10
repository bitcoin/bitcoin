//          Copyright Alain Miniussi 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Authors: Alain Miniussi

/** @file cartesian_communicator.hpp
 *
 *  This header defines facilities to support MPI communicators with
 *  cartesian topologies.
 *  If known at compiled time, the dimension of the implied grid 
 *  can be statically enforced, through the templatized communicator 
 *  class. Otherwise, a non template, dynamic, base class is provided.
 *  
 */
#ifndef BOOST_MPI_CARTESIAN_COMMUNICATOR_HPP
#define BOOST_MPI_CARTESIAN_COMMUNICATOR_HPP

#include <boost/mpi/communicator.hpp>

#include <vector>
#include <utility>
#include <iostream>
#include <utility>
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif // BOOST_NO_CXX11_HDR_INITIALIZER_LIST

// Headers required to implement cartesian topologies
#include <boost/shared_array.hpp>
#include <boost/assert.hpp>
#include <boost/foreach.hpp>

namespace boost { namespace mpi {

/**
 * @brief Specify the size and periodicity of the grid in a single dimension.
 *
 * POD lightweight object.
 */
struct cartesian_dimension {
  /** The size of the grid n this dimension. */
  int size;
  /** Is the grid periodic in this dimension. */
  bool periodic;
  
  cartesian_dimension(int sz = 0, bool p = false) : size(sz), periodic(p) {}
  
private:
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive & ar, const unsigned int version)
  {
    ar & size & periodic;
  }

};

template <>
struct is_mpi_datatype<cartesian_dimension> : mpl::true_ { };

/**
 * @brief Test if the dimensions values are identical.
 */
inline
bool
operator==(cartesian_dimension const& d1, cartesian_dimension const& d2) {
  return &d1 == &d2 || (d1.size == d2.size && d1.periodic == d2.periodic);
}

/**
 * @brief Test if the dimension values are different.
 */
inline
bool
operator!=(cartesian_dimension const& d1, cartesian_dimension const& d2) {
  return !(d1 == d2);
}

/**
 * @brief Pretty printing of a cartesian dimension (size, periodic)
 */
std::ostream& operator<<(std::ostream& out, cartesian_dimension const& d);

/**
 * @brief Describe the topology of a cartesian grid.
 * 
 * Behave mostly like a sequence of @c cartesian_dimension with the notable 
 * exception that its size is fixed.
 * This is a lightweight object, so that any constructor that could be considered 
 * missing could be replaced with a function (move constructor provided when supported).
 */
class BOOST_MPI_DECL cartesian_topology 
  : private std::vector<cartesian_dimension> {
  friend class cartesian_communicator;
  typedef std::vector<cartesian_dimension> super;
 public:
  /** 
   * Retrieve a specific dimension.
   */
  using super::operator[];
  /** 
   * @brief Topology dimentionality.
   */
  using super::size;
  using super::begin;
  using super::end;
  using super::swap;

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
  cartesian_topology() = delete;
#endif
#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
  cartesian_topology(cartesian_topology const&) = default;
  cartesian_topology& operator=(cartesian_topology const&) = default;
  // There is apparently no macro for checking the support of move constructor.
  // Assume that defaulted function is close enough.
#if !defined(BOOST_NO_CXX11_DEFAULTED_MOVES)
  cartesian_topology(cartesian_topology&& other) : super(other) {}
  cartesian_topology& operator=(cartesian_topology&& other) { 
    stl().swap(other.stl());
    return *this;
  }
#endif
  ~cartesian_topology() = default;
#endif
  /**
   * @brief Create a N dimension space.
   * Each dimension is initialized as non periodic of size 0.
   */
  cartesian_topology(int ndim) 
    : super(ndim) {}
  
  /**
   * @brief Use the provided dimensions specification as initial values.
   */
  cartesian_topology(std::vector<cartesian_dimension> const& dims) 
    : super(dims) {}

  /**
   * @brief Use dimensions specification provided in the sequence container as initial values.
   * #param dims must be a sequence container.
   */  
  template<class InitArr>
  explicit cartesian_topology(InitArr dims)
    : super(0) {
    BOOST_FOREACH(cartesian_dimension const& d, dims) {
      push_back(d);
    }
  }
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
  /**
   * @brief Use dimensions specification provided in the initialization list as initial values.
   * #param dims can be of the form { dim_1, false}, .... {dim_n, true}
   */    
  explicit cartesian_topology(std::initializer_list<cartesian_dimension> dims)
    : super(dims) {}
#endif
  /**
   * @brief Use dimensions specification provided in the array.
   * #param dims can be of the form { dim_1, false}, .... {dim_n, true}
   */    
  template<int NDIM>
  explicit cartesian_topology(cartesian_dimension (&dims)[NDIM])
    : super(dims, dims+NDIM) {}

  /**
   * @brief Use dimensions specification provided in the input ranges
   * The ranges do not need to be the same size. If the sizes are different, 
   * the missing values will be complete with zeros of the dim and assumed non periodic.
   * @param dim_rg     the dimensions, values must convert to integers.
   * @param period_rg  the periodicities, values must convert to booleans.
   * #param dims can be of the form { dim_1, false}, .... {dim_n, true}
   */    
  template<class DimRg, class PerRg>
  cartesian_topology(DimRg const& dim_rg, PerRg const& period_rg) 
    : super(0) {
    BOOST_FOREACH(int d, dim_rg) {
      super::push_back(cartesian_dimension(d));
    }
    super::iterator it = begin();
    BOOST_FOREACH(bool p, period_rg) {
      if (it < end()) {
        it->periodic = p;
      } else {
        push_back(cartesian_dimension(0,p));
      }
      ++it;
    }
  }

  
  /**
   * @brief Iterator based initializer.
   * Will use the first n iterated values. 
   * Both iterators can be single pass.
   * @param dit dimension iterator, value must convert to integer type.
   * @param pit periodicity iterator, value must convert to booleans..
   */
  template<class DimIter, class PerIter>
  cartesian_topology(DimIter dit, PerIter pit, int n) 
    : super(n) {
    for(int i = 0; i < n; ++i) {
      (*this)[i] = cartesian_dimension(*dit++, *pit++);
    }
  }
  
  /**
   * Export as an stl sequence.
   */
  std::vector<cartesian_dimension>& stl() { return *this; }
  /**
   * Export as an stl sequence.
   */
  std::vector<cartesian_dimension> const& stl() const{ return *this; }
  /** 
   * Split the topology in two sequences of sizes and periodicities.
   */
  void split(std::vector<int>& dims, std::vector<bool>& periodics) const;
};

inline
bool
operator==(cartesian_topology const& t1, cartesian_topology const& t2) {
  return t1.stl() == t2.stl();
}

inline
bool
operator!=(cartesian_topology const& t1, cartesian_topology const& t2) {
  return t1.stl() != t2.stl();
}

/**
 * @brief Pretty printing of a cartesian topology
 */
std::ostream& operator<<(std::ostream& out, cartesian_topology const& t);

/**
 * @brief An MPI communicator with a cartesian topology.
 *
 * A @c cartesian_communicator is a communicator whose topology is
 * expressed as a grid. Cartesian communicators have the same
 * functionality as (intra)communicators, but also allow one to query
 * the relationships among processes and the properties of the grid.
 */
class BOOST_MPI_DECL cartesian_communicator : public communicator
{
  friend class communicator;

  /**
   * INTERNAL ONLY
   *
   * Construct a cartesian communicator given a shared pointer to the
   * underlying MPI_Comm (which must have a cartesian topology).
   * This operation is used for "casting" from a communicator to 
   * a cartesian communicator.
   */
  explicit cartesian_communicator(const shared_ptr<MPI_Comm>& comm_ptr)
    : communicator()
  {
    this->comm_ptr = comm_ptr;
    BOOST_ASSERT(has_cartesian_topology());    
  }

public:
  /**
   * Build a new Boost.MPI cartesian communicator based on the MPI
   * communicator @p comm with cartesian topology.
   *
   * @p comm may be any valid MPI communicator. If @p comm is
   * MPI_COMM_NULL, an empty communicator (that cannot be used for
   * communication) is created and the @p kind parameter is
   * ignored. Otherwise, the @p kind parameter determines how the
   * Boost.MPI communicator will be related to @p comm:
   *
   *   - If @p kind is @c comm_duplicate, duplicate @c comm to create
   *   a new communicator. This new communicator will be freed when
   *   the Boost.MPI communicator (and all copies of it) is
   *   destroyed. This option is only permitted if the underlying MPI
   *   implementation supports MPI 2.0; duplication of
   *   intercommunicators is not available in MPI 1.x.
   *
   *   - If @p kind is @c comm_take_ownership, take ownership of @c
   *   comm. It will be freed automatically when all of the Boost.MPI
   *   communicators go out of scope.
   *
   *   - If @p kind is @c comm_attach, this Boost.MPI communicator
   *   will reference the existing MPI communicator @p comm but will
   *   not free @p comm when the Boost.MPI communicator goes out of
   *   scope. This option should only be used when the communicator is
   *   managed by the user.
   */
  cartesian_communicator(const MPI_Comm& comm, comm_create_kind kind)
    : communicator(comm, kind)
  { 
    BOOST_ASSERT(has_cartesian_topology());
  }

  /**
   *  Create a new communicator whose topology is described by the
   *  given cartesian. The indices of the vertices in the cartesian will be
   *  assumed to be the ranks of the processes within the
   *  communicator. There may be fewer vertices in the cartesian than
   *  there are processes in the communicator; in this case, the
   *  resulting communicator will be a NULL communicator.
   *
   *  @param comm The communicator that the new, cartesian communicator
   *  will be based on. 
   * 
   *  @param dims the cartesian dimension of the new communicator. The size indicate 
   *  the number of dimension. Some dimensions be set to zero, in which case
   *  the corresponding dimension value is left to the system.
   *  
   *  @param reorder Whether MPI is permitted to re-order the process
   *  ranks within the returned communicator, to better optimize
   *  communication. If false, the ranks of each process in the
   *  returned process will match precisely the rank of that process
   *  within the original communicator.
   */
  cartesian_communicator(const communicator&       comm,
                         const cartesian_topology& dims,
                         bool                      reorder = false);
  
  /**
   * Create a new cartesian communicator whose topology is a subset of
   * an existing cartesian cimmunicator.
   * @param comm the original communicator.
   * @param keep and array containiing the dimension to keep from the existing 
   * communicator.
   */
  cartesian_communicator(const cartesian_communicator& comm,
                         const std::vector<int>&       keep );
    
  using communicator::rank;

  /** 
   * Retrive the number of dimension of the underlying toppology.
   */
  int ndims() const;
  
  /**
   * Return the rank of the process at the given coordinates.
   * @param coords the coordinates. the size must match the communicator's topology.
   */
  int rank(const std::vector<int>& coords) const;
  /**
   * Return the rank of the source and target destination process through a shift.
   * @param dim the dimension in which the shift takes place. 0 <= dim <= ndim().
   * @param disp the shift displacement, can be positive (upward) or negative (downward).
   */
  std::pair<int, int> shifted_ranks(int dim, int disp) const;
  /**
   * Provides the coordinates of the process with the given rank.
   * @param rk the ranks in this communicator.
   * @returns the coordinates.
   */
  std::vector<int> coordinates(int rk) const;
  /**
   * Retrieve the topology and coordinates of this process in the grid.
   *
   */
  void topology( cartesian_topology&  dims, std::vector<int>& coords ) const;
  /**
   * Retrieve the topology of the grid.
   *
   */
  cartesian_topology topology() const;
};

/**
 * Given en number of processes, and a partially filled sequence 
 * of dimension, try to complete the dimension sequence.
 * @param nb_proc the numer of mpi processes.fill a sequence of dimension.
 * @param dims a sequence of positive or null dimensions. Non zero dimension 
 *  will be left untouched.
 */
std::vector<int>& cartesian_dimensions(int nb_proc, std::vector<int>&  dims);

} } // end namespace boost::mpi

#endif // BOOST_MPI_CARTESIAN_COMMUNICATOR_HPP
