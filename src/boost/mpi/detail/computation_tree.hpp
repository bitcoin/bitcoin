// Copyright (C) 2005 Douglas Gregor.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Compute parents, children, levels, etc. to effect a parallel
// computation tree.
#ifndef BOOST_MPI_COMPUTATION_TREE_HPP
#define BOOST_MPI_COMPUTATION_TREE_HPP

namespace boost { namespace mpi { namespace detail {

/**
 * @brief Aids tree-based parallel collective algorithms.
 *
 * Objects of this type
 */
class computation_tree
{
 public:
  computation_tree(int rank, int size, int root, int branching_factor = -1);

  /// Returns the branching factor of the tree.
  int branching_factor() const { return branching_factor_; }

  /// Returns the level in the tree on which this process resides.
  int level() const { return level_; }

  /**
   * Returns the index corresponding to the n^th level of the tree.
   *
   * @param n The level in the tree whose index will be returned.
   */
  int level_index(int n) const;

  /**
   *  @brief Returns the parent of this process.
   *
   *  @returns If this process is the root, returns itself. Otherwise,
   *  returns the process number that is the parent in the computation
   *  tree.
   */
  int parent() const;

  /// Returns the index for the first child of this process.
  int child_begin() const;

  /**
   * @brief The default branching factor within the computation tree.
   *
   * This is the default branching factor for the computation tree, to
   * be used by any computation tree that does not fix the branching
   * factor itself. The default is initialized to 3, but may be
   * changed by the application so long as all processes have the same
   * branching factor.
   */
  static int default_branching_factor;

 protected:
  /// The rank of this process in the computation tree.
  int rank;

  /// The number of processes participating in the computation tree.
  int size;

  /// The process number that is acting as the root in the computation
  /// tree.
  int root;

  /**
   * @brief The branching factor within the computation tree.
   *
   * This is the default number of children that each node in a
   * computation tree will have. This value will be used for
   * collective operations that use tree-based algorithms.
   */
  int branching_factor_;

  /// The level in the tree at which this process resides.
  int level_;
};

} } } // end namespace boost::mpi::detail

#endif // BOOST_MPI_COMPUTATION_TREE_HPP
