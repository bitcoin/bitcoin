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

#ifndef BOOST_PTR_CONTAINER_DETAIL_PTR_MAP_ADAPTER_HPP
#define BOOST_PTR_CONTAINER_DETAIL_PTR_MAP_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/ptr_container/detail/map_iterator.hpp>
#include <boost/ptr_container/detail/associative_ptr_container.hpp>
#include <boost/ptr_container/detail/meta_functions.hpp>
#include <boost/ptr_container/detail/ptr_container_disable_deprecated.hpp>
#include <boost/static_assert.hpp>
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
        class T,
        class VoidPtrMap,
        bool  Ordered
    >
    struct map_config
    {
        typedef BOOST_DEDUCED_TYPENAME remove_nullable<T>::type
                     U;
        typedef VoidPtrMap 
                     void_container_type;
        
        typedef BOOST_DEDUCED_TYPENAME VoidPtrMap::allocator_type
                     allocator_type;
        
       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered, 
                          select_value_compare<VoidPtrMap>, 
                          mpl::identity<void> >::type
                    value_compare;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered, 
                          select_key_compare<VoidPtrMap>, 
                          mpl::identity<void> >::type
                    key_compare;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered,
                          mpl::identity<void>,
                          select_hasher<VoidPtrMap> >::type
                    hasher;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::eval_if_c<Ordered,
                          mpl::identity<void>,
                          select_key_equal<VoidPtrMap> >::type
                    key_equal;

       typedef BOOST_DEDUCED_TYPENAME 
           mpl::if_c<Ordered,
                     ptr_container_detail::ordered_associative_container_tag,
                     ptr_container_detail::unordered_associative_container_tag>::type
                    container_type;
        
        typedef BOOST_DEDUCED_TYPENAME VoidPtrMap::key_type
                     key_type;
        
        typedef U    value_type;

        typedef ptr_map_iterator< BOOST_DEDUCED_TYPENAME VoidPtrMap::iterator, key_type, U* const >
                     iterator;
        
        typedef ptr_map_iterator< BOOST_DEDUCED_TYPENAME VoidPtrMap::const_iterator, key_type, const U* const>
                     const_iterator;
        
        typedef ptr_map_iterator<
           BOOST_DEDUCED_TYPENAME 
             mpl::eval_if_c<Ordered, 
                            select_iterator<VoidPtrMap>,
                            select_local_iterator<VoidPtrMap> >::type,
             key_type, U* const >
                    local_iterator;

       typedef ptr_map_iterator<
           BOOST_DEDUCED_TYPENAME 
             mpl::eval_if_c<Ordered, 
                            select_iterator<VoidPtrMap>,
                            select_const_local_iterator<VoidPtrMap> >::type,
             key_type, const U* const >
                    const_local_iterator;  
       
        template< class Iter >
        static U* get_pointer( Iter i )
        {
            return i->second;
        }

        template< class Iter >
        static const U* get_const_pointer( Iter i )
        {
            return i->second;
        }

        BOOST_STATIC_CONSTANT( bool, allow_null = boost::is_nullable<T>::value );
    };
    
    

    template
    < 
        class T,
        class VoidPtrMap, 
        class CloneAllocator,
        bool  Ordered
    >
    class ptr_map_adapter_base : 
        public ptr_container_detail::associative_ptr_container< map_config<T,VoidPtrMap,Ordered>,
                                                    CloneAllocator >
    {
        typedef ptr_container_detail::associative_ptr_container< map_config<T,VoidPtrMap,Ordered>,
                                                     CloneAllocator > 
            base_type;

        typedef map_config<T,VoidPtrMap,Ordered>                           config;
        typedef ptr_map_adapter_base<T,VoidPtrMap,CloneAllocator,Ordered>  this_type;
        
    public:

        typedef BOOST_DEDUCED_TYPENAME base_type::allocator_type
                    allocator_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator
                    iterator;
        typedef BOOST_DEDUCED_TYPENAME base_type::const_iterator
                    const_iterator;
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                    size_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::key_type
                    key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type
                    auto_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::value_type 
                    mapped_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::reference
                    mapped_reference;
        typedef BOOST_DEDUCED_TYPENAME base_type::const_reference
                    const_mapped_reference;
        typedef BOOST_DEDUCED_TYPENAME iterator_value<iterator>::type
                    value_type;
        typedef value_type
                    reference;
        typedef BOOST_DEDUCED_TYPENAME iterator_value<const_iterator>::type
                    const_reference;
        typedef value_type 
                    pointer;
        typedef const_reference 
                    const_pointer;

    private:
        const_mapped_reference lookup( const key_type& key ) const
        {
           const_iterator i = this->find( key );
           BOOST_PTR_CONTAINER_THROW_EXCEPTION( i == this->end(), bad_ptr_container_operation,
                                                "'ptr_map/multimap::at()' could"
                                                " not find key" );
           return *i->second;
        }

        struct eraser // scope guard
        {
            bool            released_;
            VoidPtrMap*     m_;
            const key_type& key_;

            eraser( VoidPtrMap* m, const key_type& key ) 
              : released_(false), m_(m), key_(key)
            {}

            ~eraser() 
            {
                if( !released_ )
                    m_->erase(key_);
            }

            void release() { released_ = true; }

        private:  
            eraser& operator=(const eraser&);  
        };

        mapped_reference insert_lookup( const key_type& key )
        {
            void*& ref = this->base()[key];
            if( ref )
            {
                return *static_cast<mapped_type>(ref);
            }
            else
            {
                eraser e(&this->base(),key);      // nothrow
                mapped_type res = new T();        // strong 
                ref = res;                        // nothrow
                e.release();                      // nothrow
                return *res;
            }
          }
        
    public:

        ptr_map_adapter_base()
        { }

        template< class SizeType >
        explicit ptr_map_adapter_base( SizeType n, 
                                       ptr_container_detail::unordered_associative_container_tag tag )
        : base_type( n, tag )
        { }

        template< class Compare, class Allocator >
        ptr_map_adapter_base( const Compare& comp,
                              const Allocator& a ) 
        : base_type( comp, a ) 
        { }

        template< class Hash, class Pred, class Allocator >
        ptr_map_adapter_base( const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( hash, pred, a )
        { }
                
        template< class InputIterator >
        ptr_map_adapter_base( InputIterator first, InputIterator last )
         : base_type( first, last )
        { }
                
        template< class InputIterator, class Comp >
        ptr_map_adapter_base( InputIterator first, InputIterator last,
                              const Comp& comp,
                              const allocator_type& a = allocator_type() )
        : base_type( first, last, comp, a )
        { }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_map_adapter_base( InputIterator first, InputIterator last,
                              const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( first, last, hash, pred, a )
        { }
                
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        explicit ptr_map_adapter_base( std::auto_ptr<PtrContainer> clone )
        : base_type( clone )
        { }

        template< typename PtrContainer >
        ptr_map_adapter_base& operator=( std::auto_ptr<PtrContainer> clone )
        {
            base_type::operator=( clone );
            return *this;
        }        
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        explicit ptr_map_adapter_base( std::unique_ptr<PtrContainer> clone )
        : base_type( std::move( clone ) )
        { }

        template< typename PtrContainer >
        ptr_map_adapter_base& operator=( std::unique_ptr<PtrContainer> clone )
        {
            base_type::operator=( std::move( clone ) );
            return *this;
        }
#endif

        iterator find( const key_type& x )                                                
        {                                                                            
            return iterator( this->base().find( x ) );                                
        }                                                                            

        const_iterator find( const key_type& x ) const                                    
        {                                                                            
            return const_iterator( this->base().find( x ) );                          
        }                                                                            

        size_type count( const key_type& x ) const                                        
        {                                                                            
            return this->base().count( x );                                           
        }                                                                            
                                                                                     
        iterator lower_bound( const key_type& x )                                         
        {                                                                            
            return iterator( this->base().lower_bound( x ) );                         
        }                                                                            
                                                                                     
        const_iterator lower_bound( const key_type& x ) const                             
        {                                                                            
            return const_iterator( this->base().lower_bound( x ) );                   
        }                                                                            
                                                                                     
        iterator upper_bound( const key_type& x )                                         
        {                                                                            
            return iterator( this->base().upper_bound( x ) );                         
        }                                                                            
                                                                                     
        const_iterator upper_bound( const key_type& x ) const                             
        {                                                                            
            return const_iterator( this->base().upper_bound( x ) );                   
        }                                                                            
                                                                                     
        iterator_range<iterator> equal_range( const key_type& x )                    
        {                                                                            
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_iterator,
                      BOOST_DEDUCED_TYPENAME base_type::ptr_iterator>
                 p = this->base().equal_range( x );   
            return make_iterator_range( iterator( p.first ), iterator( p.second ) );      
        }                                                                            
                                                                                     
        iterator_range<const_iterator> equal_range( const key_type& x ) const  
        {                                                                            
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_const_iterator,
                      BOOST_DEDUCED_TYPENAME base_type::ptr_const_iterator> 
                p = this->base().equal_range( x ); 
            return make_iterator_range( const_iterator( p.first ), 
                                        const_iterator( p.second ) );    
        }                                                                            
                                                                                     
        mapped_reference at( const key_type& key )  
        {   
            return const_cast<mapped_reference>( lookup( key ) ); 
        }
                                                                                     
        const_mapped_reference at( const key_type& key ) const
        {                                                                            
            return lookup( key );
        }

        mapped_reference operator[]( const key_type& key )
        {
            return insert_lookup( key );
        }              

        auto_type replace( iterator where, mapped_type x ) // strong  
        { 
            BOOST_ASSERT( where != this->end() );
            this->enforce_null_policy( x, "Null pointer in 'replace()'" );

            auto_type ptr( x, *this );
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( this->empty(),
                                                 bad_ptr_container_operation,
                                                 "'replace()' on empty container" );

            auto_type old( where->second, *this ); // nothrow
            where.base()->second = ptr.release();  // nothrow, commit
            return boost::ptr_container::move( old );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        auto_type replace( iterator where, std::auto_ptr<U> x )
        {
            return replace( where, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        auto_type replace( iterator where, std::unique_ptr<U> x )
        {
            return replace( where, x.release() );
        }
#endif

    protected:
        size_type bucket( const key_type& key ) const
        {
            return this->base().bucket( key );
        }
    };
    
} // ptr_container_detail

    /////////////////////////////////////////////////////////////////////////
    // ptr_map_adapter
    /////////////////////////////////////////////////////////////////////////
    
    template
    < 
        class T,
        class VoidPtrMap, 
        class CloneAllocator = heap_clone_allocator,
        bool  Ordered        = true
    >
    class ptr_map_adapter : 
        public ptr_container_detail::ptr_map_adapter_base<T,VoidPtrMap,CloneAllocator,Ordered>
    {
        typedef ptr_container_detail::ptr_map_adapter_base<T,VoidPtrMap,CloneAllocator,Ordered> 
            base_type;
    
    public:    
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator 
                     iterator;       
        typedef BOOST_DEDUCED_TYPENAME base_type::const_iterator
                     const_iterator;
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                    size_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::key_type
                    key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::const_reference
                    const_reference;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type
                    auto_type;
        typedef BOOST_DEDUCED_TYPENAME VoidPtrMap::allocator_type 
                    allocator_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::mapped_type
                    mapped_type;
    private:

        void safe_insert( const key_type& key, auto_type ptr ) // strong
        {
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_iterator,bool>
                res = 
                this->base().insert( std::make_pair( key, ptr.get() ) ); // strong, commit      
            if( res.second )                                             // nothrow     
                ptr.release();                                           // nothrow
        }

        template< class II >                                               
        void map_basic_clone_and_insert( II first, II last )                  
        {       
            while( first != last )                                            
            {                                            
                if( this->find( first->first ) == this->end() )
                {
                    const_reference p = *first.base();     // nothrow                    
                    auto_type ptr( this->null_policy_allocate_clone(p.second), *this ); 
                                                           // strong 
                    this->safe_insert( p.first, 
                                       boost::ptr_container::move( ptr ) );
                                                           // strong, commit 
                }
                ++first;                                                      
            }                                                                 
        }
    
    public:
        ptr_map_adapter( )
        { }

        template< class Comp >
        explicit ptr_map_adapter( const Comp& comp,
                                  const allocator_type& a ) 
          : base_type( comp, a ) { }

        template< class SizeType >
        explicit ptr_map_adapter( SizeType n,
            ptr_container_detail::unordered_associative_container_tag tag ) 
          : base_type( n, tag ) { }

        template< class Hash, class Pred, class Allocator >
        ptr_map_adapter( const Hash& hash,
                         const Pred& pred,
                         const Allocator& a )
         : base_type( hash, pred, a )
        { }
                
        template< class InputIterator >
        ptr_map_adapter( InputIterator first, InputIterator last )
        {
            map_basic_clone_and_insert( first, last ); 
        }
               
        template< class InputIterator, class Comp >
        ptr_map_adapter( InputIterator first, InputIterator last, 
                         const Comp& comp,
                         const allocator_type& a = allocator_type() )
          : base_type( comp, a ) 
        {
            map_basic_clone_and_insert( first, last );
        }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_map_adapter( InputIterator first, InputIterator last,
                         const Hash& hash,
                         const Pred& pred,
                         const Allocator& a )
          : base_type( hash, pred, a )
        {
            map_basic_clone_and_insert( first, last ); 
        }
                
        ptr_map_adapter( const ptr_map_adapter& r )
        {
            map_basic_clone_and_insert( r.begin(), r.end() );      
        }
        
        template< class Key, class U, class CA, bool b >
        ptr_map_adapter( const ptr_map_adapter<Key,U,CA,b>& r )
        {
            map_basic_clone_and_insert( r.begin(), r.end() );      
        }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        ptr_map_adapter( std::auto_ptr<U> r ) : base_type( r )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        ptr_map_adapter( std::unique_ptr<U> r ) : base_type( std::move( r ) )
        { }
#endif

        ptr_map_adapter& operator=( ptr_map_adapter r )
        {
            this->swap( r );
            return *this;
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        ptr_map_adapter& operator=( std::auto_ptr<U> r )
        {
            base_type::operator=( r );
            return *this;
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        ptr_map_adapter& operator=( std::unique_ptr<U> r )
        {
            base_type::operator=( std::move( r ) );
            return *this;
        }
#endif

        using base_type::release;

        template< typename InputIterator >
        void insert( InputIterator first, InputIterator last ) // basic
        {
            map_basic_clone_and_insert( first, last );
        }

        template< class Range >
        void insert( const Range& r )
        {
            insert( boost::begin(r), boost::end(r) );
        }

    private:
        std::pair<iterator,bool> insert_impl( const key_type& key, mapped_type x ) // strong
        {
            this->enforce_null_policy( x, "Null pointer in ptr_map_adapter::insert()" );

            auto_type ptr( x, *this );                                  // nothrow
            std::pair<BOOST_DEDUCED_TYPENAME base_type::ptr_iterator,bool>
                 res = this->base().insert( std::make_pair( key, x ) ); // strong, commit      
            if( res.second )                                            // nothrow     
                ptr.release();                                          // nothrow
            return std::make_pair( iterator( res.first ), res.second ); // nothrow        
        }

        iterator insert_impl( iterator before, const key_type& key, mapped_type x ) // strong
        {
            this->enforce_null_policy( x, 
                  "Null pointer in 'ptr_map_adapter::insert()'" );
            
            auto_type ptr( x, *this );  // nothrow
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator
                res = this->base().insert( before.base(), std::make_pair( key, x ) );
                                        // strong, commit        
            ptr.release();              // notrow
            return iterator( res );                       
        }
        
    public:
        
        std::pair<iterator,bool> insert( key_type& key, mapped_type x )
        {
            return insert_impl( key, x );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        std::pair<iterator,bool> insert( const key_type& key, std::auto_ptr<U> x )
        {
            return insert_impl( key, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        std::pair<iterator,bool> insert( const key_type& key, std::unique_ptr<U> x )
        {
            return insert_impl( key, x.release() );
        }
#endif

        template< class F, class S >
        iterator insert( iterator before, ptr_container_detail::ref_pair<F,S> p ) // strong
        {
            this->enforce_null_policy( p.second, 
                  "Null pointer in 'ptr_map_adapter::insert()'" );
 
            auto_type ptr( this->null_policy_allocate_clone(p.second), *this ); 
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator
                result = this->base().insert( before.base(), 
                                     std::make_pair(p.first,ptr.get()) ); // strong
            if( ptr.get() == result->second )
                ptr.release();
    
            return iterator( result );
        }

        iterator insert( iterator before, key_type& key, mapped_type x ) // strong
        {
            return insert_impl( before, key, x );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( iterator before, const key_type& key, std::auto_ptr<U> x ) // strong
        {
            return insert_impl( before, key, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( iterator before, const key_type& key, std::unique_ptr<U> x ) // strong
        {
            return insert_impl( before, key, x.release() );
        }
#endif
        
        template< class PtrMapAdapter >
        bool transfer( BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator object, 
                       PtrMapAdapter& from ) // strong
        {
            return this->single_transfer( object, from );
        }

        template< class PtrMapAdapter >
        size_type transfer( BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator first, 
                            BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator last, 
                            PtrMapAdapter& from ) // basic
        {
            return this->single_transfer( first, last, from );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    

        template< class PtrMapAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                            BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator >,
                                                            size_type >::type
        transfer( const Range& r, PtrMapAdapter& from ) // basic
        {
            return transfer( boost::begin(r), boost::end(r), from );
        }
        
#endif

        template< class PtrMapAdapter >
        size_type transfer( PtrMapAdapter& from ) // basic
        {
            return transfer( from.begin(), from.end(), from );
        }
  };
  
  /////////////////////////////////////////////////////////////////////////
  // ptr_multimap_adapter
  /////////////////////////////////////////////////////////////////////////

    template
    < 
        class T,
        class VoidPtrMultiMap, 
        class CloneAllocator = heap_clone_allocator,
        bool  Ordered        = true
    >
    class ptr_multimap_adapter : 
        public ptr_container_detail::ptr_map_adapter_base<T,VoidPtrMultiMap,CloneAllocator,Ordered>
    {
        typedef ptr_container_detail::ptr_map_adapter_base<T,VoidPtrMultiMap,CloneAllocator,Ordered>
             base_type;

    public: // typedefs
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator           
                       iterator;                 
        typedef BOOST_DEDUCED_TYPENAME base_type::const_iterator     
                       const_iterator;           
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type
                       size_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::key_type
                       key_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::const_reference
                       const_reference;
        typedef BOOST_DEDUCED_TYPENAME base_type::mapped_type
                    mapped_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type
                    auto_type;            
        typedef BOOST_DEDUCED_TYPENAME VoidPtrMultiMap::allocator_type 
                    allocator_type;
    private:

        void safe_insert( const key_type& key, auto_type ptr ) // strong
        {
            this->base().insert( 
                           std::make_pair( key, ptr.get() ) ); // strong, commit      
            ptr.release();                                     // nothrow
        }
        
        template< typename II >                                               
        void map_basic_clone_and_insert( II first, II last )                  
        {                                                         
            while( first != last )                                            
            {                                            
                const_reference pair = *first.base();     // nothrow                     
                auto_type ptr( this->null_policy_allocate_clone(pair.second), *this );    
                                                          // strong
                safe_insert( pair.first, 
                             boost::ptr_container::move( ptr ) );
                                                          // strong, commit 
                ++first;                                                      
            }                                                                 
        }
        
    public:

        ptr_multimap_adapter()
        { }

        template< class SizeType >
        ptr_multimap_adapter( SizeType n, 
                              ptr_container_detail::unordered_associative_container_tag tag )
          : base_type( n, tag )
        { }

        template< class Comp >
        explicit ptr_multimap_adapter( const Comp& comp,
                                       const allocator_type& a )
          : base_type( comp, a ) { }

        template< class Hash, class Pred, class Allocator >
        ptr_multimap_adapter( const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( hash, pred, a )
        { }

        template< class InputIterator >
        ptr_multimap_adapter( InputIterator first, InputIterator last )
        {
            map_basic_clone_and_insert( first, last );
        }
        
        template< class InputIterator, class Comp >
        ptr_multimap_adapter( InputIterator first, InputIterator last,
                              const Comp& comp,
                              const allocator_type& a )
          : base_type( comp, a )
        {
            map_basic_clone_and_insert( first, last );
        }

        template< class InputIterator, class Hash, class Pred, class Allocator >
        ptr_multimap_adapter( InputIterator first, InputIterator last,
                              const Hash& hash,
                              const Pred& pred,
                              const Allocator& a )
         : base_type( hash, pred, a )
        {
            map_basic_clone_and_insert( first, last ); 
        }

        ptr_multimap_adapter( const ptr_multimap_adapter& r )
        {
            map_basic_clone_and_insert( r.begin(), r.end() );      
        }
        
        template< class Key, class U, class CA, bool b >
        ptr_multimap_adapter( const ptr_multimap_adapter<Key,U,CA,b>& r )
        {
            map_basic_clone_and_insert( r.begin(), r.end() );      
        }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        explicit ptr_multimap_adapter( std::auto_ptr<U> r ) : base_type( r )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        explicit ptr_multimap_adapter( std::unique_ptr<U> r ) : base_type( std::move( r ) )
        { }
#endif

        ptr_multimap_adapter& operator=( ptr_multimap_adapter r )
        {
            this->swap( r );
            return *this;
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        ptr_multimap_adapter& operator=( std::auto_ptr<U> r )
        {  
            base_type::operator=( r );
            return *this;
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        ptr_multimap_adapter& operator=( std::unique_ptr<U> r )
        {  
            base_type::operator=( std::move( r ) );
            return *this;
        }
#endif

        using base_type::release;

    private:
        iterator insert_impl( const key_type& key, mapped_type x ) // strong
        {
            this->enforce_null_policy( x, 
                  "Null pointer in 'ptr_multimap_adapter::insert()'" );

            auto_type ptr( x, *this );  // nothrow
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator
                res = this->base().insert( std::make_pair( key, x ) );
                                        // strong, commit        
            ptr.release();              // notrow
            return iterator( res );           
        }

        iterator insert_impl( iterator before, const key_type& key, mapped_type x ) // strong
        {
            this->enforce_null_policy( x, 
                  "Null pointer in 'ptr_multimap_adapter::insert()'" );
            
            auto_type ptr( x, *this );  // nothrow
            BOOST_DEDUCED_TYPENAME base_type::ptr_iterator
                res = this->base().insert( before.base(), 
                                           std::make_pair( key, x ) );
                                        // strong, commit        
            ptr.release();              // notrow
            return iterator( res );                       
        }

    public:
        template< typename InputIterator >
        void insert( InputIterator first, InputIterator last ) // basic
        {
            map_basic_clone_and_insert( first, last );
        }

        template< class Range >
        void insert( const Range& r )
        {
            insert( boost::begin(r), boost::end(r) );
        }

        iterator insert( key_type& key, mapped_type x ) // strong
        {
            return insert_impl( key, x );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( const key_type& key, std::auto_ptr<U> x )
        {
            return insert_impl( key, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( const key_type& key, std::unique_ptr<U> x )
        {
            return insert_impl( key, x.release() );
        }
#endif

        template< class F, class S >
        iterator insert( iterator before, ptr_container_detail::ref_pair<F,S> p ) // strong
        {
            this->enforce_null_policy( p.second, 
                  "Null pointer in 'ptr_multimap_adapter::insert()'" );
            iterator res = insert_impl( before, p.first, 
                                        this->null_policy_allocate_clone( p.second ) );
            return res;
        }

        iterator insert( iterator before, key_type& key, mapped_type x ) // strong
        {
            return insert_impl( before, key, x );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( iterator before, const key_type& key, std::auto_ptr<U> x ) // strong
        {
            return insert_impl( before, key, x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( iterator before, const key_type& key, std::unique_ptr<U> x ) // strong
        {
            return insert_impl( before, key, x.release() );
        }
#endif
        
        template< class PtrMapAdapter >
        void transfer( BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator object, 
                       PtrMapAdapter& from ) // strong
        {
            this->multi_transfer( object, from );
        }

        template< class PtrMapAdapter >
        size_type transfer( BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator first, 
                            BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator last, 
                            PtrMapAdapter& from ) // basic
        {
            return this->multi_transfer( first, last, from );
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else    

        template<  class PtrMapAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                            BOOST_DEDUCED_TYPENAME PtrMapAdapter::iterator >,
                                                            size_type >::type
        transfer( const Range& r, PtrMapAdapter& from ) // basic
        {
            return transfer( boost::begin(r), boost::end(r), from );
        }

#endif        
        template< class PtrMapAdapter >
        void transfer( PtrMapAdapter& from ) // basic
        {
            transfer( from.begin(), from.end(), from );
            BOOST_ASSERT( from.empty() );
        }

    };

    template< class I, class F, class S >
    inline bool is_null( const ptr_map_iterator<I,F,S>& i )
    {
        return i->second == 0;
    }
    
} // namespace 'boost'  

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic pop
#endif

#endif
