// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_GRAPH_DISTRIBUTED_SELECTOR_HPP
#define BOOST_GRAPH_DISTRIBUTED_SELECTOR_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/detail/is_distributed_selector.hpp>

namespace boost { 

  /* The default local selector for a distributedS selector. */
  struct defaultS {};

  /**
   * Selector that specifies that the graph should be distributed
   * among different processes organized based on the given process
   * group.
   */
  template<typename ProcessGroup, typename LocalS = defaultS,
           typename DistributionS = defaultS>
  struct distributedS 
  {
    typedef ProcessGroup process_group_type;
    typedef LocalS local_selector;
    typedef DistributionS distribution;
  };

  namespace detail {
    template<typename ProcessGroup, typename LocalS, typename DistributionS>
    struct is_distributed_selector<distributedS<ProcessGroup, LocalS, DistributionS> >: mpl::true_ {};
  }

}

#endif // BOOST_GRAPH_DISTRIBUTED_SELECTOR_HPP
