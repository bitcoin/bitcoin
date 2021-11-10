// Copyright (C) 2006 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Jeremiah Willcock
//           Andrew Lumsdaine

// Distributed version of the two-bit color map
#ifndef BOOST_DISTRIBUTED_TWO_BIT_COLOR_MAP_HPP
#define BOOST_DISTRIBUTED_TWO_BIT_COLOR_MAP_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <boost/graph/two_bit_color_map.hpp>
#include <boost/property_map/parallel/distributed_property_map.hpp>
#include <boost/property_map/parallel/local_property_map.hpp>

namespace boost {

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
class two_bit_color_map<local_property_map<ProcessGroup,GlobalMap,StorageMap> >
  : public parallel::distributed_property_map<ProcessGroup, GlobalMap,
                                              two_bit_color_map<StorageMap> >
{
  typedef two_bit_color_map<StorageMap> local_map;

  typedef parallel::distributed_property_map<ProcessGroup, GlobalMap, 
                                             local_map >
    inherited;

  typedef local_property_map<ProcessGroup, GlobalMap, StorageMap>
    index_map_type;

public:
  two_bit_color_map(std::size_t inital_size, 
                    const index_map_type& index = index_map_type())
    : inherited(index.process_group(),  index.global(),
                local_map(inital_size, index.base())) { }

  inherited&       base()       { return *this; }
  const inherited& base() const { return *this; }
};

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
inline two_bit_color_type
get(two_bit_color_map<local_property_map<ProcessGroup,GlobalMap,StorageMap> >
      const& pm,
    typename property_traits<GlobalMap>::key_type key)
{
  return get(pm.base(), key);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
inline void
put(two_bit_color_map<local_property_map<ProcessGroup,GlobalMap,StorageMap> >
      const& pm, 
    typename property_traits<GlobalMap>::key_type key,
    two_bit_color_type value)
{
  put(pm.base(), key, value);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
class two_bit_color_map<parallel::distributed_property_map<
                          ProcessGroup, GlobalMap, StorageMap> > 
  : public parallel::distributed_property_map<
             ProcessGroup, GlobalMap, two_bit_color_map<StorageMap> >
{
  typedef two_bit_color_map<StorageMap> local_map;

  typedef parallel::distributed_property_map<ProcessGroup,GlobalMap,local_map>
    inherited;

  typedef parallel::distributed_property_map<ProcessGroup, GlobalMap,  
                                             StorageMap>
    index_map_type;

public:
  two_bit_color_map(std::size_t inital_size, 
                    const index_map_type& index = index_map_type())
    : inherited(index.process_group(),  index.global(),
                local_map(inital_size, index.base())) { }

  inherited&       base()       { return *this; }
  const inherited& base() const { return *this; }
};

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
inline two_bit_color_type
get(two_bit_color_map<
      parallel::distributed_property_map<
        ProcessGroup, GlobalMap, two_bit_color_map<StorageMap> > > const& pm,
    typename property_traits<GlobalMap>::key_type key)
{
  return get(pm.base(), key);
}

template<typename ProcessGroup, typename GlobalMap, typename StorageMap>
inline void
put(two_bit_color_map<
      parallel::distributed_property_map<
        ProcessGroup, GlobalMap, two_bit_color_map<StorageMap> > > const& pm, 
    typename property_traits<GlobalMap>::key_type key,
    two_bit_color_type value)
{
  put(pm.base(), key, value);
}

} // end namespace boost

#endif // BOOST_DISTRIBUTED_TWO_BIT_COLOR_MAP_HPP
