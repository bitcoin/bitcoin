//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2003-2005. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#ifndef BOOST_PTR_CONTAINER_PTR_SET_HPP
#define BOOST_PTR_CONTAINER_PTR_SET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/ptr_container/indirect_fun.hpp>
#include <boost/ptr_container/ptr_set_adapter.hpp>
#include <boost/ptr_container/detail/ptr_container_disable_deprecated.hpp>
#include <set>

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace boost
{

    template
    < 
        class Key, 
        class Compare        = std::less<Key>,
        class CloneAllocator = heap_clone_allocator,
        class Allocator      = std::allocator<typename ptr_container_detail::void_ptr<Key>::type>
    >
    class ptr_set : 
        public ptr_set_adapter< Key, std::set<
            typename ptr_container_detail::void_ptr<Key>::type,
            void_ptr_indirect_fun<Compare,Key>,Allocator>,
                                CloneAllocator, true >
    {
        typedef ptr_set_adapter< Key, std::set<
            typename ptr_container_detail::void_ptr<Key>::type,
            void_ptr_indirect_fun<Compare,Key>,Allocator>,
                                 CloneAllocator, true >
             base_type;

        typedef ptr_set<Key,Compare,CloneAllocator,Allocator> this_type;
        
    public:
        ptr_set()
        { }
        
        explicit ptr_set( const Compare& comp,
                          const Allocator& a = Allocator() ) 
         : base_type( comp, a ) 
        { }

        template< typename InputIterator >
        ptr_set( InputIterator first, InputIterator last )
         : base_type( first, last )
        { }

        template< typename InputIterator >
        ptr_set( InputIterator first, InputIterator last, 
                 const Compare& comp,
                 const Allocator& a = Allocator() ) 
         : base_type( first, last, comp, a )
        { }

        BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( ptr_set,
                                                      base_type,
                                                      this_type )
        
        BOOST_PTR_CONTAINER_DEFINE_COPY_CONSTRUCTORS( ptr_set, base_type )
                
    };
        
        
        
    template
    < 
        class Key, 
        class Compare        = std::less<Key>,
        class CloneAllocator = heap_clone_allocator,
        class Allocator      = std::allocator<void*>
    >
    class ptr_multiset : 
        public ptr_multiset_adapter< Key,
                                     std::multiset<void*,void_ptr_indirect_fun<Compare,Key>,Allocator>,
                                     CloneAllocator, true >
    {
        typedef ptr_multiset_adapter< Key,
                                      std::multiset<void*,void_ptr_indirect_fun<Compare,Key>,Allocator>,
                                      CloneAllocator, true >
              base_type;
        typedef ptr_multiset<Key,Compare,CloneAllocator,Allocator> this_type;
        
    public:
        ptr_multiset()
        { }
        
        explicit ptr_multiset( const Compare& comp,
                               const Allocator& a    = Allocator() )
         : base_type( comp, a ) 
        { }

        template< typename InputIterator >
        ptr_multiset( InputIterator first, InputIterator last )
         : base_type( first, last )
        { }

        template< typename InputIterator >
        ptr_multiset( InputIterator first, InputIterator last,
                      const Compare& comp,
                      const Allocator& a  = Allocator() )
         : base_type( first, last, comp, a ) 
        { }

        BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( ptr_multiset, 
                                                      base_type,
                                                      this_type )
        
        BOOST_PTR_CONTAINER_DEFINE_COPY_CONSTRUCTORS( ptr_multiset, 
                                                      base_type )     

    };

    /////////////////////////////////////////////////////////////////////////
    // clonability

    template< typename K, typename C, typename CA, typename A >
    inline ptr_set<K,C,CA,A>* new_clone( const ptr_set<K,C,CA,A>& r )
    {
        return r.clone().release();
    }

    template< typename K, typename C, typename CA, typename A >
    inline ptr_multiset<K,C,CA,A>* new_clone( const ptr_multiset<K,C,CA,A>& r )
    {
        return r.clone().release();
    }
    
    /////////////////////////////////////////////////////////////////////////
    // swap

    template< typename K, typename C, typename CA, typename A >
    inline void swap( ptr_set<K,C,CA,A>& l, ptr_set<K,C,CA,A>& r )
    {
        l.swap(r);
    }

    template< typename K, typename C, typename CA, typename A >
    inline void swap( ptr_multiset<K,C,CA,A>& l, ptr_multiset<K,C,CA,A>& r )
    {
        l.swap(r);
    }

}

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic pop
#endif

#endif
