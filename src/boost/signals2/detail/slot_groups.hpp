// Boost.Signals2 library

// Copyright Frank Mori Hess 2007-2008.
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS2_SLOT_GROUPS_HPP
#define BOOST_SIGNALS2_SLOT_GROUPS_HPP

#include <boost/signals2/connection.hpp>
#include <boost/optional.hpp>
#include <list>
#include <map>
#include <utility>

namespace boost {
  namespace signals2 {
    namespace detail {
      enum slot_meta_group {front_ungrouped_slots, grouped_slots, back_ungrouped_slots};
      template<typename Group>
      struct group_key
      {
        typedef std::pair<enum slot_meta_group, boost::optional<Group> > type;
      };
      template<typename Group, typename GroupCompare>
      class group_key_less
      {
      public:
        group_key_less()
        {}
        group_key_less(const GroupCompare &group_compare): _group_compare(group_compare)
        {}
        bool operator ()(const typename group_key<Group>::type &key1, const typename group_key<Group>::type &key2) const
        {
          if(key1.first != key2.first) return key1.first < key2.first;
          if(key1.first != grouped_slots) return false;
          return _group_compare(key1.second.get(), key2.second.get());
        }
      private:
        GroupCompare _group_compare;
      };
      template<typename Group, typename GroupCompare, typename ValueType>
      class grouped_list
      {
      public:
        typedef group_key_less<Group, GroupCompare> group_key_compare_type;
      private:
        typedef std::list<ValueType> list_type;
        typedef std::map
          <
            typename group_key<Group>::type,
            typename list_type::iterator,
            group_key_compare_type
          > map_type;
        typedef typename map_type::iterator map_iterator;
        typedef typename map_type::const_iterator const_map_iterator;
      public:
        typedef typename list_type::iterator iterator;
        typedef typename list_type::const_iterator const_iterator;
        typedef typename group_key<Group>::type group_key_type;

        grouped_list(const group_key_compare_type &group_key_compare):
          _group_key_compare(group_key_compare)
        {}
        grouped_list(const grouped_list &other): _list(other._list),
          _group_map(other._group_map), _group_key_compare(other._group_key_compare)
        {
          // fix up _group_map
          typename map_type::const_iterator other_map_it;
          typename list_type::iterator this_list_it = _list.begin();
          typename map_type::iterator this_map_it = _group_map.begin();
          for(other_map_it = other._group_map.begin();
            other_map_it != other._group_map.end();
            ++other_map_it, ++this_map_it)
          {
            BOOST_ASSERT(this_map_it != _group_map.end());
            this_map_it->second = this_list_it;
            typename list_type::const_iterator other_list_it = other.get_list_iterator(other_map_it);
            typename map_type::const_iterator other_next_map_it = other_map_it;
            ++other_next_map_it;
            typename list_type::const_iterator other_next_list_it = other.get_list_iterator(other_next_map_it);
            while(other_list_it != other_next_list_it)
            {
              ++other_list_it;
              ++this_list_it;
            }
          }
        }
        iterator begin()
        {
          return _list.begin();
        }
        iterator end()
        {
          return _list.end();
        }
        iterator lower_bound(const group_key_type &key)
        {
          map_iterator map_it = _group_map.lower_bound(key);
          return get_list_iterator(map_it);
        }
        iterator upper_bound(const group_key_type &key)
        {
          map_iterator map_it = _group_map.upper_bound(key);
          return get_list_iterator(map_it);
        }
        void push_front(const group_key_type &key, const ValueType &value)
        {
          map_iterator map_it;
          if(key.first == front_ungrouped_slots)
          {// optimization
            map_it = _group_map.begin();
          }else
          {
            map_it = _group_map.lower_bound(key);
          }
          m_insert(map_it, key, value);
        }
        void push_back(const group_key_type &key, const ValueType &value)
        {
          map_iterator map_it;
          if(key.first == back_ungrouped_slots)
          {// optimization
            map_it = _group_map.end();
          }else
          {
            map_it = _group_map.upper_bound(key);
          }
          m_insert(map_it, key, value);
        }
        void erase(const group_key_type &key)
        {
          map_iterator map_it = _group_map.lower_bound(key);
          iterator begin_list_it = get_list_iterator(map_it);
          iterator end_list_it = upper_bound(key);
          if(begin_list_it != end_list_it)
          {
            _list.erase(begin_list_it, end_list_it);
            _group_map.erase(map_it);
          }
        }
        iterator erase(const group_key_type &key, const iterator &it)
        {
          BOOST_ASSERT(it != _list.end());
          map_iterator map_it = _group_map.lower_bound(key);
          BOOST_ASSERT(map_it != _group_map.end());
          BOOST_ASSERT(weakly_equivalent(map_it->first, key));
          if(map_it->second == it)
          {
            iterator next = it;
            ++next;
            // if next is in same group
            if(next != upper_bound(key))
            {
              _group_map[key] = next;
            }else
            {
              _group_map.erase(map_it);
            }
          }
          return _list.erase(it);
        }
        void clear()
        {
          _list.clear();
          _group_map.clear();
        }
      private:
        /* Suppress default assignment operator, since it has the wrong semantics. */
        grouped_list& operator=(const grouped_list &other);

        bool weakly_equivalent(const group_key_type &arg1, const group_key_type &arg2)
        {
          if(_group_key_compare(arg1, arg2)) return false;
          if(_group_key_compare(arg2, arg1)) return false;
          return true;
        }
        void m_insert(const map_iterator &map_it, const group_key_type &key, const ValueType &value)
        {
          iterator list_it = get_list_iterator(map_it);
          iterator new_it = _list.insert(list_it, value);
          if(map_it != _group_map.end() && weakly_equivalent(key, map_it->first))
          {
            _group_map.erase(map_it);
          }
          map_iterator lower_bound_it = _group_map.lower_bound(key);
          if(lower_bound_it == _group_map.end() ||
            weakly_equivalent(lower_bound_it->first, key) == false)
          {
            /* doing the following instead of just
              _group_map[key] = new_it;
              to avoid bogus error when enabling checked iterators with g++ */
            _group_map.insert(typename map_type::value_type(key, new_it));
          }
        }
        iterator get_list_iterator(const const_map_iterator &map_it)
        {
          iterator list_it;
          if(map_it == _group_map.end())
          {
            list_it = _list.end();
          }else
          {
            list_it = map_it->second;
          }
          return list_it;
        }
        const_iterator get_list_iterator(const const_map_iterator &map_it) const
        {
          const_iterator list_it;
          if(map_it == _group_map.end())
          {
            list_it = _list.end();
          }else
          {
            list_it = map_it->second;
          }
          return list_it;
        }

        list_type _list;
        // holds iterators to first list item in each group
        map_type _group_map;
        group_key_compare_type _group_key_compare;
      };
    } // end namespace detail
    enum connect_position { at_back, at_front };
  } // end namespace signals2
} // end namespace boost

#endif // BOOST_SIGNALS2_SLOT_GROUPS_HPP
