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

#ifndef BOOST_PTR_CONTAINER_PTR_SEQUENCE_ADAPTER_HPP
#define BOOST_PTR_CONTAINER_PTR_SEQUENCE_ADAPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif


#include <boost/ptr_container/detail/reversible_ptr_container.hpp>
#include <boost/ptr_container/indirect_fun.hpp>
#include <boost/ptr_container/detail/void_ptr_iterator.hpp>
#include <boost/ptr_container/detail/ptr_container_disable_deprecated.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/next_prior.hpp>

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
        class VoidPtrSeq
    >
    struct sequence_config
    {
        typedef BOOST_DEDUCED_TYPENAME remove_nullable<T>::type
                    U;
        typedef VoidPtrSeq
                    void_container_type;

        typedef BOOST_DEDUCED_TYPENAME VoidPtrSeq::allocator_type
                    allocator_type;
        
        typedef U   value_type;

        typedef void_ptr_iterator<
                        BOOST_DEDUCED_TYPENAME VoidPtrSeq::iterator, U > 
                    iterator;
       
        typedef void_ptr_iterator<
                        BOOST_DEDUCED_TYPENAME VoidPtrSeq::const_iterator, const U >
                    const_iterator;

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)

        template< class Iter >
        static U* get_pointer( Iter i )
        {
            return static_cast<U*>( *i.base() );
        }
        
#else
        template< class Iter >
        static U* get_pointer( void_ptr_iterator<Iter,U> i )
        {
            return static_cast<U*>( *i.base() );
        }

        template< class Iter >
        static U* get_pointer( Iter i )
        {
            return &*i;
        }
#endif        

#if defined(BOOST_NO_SFINAE) && !BOOST_WORKAROUND(__MWERKS__, <= 0x3003)

        template< class Iter >
        static const U* get_const_pointer( Iter i )
        {
            return static_cast<const U*>( *i.base() );
        }
        
#else // BOOST_NO_SFINAE

#if BOOST_WORKAROUND(__MWERKS__, <= 0x3003)
        template< class Iter >
        static const U* get_const_pointer( void_ptr_iterator<Iter,U> i )
        {
            return static_cast<const U*>( *i.base() );
        }
#else // BOOST_WORKAROUND
        template< class Iter >
        static const U* get_const_pointer( void_ptr_iterator<Iter,const U> i )
        {
            return static_cast<const U*>( *i.base() );
        }
#endif // BOOST_WORKAROUND

        template< class Iter >
        static const U* get_const_pointer( Iter i )
        {
            return &*i;
        }
#endif // BOOST_NO_SFINAE

        BOOST_STATIC_CONSTANT(bool, allow_null = boost::is_nullable<T>::value );
    };
    
} // ptr_container_detail


    template< class Iterator, class T >
    inline bool is_null( void_ptr_iterator<Iterator,T> i )
    {
        return *i.base() == 0;
    }


    
    template
    < 
        class T,
        class VoidPtrSeq, 
        class CloneAllocator = heap_clone_allocator
    >
    class ptr_sequence_adapter : public 
        ptr_container_detail::reversible_ptr_container< ptr_container_detail::sequence_config<T,VoidPtrSeq>, 
                                            CloneAllocator >
    {
        typedef ptr_container_detail::reversible_ptr_container< ptr_container_detail::sequence_config<T,VoidPtrSeq>,
                                                    CloneAllocator >
             base_type;
        
        typedef ptr_sequence_adapter<T,VoidPtrSeq,CloneAllocator>                         
            this_type;

    protected:
        typedef BOOST_DEDUCED_TYPENAME base_type::scoped_deleter scoped_deleter;
         
    public:
        typedef BOOST_DEDUCED_TYPENAME base_type::value_type  value_type; 
        typedef BOOST_DEDUCED_TYPENAME base_type::reference   reference; 
        typedef BOOST_DEDUCED_TYPENAME base_type::const_reference 
                                                              const_reference;
        typedef BOOST_DEDUCED_TYPENAME base_type::auto_type   auto_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::clone_allocator_type
                                                              clone_allocator_type;
        typedef BOOST_DEDUCED_TYPENAME base_type::iterator    iterator;          
        typedef BOOST_DEDUCED_TYPENAME base_type::size_type   size_type;  
        typedef BOOST_DEDUCED_TYPENAME base_type::allocator_type  
                                                              allocator_type;
                
        ptr_sequence_adapter()
        { }

        template< class Allocator >
        explicit ptr_sequence_adapter( const Allocator& a )
          : base_type( a )
        { }

        template< class SizeType >
        ptr_sequence_adapter( SizeType n, 
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( n, tag )
        { }

        template< class SizeType, class Allocator >
        ptr_sequence_adapter( SizeType n, const Allocator& a, 
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( n, a, tag )
        { }

        template< class InputIterator >
        ptr_sequence_adapter( InputIterator first, InputIterator last )
          : base_type( first, last )
        { }

        template< class InputIterator, class Allocator >
        ptr_sequence_adapter( InputIterator first, InputIterator last,
                              const Allocator& a )
          : base_type( first, last, a )
        { }

        template< class ForwardIterator >
        ptr_sequence_adapter( ForwardIterator first,
                              ForwardIterator last,
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( first, last,  tag )
        { }

        template< class SizeType, class ForwardIterator >
        ptr_sequence_adapter( SizeType n,
                              ForwardIterator first,
                              ForwardIterator last,
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( n, first, last,  tag )
        { }

        ptr_sequence_adapter( const ptr_sequence_adapter& r )
          : base_type( r )
        { }
        
        template< class U >
        ptr_sequence_adapter( const ptr_sequence_adapter<U,VoidPtrSeq,CloneAllocator>& r )
          : base_type( r )
        { }
        
        ptr_sequence_adapter( const ptr_sequence_adapter& r,
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( r, tag )
        { }
        
        template< class U >
        ptr_sequence_adapter( const ptr_sequence_adapter<U,VoidPtrSeq,CloneAllocator>& r,
                              ptr_container_detail::fixed_length_sequence_tag tag )
          : base_type( r, tag )
        { }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        explicit ptr_sequence_adapter( std::auto_ptr<PtrContainer> clone )
          : base_type( clone )
        { }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        explicit ptr_sequence_adapter( std::unique_ptr<PtrContainer> clone )
          : base_type( std::move( clone ) )
        { }
#endif

        ptr_sequence_adapter& operator=( const ptr_sequence_adapter r )
        {
            this->swap( r );
            return *this; 
        }
        
#ifndef BOOST_NO_AUTO_PTR
        template< class PtrContainer >
        ptr_sequence_adapter& operator=( std::auto_ptr<PtrContainer> clone )
        {
            base_type::operator=( clone );
            return *this;
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class PtrContainer >
        ptr_sequence_adapter& operator=( std::unique_ptr<PtrContainer> clone )
        {
            base_type::operator=( std::move( clone ) );
            return *this;
        }
#endif

        /////////////////////////////////////////////////////////////
        // modifiers
        /////////////////////////////////////////////////////////////

        void push_back( value_type x )  // strong               
        {
            this->enforce_null_policy( x, "Null pointer in 'push_back()'" );
            auto_type ptr( x, *this );    // notrow
            this->base().push_back( x );  // strong, commit
            ptr.release();                // nothrow
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        void push_back( std::auto_ptr<U> x )
        {
            push_back( x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        void push_back( std::unique_ptr<U> x )
        {
            push_back( x.release() );
        }
#endif
        
        void push_front( value_type x )                
        {
            this->enforce_null_policy( x, "Null pointer in 'push_front()'" );
            auto_type ptr( x, *this );    // nothrow            
            this->base().push_front( x ); // strong, commit
            ptr.release();                // nothrow
        }

#ifndef BOOST_NO_AUTO_PTR
        template< class U >
        void push_front( std::auto_ptr<U> x )
        {
            push_front( x.release() );
        }
#endif
#ifndef BOOST_NO_CXX11_SMART_PTR
        template< class U >
        void push_front( std::unique_ptr<U> x )
        {
            push_front( x.release() );
        }
#endif

        auto_type pop_back()
        {
            BOOST_ASSERT( !this->empty() && 
                          "'pop_back()' on empty container" );
            auto_type ptr( static_cast<value_type>(this->base().back()), *this );      
                                                       // nothrow
            this->base().pop_back();                   // nothrow
            return ptr_container_detail::move( ptr );  // nothrow
        }

        auto_type pop_front()
        {
            BOOST_ASSERT( !this->empty() &&
                          "'pop_front()' on empty container" ); 
            auto_type ptr( static_cast<value_type>(this->base().front()), *this ); 
                                         // nothrow 
            this->base().pop_front();    // nothrow
            return ptr_container_detail::move( ptr ); 
        }
        
        reference front()        
        { 
            BOOST_ASSERT( !this->empty() &&
                          "accessing 'front()' on empty container" );

            BOOST_ASSERT( !::boost::is_null( this->begin() ) );
            return *this->begin(); 
        }

        const_reference front() const  
        {
            return const_cast<ptr_sequence_adapter*>(this)->front();
        }

        reference back()
        {
            BOOST_ASSERT( !this->empty() &&
                          "accessing 'back()' on empty container" );
            BOOST_ASSERT( !::boost::is_null( --this->end() ) );
            return *--this->end(); 
        }

        const_reference back() const
        {
            return const_cast<ptr_sequence_adapter*>(this)->back();
        }

    public: // deque/vector inerface
        
        reference operator[]( size_type n ) // nothrow 
        {
            BOOST_ASSERT( n < this->size() );
            BOOST_ASSERT( !this->is_null( n ) );
            return *static_cast<value_type>( this->base()[n] ); 
        }
        
        const_reference operator[]( size_type n ) const // nothrow  
        { 
            BOOST_ASSERT( n < this->size() ); 
            BOOST_ASSERT( !this->is_null( n ) );
            return *static_cast<value_type>( this->base()[n] );
        }
        
        reference at( size_type n )
        {
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( n >= this->size(), bad_index, 
                                                 "'at()' out of bounds" );
            BOOST_ASSERT( !this->is_null( n ) );
            return (*this)[n];
        }
        
        const_reference at( size_type n ) const
        {
            BOOST_PTR_CONTAINER_THROW_EXCEPTION( n >= this->size(), bad_index, 
                                                 "'at()' out of bounds" );
            BOOST_ASSERT( !this->is_null( n ) );
            return (*this)[n]; 
        }
        
    public: // vector interface
        
        size_type capacity() const
        {
            return this->base().capacity();
        }
        
        void reserve( size_type n )
        {
            this->base().reserve( n ); 
        }

        void reverse()
        {
            this->base().reverse(); 
        }

    public: // assign, insert, transfer

        // overhead: 1 heap allocation (very cheap compared to cloning)
        template< class InputIterator >
        void assign( InputIterator first, InputIterator last ) // strong
        { 
            base_type temp( first, last );
            this->swap( temp );
        }

        template< class Range >
        void assign( const Range& r ) // strong
        {
            assign( boost::begin(r), boost::end(r ) );
        }

    private:
        template< class I >
        void insert_impl( iterator before, I first, I last, std::input_iterator_tag ) // strong
        {
            ptr_sequence_adapter temp(first,last);  // strong
            transfer( before, temp );               // strong, commit
        }

        template< class I >
        void insert_impl( iterator before, I first, I last, std::forward_iterator_tag ) // strong
        {
            if( first == last ) 
                return;
            scoped_deleter sd( *this, first, last );         // strong
            this->insert_clones_and_release( sd, before );   // strong, commit 
        }

    public:

        using base_type::insert;
        
        template< class InputIterator >
        void insert( iterator before, InputIterator first, InputIterator last ) // strong
        {
            insert_impl( before, first, last, BOOST_DEDUCED_TYPENAME
                         iterator_category<InputIterator>::type() );
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
        
        template< class PtrSeqAdapter >
        void transfer( iterator before, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator first, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator last, 
                       PtrSeqAdapter& from ) // strong
        {
            BOOST_ASSERT( (void*)&from != (void*)this );
            if( from.empty() )
                return;
            this->base().
                insert( before.base(), first.base(), last.base() ); // strong
            from.base().erase( first.base(), last.base() );         // nothrow
        }

        template< class PtrSeqAdapter >
        void transfer( iterator before, 
                       BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator object, 
                       PtrSeqAdapter& from ) // strong
        {
            BOOST_ASSERT( (void*)&from != (void*)this );
            if( from.empty() )
                return;
            this->base().insert( before.base(), *object.base() ); // strong 
            from.base().erase( object.base() );                  // nothrow 
        }

#if defined(BOOST_NO_SFINAE) || defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
#else
        
        template< class PtrSeqAdapter, class Range >
        BOOST_DEDUCED_TYPENAME boost::disable_if< boost::is_same< Range,
                      BOOST_DEDUCED_TYPENAME PtrSeqAdapter::iterator > >::type
        transfer( iterator before, const Range& r, PtrSeqAdapter& from ) // strong
        {
            transfer( before, boost::begin(r), boost::end(r), from );
        }

#endif
        template< class PtrSeqAdapter >
        void transfer( iterator before, PtrSeqAdapter& from ) // strong
        {
            BOOST_ASSERT( (void*)&from != (void*)this );
            if( from.empty() )
                return;
            this->base().
                insert( before.base(),
                        from.begin().base(), from.end().base() ); // strong
            from.base().clear();                                  // nothrow
        }

    public: // C-array support
    
        void transfer( iterator before, value_type* from, 
                       size_type size, bool delete_from = true ) // strong 
        {
            BOOST_ASSERT( from != 0 );
            if( delete_from )
            {
                BOOST_DEDUCED_TYPENAME base_type::scoped_deleter 
                    deleter( *this, from, size );                         // nothrow
                this->base().insert( before.base(), from, from + size );  // strong
                deleter.release();                                        // nothrow
            }
            else
            {
                this->base().insert( before.base(), from, from + size ); // strong
            }
        }

        value_type* c_array() // nothrow
        {
            if( this->empty() )
                return 0;
            T** res = reinterpret_cast<T**>( &this->begin().base()[0] );
            return res;
        }

    public: // null functions
         
        bool is_null( size_type idx ) const
        {
            BOOST_ASSERT( idx < this->size() );
            return this->base()[idx] == 0;
        }

    public: // resize

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
                
    public: // algorithms

        void sort( iterator first, iterator last )
        {
            sort( first, last, std::less<T>() );
        }
        
        void sort()
        {
            sort( this->begin(), this->end() );
        }

        template< class Compare >
        void sort( iterator first, iterator last, Compare comp )
        {
            BOOST_ASSERT( first <= last && "out of range sort()" );
            BOOST_ASSERT( this->begin() <= first && "out of range sort()" );
            BOOST_ASSERT( last <= this->end() && "out of range sort()" ); 
            // some static assert on the arguments of the comparison
            std::sort( first.base(), last.base(), 
                       void_ptr_indirect_fun<Compare,T>(comp) );
        }
        
        template< class Compare >
        void sort( Compare comp )
        {
            sort( this->begin(), this->end(), comp );
        }
        
        void unique( iterator first, iterator last )
        {
            unique( first, last, std::equal_to<T>() );
        }
        
        void unique()
        {
            unique( this->begin(), this->end() );
        }

    private:
        struct is_not_zero_ptr
        {
            template< class U >
            bool operator()( const U* r ) const
            {
                return r != 0;
            }
        };

    protected:
        template< class Fun, class Arg1 >
        class void_ptr_delete_if 
        {
            Fun fun;
        public:
        
            void_ptr_delete_if() : fun(Fun())
            { }
        
            void_ptr_delete_if( Fun f ) : fun(f)
            { }
        
            bool operator()( void* r ) const
            {
               BOOST_ASSERT( r != 0 );
               Arg1 arg1 = static_cast<Arg1>(r);
               if( fun( *arg1 ) )
               { 
                   clone_allocator_type::deallocate_clone( arg1 );
                   return true;
               }
               return false;
            }
        };

    private:
        void compact_and_erase_nulls( iterator first, iterator last ) // nothrow
        {
            typename base_type::ptr_iterator p = std::stable_partition( 
                                                    first.base(), 
                                                    last.base(), 
                                                    is_not_zero_ptr() );
            this->base().erase( p, this->end().base() );
            
        }

        void range_check_impl( iterator, iterator, 
                               std::bidirectional_iterator_tag )
        { /* do nothing */ }

        void range_check_impl( iterator first, iterator last,
                               std::random_access_iterator_tag )
        {
            BOOST_ASSERT( first <= last && "out of range unique()/erase_if()" );
            BOOST_ASSERT( this->begin() <= first && "out of range unique()/erase_if()" );
            BOOST_ASSERT( last <= this->end() && "out of range unique()/erase_if)(" );             
        }
        
        void range_check( iterator first, iterator last )
        {
            range_check_impl( first, last, 
                              BOOST_DEDUCED_TYPENAME iterator_category<iterator>::type() );
        }
        
    public:
        
        template< class Compare >
        void unique( iterator first, iterator last, Compare comp )
        {
            range_check(first,last);
            
            iterator prev = first;
            iterator next = first;
            ++next;
            for( ; next != last; ++next )
            {
                BOOST_ASSERT( !::boost::is_null(prev) );
                BOOST_ASSERT( !::boost::is_null(next) );
                if( comp( *prev, *next ) )
                {
                    this->remove( next ); // delete object
                    *next.base() = 0;     // mark pointer as deleted
                }
                else
                {
                    prev = next;
                }
                // ++next
            }

            compact_and_erase_nulls( first, last );
        }
        
        template< class Compare >
        void unique( Compare comp )
        {
            unique( this->begin(), this->end(), comp );
        }

        template< class Pred >
        void erase_if( iterator first, iterator last, Pred pred )
        {
            range_check(first,last);
            this->base().erase( std::remove_if( first.base(), last.base(), 
                                                void_ptr_delete_if<Pred,value_type>(pred) ),
                                last.base() );  
        }
        
        template< class Pred >
        void erase_if( Pred pred )
        {
            erase_if( this->begin(), this->end(), pred );
        }


        void merge( iterator first, iterator last, 
                    ptr_sequence_adapter& from )
        {
             merge( first, last, from, std::less<T>() );
        }
        
        template< class BinPred >
        void merge( iterator first, iterator last, 
                    ptr_sequence_adapter& from, BinPred pred )
        {
            void_ptr_indirect_fun<BinPred,T>  bin_pred(pred);
            size_type                         current_size = this->size(); 
            this->transfer( this->end(), first, last, from );
            typename base_type::ptr_iterator middle = this->begin().base();
            std::advance(middle,current_size); 
            std::inplace_merge( this->begin().base(),
                                middle,
                                this->end().base(),
                                bin_pred );
        }
        
        void merge( ptr_sequence_adapter& r )
        {
            merge( r, std::less<T>() );
            BOOST_ASSERT( r.empty() );
        }
        
        template< class BinPred >
        void merge( ptr_sequence_adapter& r, BinPred pred )
        {
            merge( r.begin(), r.end(), r, pred );
            BOOST_ASSERT( r.empty() );    
        }
        
    };


} // namespace 'boost'  

#if defined(BOOST_PTR_CONTAINER_DISABLE_DEPRECATED)
#pragma GCC diagnostic pop
#endif

#endif
