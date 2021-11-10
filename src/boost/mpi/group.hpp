// Copyright (C) 2007 Trustees of Indiana University

// Authors: Douglas Gregor
//          Andrew Lumsdaine

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/** @file group.hpp
 *
 *  This header defines the @c group class, which allows one to
 *  manipulate and query groups of processes.
 */
#ifndef BOOST_MPI_GROUP_HPP
#define BOOST_MPI_GROUP_HPP

#include <boost/mpi/exception.hpp>
#include <boost/mpi/detail/antiques.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <vector>

namespace boost { namespace mpi {

/**
 * @brief A @c group is a representation of a subset of the processes
 * within a @c communicator.
 *
 * The @c group class allows one to create arbitrary subsets of the
 * processes within a communicator. One can compute the union,
 * intersection, or difference of two groups, or create new groups by
 * specifically including or excluding certain processes. Given a
 * group, one can create a new communicator containing only the
 * processes in that group.
 */
class BOOST_MPI_DECL group
{
public:
  /**
   * @brief Constructs an empty group.
   */
  group() : group_ptr() { }

  /**
   * @brief Constructs a group from an @c MPI_Group.
   *
   * This routine allows one to construct a Boost.MPI @c group from a
   * C @c MPI_Group. The @c group object can (optionally) adopt the @c
   * MPI_Group, after which point the @c group object becomes
   * responsible for freeing the @c MPI_Group when the last copy of @c
   * group disappears.
   *
   * @param in_group The @c MPI_Group used to construct this @c group.
   *
   * @param adopt Whether the @c group should adopt the @c
   * MPI_Group. When true, the @c group object (or one of its copies)
   * will free the group (via @c MPI_Comm_free) when the last copy is
   * destroyed. Otherwise, the user is responsible for calling @c
   * MPI_Group_free.
   */
  group(const MPI_Group& in_group, bool adopt);

  /**
   * @brief Determine the rank of the calling process in the group.
   * 
   * This routine is equivalent to @c MPI_Group_rank.
   *
   * @returns The rank of the calling process in the group, which will
   * be a value in [0, size()). If the calling process is not in the
   * group, returns an empty value.
   */
  optional<int> rank() const;

  /**
   * @brief Determine the number of processes in the group.
   *
   * This routine is equivalent to @c MPI_Group_size.
   *
   * @returns The number of processes in the group.
   */
  int size() const;

  /**
   * @brief Translates the ranks from one group into the ranks of the
   * same processes in another group.
   *
   * This routine translates each of the integer rank values in the
   * iterator range @c [first, last) from the current group into rank
   * values of the corresponding processes in @p to_group. The
   * corresponding rank values are written via the output iterator @c
   * out. When there is no correspondence between a rank in the
   * current group and a rank in @c to_group, the value @c
   * MPI_UNDEFINED is written to the output iterator.
   *
   * @param first Beginning of the iterator range of ranks in the
   * current group.
   *
   * @param last Past the end of the iterator range of ranks in the
   * current group.
   *
   * @param to_group The group that we are translating ranks to.
   *
   * @param out The output iterator to which the translated ranks will
   * be written.
   *
   * @returns the output iterator, which points one step past the last
   * rank written.
   */
  template<typename InputIterator, typename OutputIterator>
  OutputIterator translate_ranks(InputIterator first, InputIterator last,
                                 const group& to_group, OutputIterator out);

  /**
   * @brief Determines whether the group is non-empty.
   *
   * @returns True if the group is not empty, false if it is empty.
   */
  operator bool() const { return (bool)group_ptr; }

  /**
   * @brief Retrieves the underlying @c MPI_Group associated with this
   * group.
   *
   * @returns The @c MPI_Group handle manipulated by this object. If
   * this object represents the empty group, returns @c
   * MPI_GROUP_EMPTY.
   */
  operator MPI_Group() const
  {
    if (group_ptr)
      return *group_ptr;
    else
      return MPI_GROUP_EMPTY;
  }

  /**
   *  @brief Creates a new group including a subset of the processes
   *  in the current group.
   *
   *  This routine creates a new @c group which includes only those
   *  processes in the current group that are listed in the integer
   *  iterator range @c [first, last). Equivalent to @c
   *  MPI_Group_incl.
   *
   *  @c first The beginning of the iterator range of ranks to include.
   *
   *  @c last Past the end of the iterator range of ranks to include.
   *
   *  @returns A new group containing those processes with ranks @c
   *  [first, last) in the current group.
   */
  template<typename InputIterator>
  group include(InputIterator first, InputIterator last);

  /**
   *  @brief Creates a new group from all of the processes in the
   *  current group, exluding a specific subset of the processes.
   *
   *  This routine creates a new @c group which includes all of the
   *  processes in the current group except those whose ranks are
   *  listed in the integer iterator range @c [first,
   *  last). Equivalent to @c MPI_Group_excl.
   *
   *  @c first The beginning of the iterator range of ranks to exclude.
   *
   *  @c last Past the end of the iterator range of ranks to exclude.
   *
   *  @returns A new group containing all of the processes in the
   *  current group except those processes with ranks @c [first, last)
   *  in the current group. 
   */
  template<typename InputIterator>
  group exclude(InputIterator first, InputIterator last);
  

protected:
  /**
   * INTERNAL ONLY
   *
   * Function object that frees an MPI group and deletes the
   * memory associated with it. Intended to be used as a deleter with
   * shared_ptr.
   */
  struct group_free
  {
    void operator()(MPI_Group* comm) const
    {
      int finalized;
      BOOST_MPI_CHECK_RESULT(MPI_Finalized, (&finalized));
      if (!finalized)
        BOOST_MPI_CHECK_RESULT(MPI_Group_free, (comm));
      delete comm;
    }
  };

  /**
   * The underlying MPI group. This is a shared pointer, so the actual
   * MPI group which will be shared among all related instances of the
   * @c group class. When there are no more such instances, the group
   * will be automatically freed.
   */
  shared_ptr<MPI_Group> group_ptr;
};

/**
 * @brief Determines whether two process groups are identical.
 *
 * Equivalent to calling @c MPI_Group_compare and checking whether the
 * result is @c MPI_IDENT.
 *
 * @returns True when the two process groups contain the same
 * processes in the same order.
 */
BOOST_MPI_DECL bool operator==(const group& g1, const group& g2);

/**
 * @brief Determines whether two process groups are not identical.
 *
 * Equivalent to calling @c MPI_Group_compare and checking whether the
 * result is not @c MPI_IDENT.
 *
 * @returns False when the two process groups contain the same
 * processes in the same order.
 */
inline bool operator!=(const group& g1, const group& g2)
{ 
  return !(g1 == g2);
}

/**
 * @brief Computes the union of two process groups.
 *
 * This routine returns a new @c group that contains all processes
 * that are either in group @c g1 or in group @c g2 (or both). The
 * processes that are in @c g1 will be first in the resulting group,
 * followed by the processes from @c g2 (but not also in @c
 * g1). Equivalent to @c MPI_Group_union.
 */
BOOST_MPI_DECL group operator|(const group& g1, const group& g2);

/**
 * @brief Computes the intersection of two process groups.
 *
 * This routine returns a new @c group that contains all processes
 * that are in group @c g1 and in group @c g2, ordered in the same way
 * as @c g1. Equivalent to @c MPI_Group_intersection.
 */
BOOST_MPI_DECL group operator&(const group& g1, const group& g2);

/**
 * @brief Computes the difference between two process groups.
 *
 * This routine returns a new @c group that contains all processes
 * that are in group @c g1 but not in group @c g2, ordered in the same way
 * as @c g1. Equivalent to @c MPI_Group_difference.
 */
BOOST_MPI_DECL group operator-(const group& g1, const group& g2);

/************************************************************************
 * Implementation details                                               *
 ************************************************************************/
template<typename InputIterator, typename OutputIterator>
OutputIterator 
group::translate_ranks(InputIterator first, InputIterator last,
                       const group& to_group, OutputIterator out)
{
  std::vector<int> in_array(first, last);
  if (in_array.empty())
    return out;

  std::vector<int> out_array(in_array.size());
  BOOST_MPI_CHECK_RESULT(MPI_Group_translate_ranks,
                         ((MPI_Group)*this,
                          in_array.size(),
                          detail::c_data(in_array),
                          (MPI_Group)to_group,
                          detail::c_data(out_array)));

  for (std::vector<int>::size_type i = 0, n = out_array.size(); i < n; ++i)
    *out++ = out_array[i];
  return out;
}

/**
 * INTERNAL ONLY
 * 
 * Specialization of translate_ranks that handles the one case where
 * we can avoid any memory allocation or copying.
 */
template<> 
BOOST_MPI_DECL int*
group::translate_ranks(int* first, int* last, const group& to_group, int* out);

template<typename InputIterator>
group group::include(InputIterator first, InputIterator last)
{
  if (first == last)
    return group();

  std::vector<int> ranks(first, last);
  MPI_Group result;
  BOOST_MPI_CHECK_RESULT(MPI_Group_incl,
                         ((MPI_Group)*this, ranks.size(), detail::c_data(ranks), &result));
  return group(result, /*adopt=*/true);
}

/**
 * INTERNAL ONLY
 * 
 * Specialization of group::include that handles the one case where we
 * can avoid any memory allocation or copying before creating the
 * group.
 */
template<> BOOST_MPI_DECL group group::include(int* first, int* last);

template<typename InputIterator>
group group::exclude(InputIterator first, InputIterator last)
{
  if (first == last)
    return group();

  std::vector<int> ranks(first, last);
  MPI_Group result;
  BOOST_MPI_CHECK_RESULT(MPI_Group_excl,
                         ((MPI_Group)*this, ranks.size(), detail::c_data(ranks), &result));
  return group(result, /*adopt=*/true);
}

/**
 * INTERNAL ONLY
 * 
 * Specialization of group::exclude that handles the one case where we
 * can avoid any memory allocation or copying before creating the
 * group.
 */
template<> BOOST_MPI_DECL group group::exclude(int* first, int* last);

} } // end namespace boost::mpi

#endif // BOOST_MPI_GROUP_HPP
