//
// Boost.Pointer Container
//
//  Copyright Thorsten Ottosen 2008. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/ptr_container/
//

#ifndef BOOST_PTR_CONTAINER_PTR_CIRCULAR_BUFFER_HPP
#define BOOST_PTR_CONTAINER_PTR_CIRCULAR_BUFFER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif

#include <boost/circular_buffer.hpp>
#include <boost/ptr_container/ptr_sequence_adapter.hpp>
#include <boost/next_prior.hpp>
#include <boost/ptr_container/detail/ptr_container_disable_deprecated.hpp>

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace boost
{

    template
    < 
        class T, 
        class CloneAllocator = heap_clone_allocator,
        class Allocator      = std::allocator<void*>
    >
    class ptr_circular_buffer : public 
        ptr_sequence_adapter< T, boost::circular_buffer<
                typename ptr_container_detail::void_ptr<T>::type,Allocator>, 
                              CloneAllocator >
    {  
        typedef ptr_sequence_adapter< T, boost::circular_buffer<
                typename ptr_container_detail::void_ptr<T>::type,Allocator>, 
                                      CloneAllocator > 
            base_type;

        typedef boost::circular_buffer<typename 
            ptr_container_detail::void_ptr<T>::type,Allocator>  circular_buffer_type;
        typedef ptr_circular_buffer<T,CloneAllocator,Allocator> this_type;
        
    public: // typedefs
        typedef typename base_type::value_type     value_type;
        typedef value_type*                        pointer;
        typedef const value_type*                  const_pointer;
        typedef typename base_type::size_type      size_type;
        typedef typename base_type::allocator_type allocator_type;
        typedef typename base_type::iterator       iterator;
        typedef typename base_type::const_iterator const_iterator;
        typedef typename base_type::auto_type      auto_type;
        
        typedef std::pair<pointer,size_type>                  array_range;
        typedef std::pair<const_pointer,size_type>            const_array_range;
        typedef typename circular_buffer_type::capacity_type  capacity_type;

    public: // constructors
        ptr_circular_buffer()
        { }

        explicit ptr_circular_buffer( capacity_type n )
          : base_type( n, ptr_container_detail::fixed_length_sequence_tag() )
        { }
        
        ptr_circular_buffer( capacity_type n,
                             const allocator_type& alloc )
          : base_type( n, alloc, ptr_container_detail::fixed_length_sequence_tag() )
        { }

        template< class ForwardIterator >
        ptr_circular_buffer( ForwardIterator first, ForwardIterator last )
          : base_type( first, last, ptr_container_detail::fixed_length_sequence_tag() )
        { }

        template< class InputIterator >
        ptr_circular_buffer( capacity_type n, InputIterator first, InputIterator last )
          : base_type( n, first, last, ptr_container_detail::fixed_length_sequence_tag() )
        { }

        ptr_circular_buffer( const ptr_circular_buffer& r )
          : base_type( r.size(), r.begin(), r.end(), 
                       ptr_container_detail::fixed_length_sequence_tag() )
        { }

        template< class U >
        ptr_circular_buffer( const ptr_circular_buffer<U>& r )
          : base_type( r.size(), r.begin(), r.end(), 
                       ptr_container_detail::fixed_length_sequence_tag() )
        { }

        ptr_circular_buffer& operator=( ptr_circular_buffer r )
        {
            this->swap( r );
            return *this;
        }

        BOOST_PTR_CONTAINER_DEFINE_RELEASE_AND_CLONE( ptr_circular_buffer,
                                                      base_type, this_type )
            
    public: // allocators
        allocator_type& get_allocator() 
        {
            return this->base().get_allocator();
        }

        allocator_type get_allocator() const
        {
            return this->base().get_allocator();
        }

    public: // circular buffer functions
        array_range array_one() // nothrow
        {
            typename circular_buffer_type::array_range r = this->base().array_one();
            return array_range( reinterpret_cast<pointer>(r.first), r.second );
        }

        const_array_range array_one() const // nothrow
        {
            typename circular_buffer_type::const_array_range r = this->base().array_one();
            return const_array_range( reinterpret_cast<const_pointer>(r.first), r.second );
        }

        array_range array_two() // nothrow
        {
            typename circular_buffer_type::array_range r = this->base().array_two();
            return array_range( reinterpret_cast<pointer>(r.first), r.second );
        }

        const_array_range array_two() const // nothrow
        {
            typename circular_buffer_type::const_array_range r = this->base().array_two();
            return const_array_range( reinterpret_cast<const_pointer>(r.first), r.second );
        }

        pointer linearize() // nothrow
        {
            return reinterpret_cast<pointer>(this->base().linearize());
        }

        bool full() const // nothrow
        {
            return this->base().full();
        }

        size_type reserve() const // nothrow
        {
            return this->base().reserve();
        }

        void reserve( size_type n ) // strong
        {
            if( capacity() < n )
                set_capacity( n );
        }

        capacity_type capacity() const // nothrow
        {
            return this->base().capacity();
        }

        void set_capacity( capacity_type new_capacity ) // strong
        {
            if( this->size() > new_capacity )
            {
                this->erase( this->begin() + new_capacity, this->end() );
            }
            this->base().set_capacity( new_capacity );
        }

        void rset_capacity( capacity_type new_capacity ) // strong
        {
            if( this->size() > new_capacity )
            {
                this->erase( this->begin(), 
                             this->begin() + (this->size()-new_capacity) );
            }
            this->base().rset_capacity( new_capacity );
        }

        void resize( size_type size ) // basic
        {
            size_type old_size = this->size();
            if( old_size > size )
            {
                this->erase( boost::next( this->begin(), size ), this->end() );  
            }
            else if( size > old_size )
            {
                for( ; old_size != size; ++old_size )
                    this->push_back( new BOOST_DEDUCED_TYPENAME 
                                     boost::remove_pointer<value_type>::type() ); 
            }

            BOOST_ASSERT( this->size() == size );
        }

        void resize( size_type size, value_type to_clone ) // basic
        {
            size_type old_size = this->size();
            if( old_size > size )
            {
                this->erase( boost::next( this->begin(), size ), this->end() );  
            }
            else if( size > old_size )
            {
                for( ; old_size != size; ++old_size )
                    this->push_back( this->null_policy_allocate_clone( to_clone ) ); 
            }

            BOOST_ASSERT( this->size() == size );        
        }

        void rresize( size_type size ) // basic
        {
            size_type old_size = this->size();
            if( old_size > size )
            {
                this->erase( this->begin(), 
                             boost::next( this->begin(), old_size - size ) );  
            }
            else if( size > old_size )
            {
                for( ; old_size != size; ++old_size )
                    this->push_front( new BOOST_DEDUCED_TYPENAME 
                                      boost::remove_pointer<value_type>::type() ); 
            }

            BOOST_ASSERT( this->size() == size );
        }

        void rresize( size_type size, value_type to_clone ) // basic
        {
            size_type old_size = this->size();
            if( old_size > size )
            {
                this->erase( this->begin(), 
                             boost::next( this->begin(), old_size - size ) );  
            }
            else if( size > old_size )
            {
                for( ; old_size != size; ++old_size )
                    this->push_front( this->null_policy_allocate_clone( to_clone ) ); 
            }

            BOOST_ASSERT( this->size() == size );
        }           

        template< class InputIterator >
        void assign( InputIterator first, InputIterator last ) // strong
        { 
            ptr_circular_buffer temp( first, last );
            this->swap( temp );
        }

        template< class Range >
        void assign( const Range& r ) // strong
        {
            assign( boost::begin(r), boost::end(r ) );
        }

        void assign( size_type n, value_type to_clone ) // strong
        {
            ptr_circular_buffer temp( n );
            for( size_type i = 0u; i != n; ++i )
               temp.push_back( temp.null_policy_allocate_clone( to_clone ) );
            this->swap( temp ); 
        }
        
        void assign( capacity_type capacity, size_type n, 
                     value_type to_clone ) // basic
        {
            this->assign( (std::min)(n,capacity), to_clone );
        }

        template< class InputIterator >
        void assign( capacity_type capacity, 
                     InputIterator first, InputIterator last ) // basic
        {
            this->assign( first, last );
            this->set_capacity( capacity );
        }

        void push_back( value_type ptr ) // nothrow
        {
            BOOST_ASSERT( capacity() > 0 );
            this->enforce_null_policy( ptr, "Null pointer in 'push_back()'" );
         
            auto_type old_ptr( value_type(), *this );
            if( full() )
                old_ptr.reset( &*this->begin(), *this );
            this->base().push_back( ptr );           
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        void push_back( std::auto_ptr<U> ptr ) // nothrow
        {
            push_back( ptr.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        void push_back( std::unique_ptr<U> ptr ) // nothrow
        {
            push_back( ptr.release() );
        }
#endif

        void push_front( value_type ptr ) // nothrow
        {
            BOOST_ASSERT( capacity() > 0 );
            this->enforce_null_policy( ptr, "Null pointer in 'push_front()'" );

            auto_type old_ptr( value_type(), *this );
            if( full() )
                old_ptr.reset( &*(--this->end()), *this );
            this->base().push_front( ptr );            
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        void push_front( std::auto_ptr<U> ptr ) // nothrow
        {
            push_front( ptr.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        void push_front( std::unique_ptr<U> ptr ) // nothrow
        {
            push_front( ptr.release() );
        }
#endif

        iterator insert( iterator pos, value_type ptr ) // nothrow
        {
            BOOST_ASSERT( capacity() > 0 );
            this->enforce_null_policy( ptr, "Null pointer in 'insert()'" );

            auto_type new_ptr( ptr, *this );
            iterator b = this->begin();
            if( full() && pos == b )
                return b;
            
            new_ptr.release();            
            auto_type old_ptr( value_type(), *this );
            if( full() )
                old_ptr.reset( &*this->begin(), *this );

            return this->base().insert( pos.base(), ptr );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator insert( iterator pos, std::auto_ptr<U> ptr ) // nothrow
        {
            return insert( pos, ptr.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator insert( iterator pos, std::unique_ptr<U> ptr ) // nothrow
        {
            return insert( pos, ptr.release() );
        }
#endif

        template< class InputIterator >
        void insert( iterator pos, InputIterator first, InputIterator last ) // basic
        {
            for( ; first != last; ++first, ++pos )
                pos = insert( pos, this->null_policy_allocate_clone( &*first ) );                
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else
        template< class Range >
        BOOST_DEDUCED_TYPENAME
        boost::disable_if< ptr_container_detail::is_pointer_or_integral<Range> >::type
        insert( iterator before, const Range& r )
        {
            insert( before, boost::begin(r), boost::end(r) );
        }

#endif
           
        iterator rinsert( iterator pos, value_type ptr ) // nothrow
        {
            BOOST_ASSERT( capacity() > 0 );
            this->enforce_null_policy( ptr, "Null pointer in 'rinsert()'" );

            auto_type new_ptr( ptr, *this );
            iterator b = this->end();
            if (full() && pos == b)
                return b;
                       
            new_ptr.release();            
            auto_type old_ptr( value_type(), *this );
            if( full() )
                old_ptr.reset( &this->back(), *this );

            return this->base().rinsert( pos.base(), ptr );
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        iterator rinsert( iterator pos, std::auto_ptr<U> ptr ) // nothrow
        {
            return rinsert( pos, ptr.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        iterator rinsert( iterator pos, std::unique_ptr<U> ptr ) // nothrow
        {
            return rinsert( pos, ptr.release() );
        }
#endif

 
        template< class InputIterator >
        void rinsert( iterator pos, InputIterator first, InputIterator last ) // basic
        {
            for( ; first != last; ++first, ++pos )
                pos = rinsert( pos, this->null_policy_allocate_clone( &*first ) );                
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else
        template< class Range >
        BOOST_DEDUCED_TYPENAME
        boost::disable_if< ptr_container_detail::is_pointer_or_integral<Range> >::type
        rinsert( iterator before, const Range& r )
        {
            rinsert( before, boost::begin(r), boost::end(r) );
        }

#endif

        iterator rerase( iterator pos ) // nothrow
        {
            BOOST_ASSERT( !this->empty() );
            BOOST_ASSERT( pos != this->end() );

            this->remove( pos );
            return iterator( this->base().rerase( pos.base() ) );
        }

        iterator rerase( iterator first, iterator last ) // nothrow
        {
            this->remove( first, last );
            return iterator( this->base().rerase( first.base(),
                                                  last.base() ) );
        }

        template< class Range >
        iterator rerase( const Range& r ) // nothrow
        {
            return rerase( boost::begin(r), boost::end(r) );
        }

        void rotate( const_iterator new_begin ) // nothrow
        {
            this->base().rotate( new_begin.base() );
        }

    public: // transfer        
        template< class PtrSeqAdapter >
        void transfer( iterator before, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator first, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator last, 
                       PtrSeqAdapter& from ) // nothrow
        {
            BOOST_ASSERT( (void*)&from != (void*)this );
            if( from.empty() )
                return;
            for( BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator begin = first; 
                 begin != last;  ++begin, ++before )
                before = insert( before, &*begin );          // nothrow
            from.base().erase( first.base(), last.base() );  // nothrow
        }

        template< class PtrSeqAdapter >
        void transfer( iterator before, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator object, 
                       PtrSeqAdapter& from ) // nothrow
        {
            BOOST_ASSERT( (void*)&from != (void*)this );
            if( from.empty() )
                return;
            insert( before, &*object );          // nothrow 
            from.base().erase( object.base() );  // nothrow 
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else
        
        template< class PtrSeqAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                      BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator > >::type
        transfer( iterator before, const Range& r, PtrSeqAdapter& from ) // nothrow
        {
            transfer( before, boost::begin(r), boost::end(r), from );
        }

#endif
        template< class PtrSeqAdapter >
        void transfer( iterator before, PtrSeqAdapter& from ) // nothrow
        {
            transfer( before, from.begin(), from.end(), from );            
        }

    public: // C-array support
    
        void transfer( iterator before, value_type* from, 
                       size_type size, bool delete_from = true ) // nothrow 
        {
            BOOST_ASSERT( from != 0 );
            if( delete_from )
            {
                BOOST_DEDUCED_TYPENAME base_type::scoped_deleter 
                    deleter( *this, from, size );                  // nothrow
                for( size_type i = 0u; i != size; ++i, ++before )
                    before = insert( before, *(from+i) );          // nothrow
                deleter.release();                                 // nothrow
            }
            else
            {
                for( size_type i = 0u; i != size; ++i, ++before )
                    before = insert( before, *(from+i) );          // nothrow
           }
        }

        value_type* c_array() // nothrow
        {
            if( this->empty() )
                return 0;
            this->linearize();
            T** res = reinterpret_cast<T**>( &this->begin().base()[0] );
            return res;
        }

    };

    //////////////////////////////////////////////////////////////////////////////
    // clonability

    template< typename T, typename CA, typename A >
    inline ptr_circular_buffer<T,CA,A>* new_clone( const ptr_circular_buffer<T,CA,A>& r )
    {
        return r.clone().release();
    }

    /////////////////////////////////////////////////////////////////////////
    // swap

    template< typename T, typename CA, typename A >
    inline void swap( ptr_circular_buffer<T,CA,A>& l, ptr_circular_buffer<T,CA,A>& r )
    {
        l.swap(r);
    }
    
}

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic pop
#endif

#endif
