
// Copyright (C) 2005-2016 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered/detail/implementation.hpp>
#include <boost/unordered/unordered_set_fwd.hpp>

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename A, typename T, typename H, typename P> struct set
      {
        typedef boost::unordered::detail::set<A, T, H, P> types;

        typedef T value_type;
        typedef H hasher;
        typedef P key_equal;
        typedef T const const_key_type;

        typedef
          typename ::boost::unordered::detail::rebind_wrap<A, value_type>::type
            value_allocator;
        typedef boost::unordered::detail::allocator_traits<value_allocator>
          value_allocator_traits;

        typedef boost::unordered::detail::pick_node<A, value_type> pick;
        typedef typename pick::node node;
        typedef typename pick::bucket bucket;
        typedef typename pick::link_pointer link_pointer;

        typedef boost::unordered::detail::table<types> table;
        typedef boost::unordered::detail::set_extractor<value_type> extractor;

        typedef typename boost::unordered::detail::pick_policy<T>::type policy;

        typedef boost::unordered::iterator_detail::c_iterator<node> iterator;
        typedef boost::unordered::iterator_detail::c_iterator<node> c_iterator;
        typedef boost::unordered::iterator_detail::cl_iterator<node> l_iterator;
        typedef boost::unordered::iterator_detail::cl_iterator<node>
          cl_iterator;

        typedef boost::unordered::node_handle_set<node, T, A> node_type;
        typedef boost::unordered::insert_return_type_set<node, T, A>
          insert_return_type;
      };

      template <typename T, typename H, typename P, typename A>
      class instantiate_set
      {
        typedef boost::unordered_set<T, H, P, A> container;
        container x;
        typename container::node_type node_type;
        typename container::insert_return_type insert_return_type;
      };

      template <typename T, typename H, typename P, typename A>
      class instantiate_multiset
      {
        typedef boost::unordered_multiset<T, H, P, A> container;
        container x;
        typename container::node_type node_type;
      };
    }
  }
}
