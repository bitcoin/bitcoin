// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#ifndef BOOST_PARALLEL_CACHING_PROPERTY_MAP_HPP
#define BOOST_PARALLEL_CACHING_PROPERTY_MAP_HPP

#include <boost/property_map/property_map.hpp>

namespace boost { 

// This probably doesn't belong here
template<typename Key, typename Value>
inline void local_put(dummy_property_map, const Key&, const Value&) {}

namespace parallel {

/** Property map that caches values placed in it but does not
 * broadcast values to remote processors.  This class template is
 * meant as an adaptor for @ref distributed_property_map that
 * suppresses communication in the event of a remote @c put operation
 * by mapping it to a local @c put operation.
 *
 * @todo Find a better name for @ref caching_property_map
 */
template<typename PropertyMap>
class caching_property_map
{
public:
  typedef typename property_traits<PropertyMap>::key_type   key_type;
  typedef typename property_traits<PropertyMap>::value_type value_type;
  typedef typename property_traits<PropertyMap>::reference  reference;
  typedef typename property_traits<PropertyMap>::category   category;

  explicit caching_property_map(const PropertyMap& property_map)
    : property_map(property_map) {}

  PropertyMap&        base()       { return property_map; }
  const PropertyMap&  base() const { return property_map; }

  template<typename Reduce>
  void set_reduce(const Reduce& reduce)
  { property_map.set_reduce(reduce); }

  void reset() { property_map.reset(); }

#if 0
  reference operator[](const key_type& key) const
  {
    return property_map[key];
  }
#endif

private:
  PropertyMap property_map;
};

template<typename PropertyMap, typename Key>
inline typename caching_property_map<PropertyMap>::value_type
get(const caching_property_map<PropertyMap>& pm, const Key& key)
{ return get(pm.base(), key); }

template<typename PropertyMap, typename Key, typename Value>
inline void
local_put(const caching_property_map<PropertyMap>& pm, const Key& key,
          const Value& value)
{ local_put(pm.base(), key, value); }

template<typename PropertyMap, typename Key, typename Value>
inline void
cache(const caching_property_map<PropertyMap>& pm, const Key& key,
      const Value& value)
{ cache(pm.base(), key, value); }

template<typename PropertyMap, typename Key, typename Value>
inline void
put(const caching_property_map<PropertyMap>& pm, const Key& key,
    const Value& value)
{ local_put(pm.base(), key, value); }

template<typename PropertyMap>
inline caching_property_map<PropertyMap>
make_caching_property_map(const PropertyMap& pm)
{ return caching_property_map<PropertyMap>(pm); }

} } // end namespace boost::parallel

#endif // BOOST_PARALLEL_CACHING_PROPERTY_MAP_HPP
