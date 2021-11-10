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

#ifndef BOOST_PTR_CONTAINER_PTR_SET_ADAPTER_HPP
#define BOOST_PTR_CONTAINER_PTR_SET_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/ptr_container/detail/associative_ptr_container.hpp>
#include <boost/ptr_container/detail/meta_functions.hpp>
#include <boost/ptr_container/detail/void_ptr_iterator.hpp>
#include <boost/ptr_container/detail/ptr_container_disable_deprecated.hpp>
#include <boost/range/iterator_range.hpp>

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace boost
{
namespace ptr_container_detail
{
    template
    < 
        class Key,
        class VoidPtrSet,
        bool  Ordered
    >
    struct set_config
    {
       typedef VoidPtrSet 
                    void_container_type;

       typedef BOOST_DEDUCED_TYPENAME VoidPtrSet::allocator_type 
                    allocator_type;

       typedef Key  value_type;

       typedef value_type 
                    key_type;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered, 
                          select_value_compare<VoidPtrSet>, 
                          mpl::identity<void> >::type
                    value_compare;

       typedef value_compare 
                    key_compare;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered,
                          mpl::identity<void>,
                          select_hasher<VoidPtrSet> >::type
                    hasher;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered,
                          mpl::identity<void>,
                          select_key_equal<VoidPtrSet> >::type
                    key_equal;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::if_c<Ordered,
                     ordered_associative_container_tag,
                     unordered_associative_container_tag>::type
                    container_type;

       typedef void_ptr_iterator<
                       BOOST_DEDUCED_TYPENAME VoidPtrSet::iterator, Key > 
                    iterator;

       typedef void_ptr_iterator<
                        BOOST_DEDUCED_TYPENAME VoidPtrSet::const_iterator, const Key >
                    const_iterator;

       typedef void_ptr_iterator<
           BOOST_DEDUCED_TYPENAME 
             mpl::eval_if_c<Ordered, 
                            select_iterator<VoidPtrSet>,
                            select_local_iterator<VoidPtrSet> >::type,
             Key >
                    local_iterator;

       typedef void_ptr_iterator<
           BOOST_DEDUCED_TYPENAME 
             mpl::eval_if_c<Ordered, 
                            select_iterator<VoidPtrSet>,
                            select_const_local_iterator<VoidPtrSet> >::type,
             const Key >
                    const_local_iterator;           
       
       template< class Iter >
       static Key* get_pointer( Iter i )
       {
           return static_cast<Key*>( *i.base() );
       }

       template< class Iter >
       static const Key* get_const_pointer( Iter i )
       {
           return static_cast<const Key*>( *i.base() );
       }

       BOOST_STATIC_CONSTANT(bool, allow_null = false );
    };

 
    
    template
    < 
        class Key,
        class VoidPtrSet, 
        class CloneAllocator = heap_clone_allocator,
        bool  Ordered        = true
    >
    class ptr_set_adapter_base 
        : public ptr_container_detail::associative_ptr_container< set_config<Key,VoidPtrSet,Ordered>,
                                                      CloneAllocator >
    {
        typedef ptr_container_detail::associative_ptr_container< set_config<Key,VoidPtrSet,Ordered>,
                                                     CloneAllocator >
              base_type;
    public:  
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator 
                      iterator;
        typedef BOOST_DEDUCED_TYPENAME base_type::const_iterator 
                      const_iterator;
        typedef Key   key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                      size_type;

    public:
        ptr_set_adapter_base() 
        { }

        template< class SizeType >
        ptr_set_adapter_base( SizeType n, 
                              ptr_container_detail::unordered_associative_container_tag tag )
          : base_type( n, tag )
        { }
                
        template< class Compare, class Allocator >
        ptr_set_adapter_base( const Compare& comp,
                              const Allocator& a ) 
         : base_type( comp, a ) 
        { }

        template< class Hash, class Pred, class Allocator >
        ptr_set_adapter_base( const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( hash, pred, a )
        { }

        template< class InputIterator, class Compare, class Allocator >
        ptr_set_adapter_base( InputIterator first, InputIterator last,
                              const Compare& comp,
                              const Allocator& a ) 
         : base_type( first, last, comp, a ) 
        { }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_set_adapter_base( InputIterator first, InputIterator last,
                              const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( first, last, hash, pred, a )
        { }
               
        template< class U, class Set, class CA, bool b >
        ptr_set_adapter_base( const ptr_set_adapter_base<U,Set,CA,b>& r )
          : base_type( r )
        { }

        ptr_set_adapter_base( const ptr_set_adapter_base& r )
          : base_type( r )
        { }
                
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        explicit ptr_set_adapter_base( std::auto_ptr<PtrContainer> clone )
         : base_type( clone )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        explicit ptr_set_adapter_base( std::unique_ptr<PtrContainer> clone )
         : base_type( std::move( clone ) )
        { }
#endif
        
        ptr_set_adapter_base& operator=( ptr_set_adapter_base r ) 
        {
            this->swap( r );
            return *this;
        }
        
#ifndef BOOST_NO_AUTO_PTR
        template< typename PtrContainer >
        ptr_set_adapter_base& operator=( std::auto_ptr<PtrContainer> clone )
        {
            base_type::operator=( clone );
            return *this;
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< typename PtrContainer >
        ptr_set_adapter_base& operator=( std::unique_ptr<PtrContainer> clone )
        {
            base_type::operator=( std::move( clone ) );
            return *this;
        }
#endif

        using base_type::erase;
        
        size_type erase( const key_type& x ) // nothrow
        {
            key_type* key = const_cast<key_type*>(&x);
            iterator i( this->base().find( key ) );       
            if( i == this->end() )                                  // nothrow
                return 0u;                                          // nothrow
            key = static_cast<key_type*>(*i.base());                // nothrow
            size_type res = this->base().erase( key );              // nothrow 
            this->remove( key );                                    // nothrow
            return res;
        }


        iterator find( const key_type& x )                                                
        {                                                                            
            return iterator( this->base().
                             find( const_cast<key_type*>(&x) ) );            
        }                                                                            

        const_iterator find( const key_type& x ) const                                    
        {                                                                            
            return const_iterator( this->base().
                                   find( const_cast<key_type*>(&x) ) );                  
        }                                                                            

        size_type count( const key_type& x ) const                                        
        {                                                                            
            return this->base().count( const_cast<key_type*>(&x) );                      
        }                                                                            
                                                                                     
        iterator lower_bound( const key_type& x )                                         
        {                                                                            
            return iterator( this->base().
                             lower_bound( const_cast<key_type*>(&x) ) );                   
        }                                                                            
                                                                                     
        const_iterator lower_bound( const key_type& x ) const                             
        {                                                                            
            return const_iterator( this->base().
                                   lower_bound( const_cast<key_type*>(&x) ) );       
        }                                                                            
                                                                                     
        iterator upper_bound( const key_type& x )                                         
        {                                                                            
            return iterator( this->base().
                             upper_bound( const_cast<key_type*>(&x) ) );           
        }                                                                            
                                                                                     
        const_iterator upper_bound( const key_type& x ) const                             
        {                                                                            
            return const_iterator( this->base().
                                   upper_bound( const_cast<key_type*>(&x) ) );             
        }                                                                            
                                                                                     
        iterator_range<iterator> equal_range( const key_type& x )                    
        {                                                                            
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_iterator,
                      BOOST_DEDUCED_TYPENAME base_type::ptr_iterator> 
                p = this->base().
                equal_range( const_cast<key_type*>(&x) );   
            return make_iterator_range( iterator( p.first ), 
                                        iterator( p.second ) );      
        }                                                                            
                                                                                     
        iterator_range<const_iterator> equal_range( const key_type& x ) const  
        {                                                                            
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_const_iterator,
                      BOOST_DEDUCED_TYPENAME base_type::ptr_const_iterator> 
                p = this->base().
                equal_range( const_cast<key_type*>(&x) ); 
            return make_iterator_range( const_iterator( p.first ), 
                                        const_iterator( p.second ) );    
        }    

    protected:
        size_type bucket( const key_type& key ) const
        {
            return this->base().bucket( const_cast<key_type*>(&key) );
        }
    };

} // ptr_container_detail

    /////////////////////////////////////////////////////////////////////////
    // ptr_set_adapter
    /////////////////////////////////////////////////////////////////////////
  
    template
    <
        class Key,
        class VoidPtrSet, 
        class CloneAllocator = heap_clone_allocator,
        bool  Ordered        = true
    >
    class ptr_set_adapter : 
        public ptr_container_detail::ptr_set_adapter_base<Key,VoidPtrSet,CloneAllocator,Ordered>
    {
        typedef ptr_container_detail::ptr_set_adapter_base<Key,VoidPtrSet,CloneAllocator,Ordered> 
            base_type;
    
    public: // typedefs
       
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator
                     iterator;                 
        typedef BOOST_DEDUCED_TYPENAME base_type::const_iterator
                     const_iterator;                 
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                     size_type;    
        typedef Key  key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type
                     auto_type;
        typedef BOOST_DEDUCED_TYPENAME VoidPtrSet::allocator_type
                     allocator_type;        
    private:
        
        template< typename II >                                               
        void set_basic_clone_and_insert( II first, II last ) // basic                 
        {                                                                     
            while( first != last )                                            
            {           
                if( this->find( *first ) == this->end() )
                    insert( this->null_policy_allocate_clone_from_iterator( first ) ); // strong, commit
                ++first;                                                      
            }                                                                 
        }                         

    public:
        ptr_set_adapter()
        { }

        template< class SizeType >
        ptr_set_adapter( SizeType n, 
                         ptr_container_detail::unordered_associative_container_tag tag )
          : base_type( n, tag )
        { }
        
        template< class Comp >
        explicit ptr_set_adapter( const Comp& comp,
                                  const allocator_type& a ) 
          : base_type( comp, a ) 
        {
            BOOST_ASSERT( this->empty() ); 
        }

        template< class SizeType, class Hash, class Pred, class Allocator >
        ptr_set_adapter( SizeType n,
                         const Hash& hash,
                         const Pred& pred,
                         const Allocator& a )
         : base_type( n, hash, pred, a )
        { }
        
        template< class Hash, class Pred, class Allocator >
        ptr_set_adapter( const Hash& hash,
                         const Pred& pred,
                         const Allocator& a )
         : base_type( hash, pred, a )
        { }

        template< class InputIterator >
        ptr_set_adapter( InputIterator first, InputIterator last )
         : base_type( first, last )
        { }

        template< class InputIterator, class Compare, class Allocator >
        ptr_set_adapter( InputIterator first, InputIterator last, 
                         const Compare& comp,
                         const Allocator a = Allocator() )
          : base_type( comp, a )
        {
            BOOST_ASSERT( this->empty() );
            set_basic_clone_and_insert( first, last );
        }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_set_adapter( InputIterator first, InputIterator last,
                         const Hash& hash,
                         const Pred& pred,
                         const Allocator& a )
          : base_type( first, last, hash, pred, a )
        { }

        explicit ptr_set_adapter( const ptr_set_adapter& r )
          : base_type( r )
        { }

        template< class U, class Set, class CA, bool b >
        explicit ptr_set_adapter( const ptr_set_adapter<U,Set,CA,b>& r )
          : base_type( r )
        { }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        explicit ptr_set_adapter( std::auto_ptr<PtrContainer> clone )
         : base_type( clone )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        explicit ptr_set_adapter( std::unique_ptr<PtrContainer> clone )
         : base_type( std::move( clone ) )
        { }
#endif

        template< class U, class Set, class CA, bool b >
        ptr_set_adapter& operator=( const ptr_set_adapter<U,Set,CA,b>& r ) 
        {
            base_type::operator=( r );
            return *this;
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class T >
        void operator=( std::auto_ptr<T> r )
        {
            base_type::operator=( r );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class T >
        void operator=( std::unique_ptr<T> r )
        {
            base_type::operator=( std::move( r ) );
        }
#endif

        std::pair<iterator,bool> insert( key_type* x ) // strong                      
        {       
            this->enforce_null_policy( x, "Null pointer in 'ptr_set::insert()'" );
            
            auto_type ptr( x, *this );                                
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_iterator,bool>
                 res = this->base().insert( x );       
            if( res.second )                                                 
                ptr.release();                                                  
            return std::make_pair( iterator( res.first ), res.second );     
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        std::pair<iterator,bool> insert( std::auto_ptr<U> x )
        {
            return insert( x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        std::pair<iterator,bool> insert( std::unique_ptr<U> x )
        {
            return insert( x.release() );
        }
#endif

        
        iterator insert( iterator where, key_type* x ) // strong
        {
            this->enforce_null_policy( x, "Null pointer in 'ptr_set::insert()'" );

            auto_type ptr( x, *this );                                
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator 
                res = this->base().insert( where.base(), x );
            if( *res == x )                                                 
                ptr.release();                                                  
            return iterator( res);
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( iterator where, std::auto_ptr<U> x )
        {
            return insert( where, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( iterator where, std::unique_ptr<U> x )
        {
            return insert( where, x.release() );
        }
#endif
        
        template< typename InputIterator >
        void insert( InputIterator first, InputIterator last ) // basic
        {
            set_basic_clone_and_insert( first, last );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    
        
        template< class Range >
        BOOST_DEDUCED_TYPENAME
        boost::disable_if< ptr_container_detail::is_pointer_or_integral<Range> >::type
        insert( const Range& r )
        {
            insert( boost::begin(r), boost::end(r) );
        }

#endif        

        template< class PtrSetAdapter >
        bool transfer( BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator object, 
                       PtrSetAdapter& from ) // strong
        {
            return this->single_transfer( object, from );
        }

        template< class PtrSetAdapter >
        size_type 
        transfer( BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator first, 
                  BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator last, 
                  PtrSetAdapter& from ) // basic
        {
            return this->single_transfer( first, last, from );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    

        template< class PtrSetAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                            BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator >,
                                                            size_type >::type
        transfer( const Range& r, PtrSetAdapter& from ) // basic
        {
            return transfer( boost::begin(r), boost::end(r), from );
        }

#endif

        template< class PtrSetAdapter >
        size_type transfer( PtrSetAdapter& from ) // basic
        {
            return transfer( from.begin(), from.end(), from );
        }

    };
    
    /////////////////////////////////////////////////////////////////////////
    // ptr_multiset_adapter
    /////////////////////////////////////////////////////////////////////////

    template
    < 
        class Key,
        class VoidPtrMultiSet, 
        class CloneAllocator = heap_clone_allocator,
        bool Ordered         = true 
    >
    class ptr_multiset_adapter : 
        public ptr_container_detail::ptr_set_adapter_base<Key,VoidPtrMultiSet,CloneAllocator,Ordered>
    {
         typedef ptr_container_detail::ptr_set_adapter_base<Key,VoidPtrMultiSet,CloneAllocator,Ordered> base_type;
    
    public: // typedefs
    
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator   
                       iterator;          
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                       size_type;
        typedef Key    key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type
                       auto_type;
        typedef BOOST_DEDUCED_TYPENAME VoidPtrMultiSet::allocator_type
                       allocator_type;        
    private:
        template< typename II >                                               
        void set_basic_clone_and_insert( II first, II last ) // basic                 
        {               
            while( first != last )                                            
            {           
                insert( this->null_policy_allocate_clone_from_iterator( first ) ); // strong, commit                              
                ++first;                                                     
            }                                                                 
        }                         
    
    public:
        ptr_multiset_adapter()
        { }

        template< class SizeType >
        ptr_multiset_adapter( SizeType n, 
                              ptr_container_detail::unordered_associative_container_tag tag )
          : base_type( n, tag )
        { }

        template< class Comp >
        explicit ptr_multiset_adapter( const Comp& comp,
                                       const allocator_type& a )
        : base_type( comp, a ) 
        { }

        template< class Hash, class Pred, class Allocator >
        ptr_multiset_adapter( const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( hash, pred, a )
        { }

        template< class InputIterator >
        ptr_multiset_adapter( InputIterator first, InputIterator last )
         : base_type( first, last )
        { }
        
        template< class InputIterator, class Comp >
        ptr_multiset_adapter( InputIterator first, InputIterator last,
                              const Comp& comp,
                              const allocator_type& a = allocator_type() )
        : base_type( comp, a ) 
        {
            set_basic_clone_and_insert( first, last );
        }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_multiset_adapter( InputIterator first, InputIterator last,
                              const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( first, last, hash, pred, a )
        { }
                
        template< class U, class Set, class CA, bool b >
        explicit ptr_multiset_adapter( const ptr_multiset_adapter<U,Set,CA,b>& r )
          : base_type( r )
        { }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        explicit ptr_multiset_adapter( std::auto_ptr<PtrContainer> clone )
         : base_type( clone )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        explicit ptr_multiset_adapter( std::unique_ptr<PtrContainer> clone )
         : base_type( std::move( clone ) )
        { }
#endif

        template< class U, class Set, class CA, bool b >
        ptr_multiset_adapter& operator=( const ptr_multiset_adapter<U,Set,CA,b>& r ) 
        {
            base_type::operator=( r );
            return *this;
        }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class T >
        void operator=( std::auto_ptr<T> r ) 
        {
            base_type::operator=( r ); 
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class T >
        void operator=( std::unique_ptr<T> r ) 
        {
            base_type::operator=( std::move( r ) ); 
        }
#endif

        iterator insert( iterator before, key_type* x ) // strong  
        {
            return base_type::insert( before, x ); 
        } 

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( iterator before, std::auto_ptr<U> x )
        {
            return insert( before, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( iterator before, std::unique_ptr<U> x )
        {
            return insert( before, x.release() );
        }
#endif
    
        iterator insert( key_type* x ) // strong                                      
        {   
            this->enforce_null_policy( x, "Null pointer in 'ptr_multiset::insert()'" );
    
            auto_type ptr( x, *this );                                
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator
                 res = this->base().insert( x );                         
            ptr.release();                                                      
            return iterator( res );                                             
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( std::auto_ptr<U> x )
        {
            return insert( x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( std::unique_ptr<U> x )
        {
            return insert( x.release() );
        }
#endif
    
        template< typename InputIterator >
        void insert( InputIterator first, InputIterator last ) // basic
        {
            set_basic_clone_and_insert( first, last );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    
        
        template< class Range >
        BOOST_DEDUCED_TYPENAME
        boost::disable_if< ptr_container_detail::is_pointer_or_integral<Range> >::type
        insert( const Range& r )
        {
            insert( boost::begin(r), boost::end(r) );
        }

#endif

        template< class PtrSetAdapter >
        void transfer( BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator object, 
                       PtrSetAdapter& from ) // strong
        {
            this->multi_transfer( object, from );
        }

        template< class PtrSetAdapter >
        size_type transfer( BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator first, 
                            BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator last, 
                            PtrSetAdapter& from ) // basic
        {
            return this->multi_transfer( first, last, from );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    
        
        template< class PtrSetAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                       BOOST_DEDUCED_TYPENAME PtrSetAdapter::iterator >, size_type >::type
        transfer(  const Range& r, PtrSetAdapter& from ) // basic
        {
            return transfer( boost::begin(r), boost::end(r), from );
        }

#endif        

        template< class PtrSetAdapter >
        void transfer( PtrSetAdapter& from ) // basic
        {
            transfer( from.begin(), from.end(), from );
            BOOST_ASSERT( from.empty() );
        }
        
    };

} // namespace 'boost'  

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic pop
#endif

#endif
