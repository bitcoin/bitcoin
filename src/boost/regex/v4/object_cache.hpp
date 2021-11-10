/*
 *
 * Copyright (c) 2004
 * John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         object_cache.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Implements a generic object cache.
  */

#ifndef BOOST_REGEX_OBJECT_CACHE_HPP
#define BOOST_REGEX_OBJECT_CACHE_HPP

#include <boost/regex/config.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <list>
#include <stdexcept>
#include <string>
#ifdef BOOST_HAS_THREADS
#include <boost/regex/pending/static_mutex.hpp>
#endif

namespace boost{

template <class Key, class Object>
class object_cache
{
public:
   typedef std::pair< ::boost::shared_ptr<Object const>, Key const*> value_type;
   typedef std::list<value_type> list_type;
   typedef typename list_type::iterator list_iterator;
   typedef std::map<Key, list_iterator> map_type;
   typedef typename map_type::iterator map_iterator;
   typedef typename list_type::size_type size_type;
   static boost::shared_ptr<Object const> get(const Key& k, size_type l_max_cache_size);

private:
   static boost::shared_ptr<Object const> do_get(const Key& k, size_type l_max_cache_size);

   struct data
   {
      list_type   cont;
      map_type    index;
   };

   // Needed by compilers not implementing the resolution to DR45. For reference,
   // see http://www.open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#45.
   friend struct data;
};

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable: 4702)
#endif
template <class Key, class Object>
boost::shared_ptr<Object const> object_cache<Key, Object>::get(const Key& k, size_type l_max_cache_size)
{
#ifdef BOOST_HAS_THREADS
   static boost::static_mutex mut = BOOST_STATIC_MUTEX_INIT;
   boost::static_mutex::scoped_lock l(mut);
   if (l)
   {
      return do_get(k, l_max_cache_size);
   }
   //
   // what do we do if the lock fails?
   // for now just throw, but we should never really get here...
   //
   ::boost::throw_exception(std::runtime_error("Error in thread safety code: could not acquire a lock"));
#if defined(BOOST_NO_UNREACHABLE_RETURN_DETECTION) || defined(BOOST_NO_EXCEPTIONS)
   return boost::shared_ptr<Object>();
#endif
#else
   return do_get(k, l_max_cache_size);
#endif
}
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

template <class Key, class Object>
boost::shared_ptr<Object const> object_cache<Key, Object>::do_get(const Key& k, size_type l_max_cache_size)
{
   typedef typename object_cache<Key, Object>::data object_data;
   typedef typename map_type::size_type map_size_type;
   static object_data s_data;

   //
   // see if the object is already in the cache:
   //
   map_iterator mpos = s_data.index.find(k);
   if(mpos != s_data.index.end())
   {
      //
      // Eureka! 
      // We have a cached item, bump it up the list and return it:
      //
      if(--(s_data.cont.end()) != mpos->second)
      {
         // splice out the item we want to move:
         list_type temp;
         temp.splice(temp.end(), s_data.cont, mpos->second);
         // and now place it at the end of the list:
         s_data.cont.splice(s_data.cont.end(), temp, temp.begin());
         BOOST_REGEX_ASSERT(*(s_data.cont.back().second) == k);
         // update index with new position:
         mpos->second = --(s_data.cont.end());
         BOOST_REGEX_ASSERT(&(mpos->first) == mpos->second->second);
         BOOST_REGEX_ASSERT(&(mpos->first) == s_data.cont.back().second);
      }
      return s_data.cont.back().first;
   }
   //
   // if we get here then the item is not in the cache,
   // so create it:
   //
   boost::shared_ptr<Object const> result(new Object(k));
   //
   // Add it to the list, and index it:
   //
   s_data.cont.push_back(value_type(result, static_cast<Key const*>(0)));
   s_data.index.insert(std::make_pair(k, --(s_data.cont.end())));
   s_data.cont.back().second = &(s_data.index.find(k)->first);
   map_size_type s = s_data.index.size();
   BOOST_REGEX_ASSERT(s_data.index[k]->first.get() == result.get());
   BOOST_REGEX_ASSERT(&(s_data.index.find(k)->first) == s_data.cont.back().second);
   BOOST_REGEX_ASSERT(s_data.index.find(k)->first == k);
   if(s > l_max_cache_size)
   {
      //
      // We have too many items in the list, so we need to start
      // popping them off the back of the list, but only if they're
      // being held uniquely by us:
      //
      list_iterator pos = s_data.cont.begin();
      list_iterator last = s_data.cont.end();
      while((pos != last) && (s > l_max_cache_size))
      {
         if(pos->first.unique())
         {
            list_iterator condemmed(pos);
            ++pos;
            // now remove the items from our containers, 
            // then order has to be as follows:
            BOOST_REGEX_ASSERT(s_data.index.find(*(condemmed->second)) != s_data.index.end());
            s_data.index.erase(*(condemmed->second));
            s_data.cont.erase(condemmed); 
            --s;
         }
         else
            ++pos;
      }
      BOOST_REGEX_ASSERT(s_data.index[k]->first.get() == result.get());
      BOOST_REGEX_ASSERT(&(s_data.index.find(k)->first) == s_data.cont.back().second);
      BOOST_REGEX_ASSERT(s_data.index.find(k)->first == k);
   }
   return result;
}

}

#endif
