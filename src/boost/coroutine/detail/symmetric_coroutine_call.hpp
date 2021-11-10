
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_CALL_H
#define BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_CALL_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/preallocated.hpp>
#include <boost/coroutine/detail/symmetric_coroutine_impl.hpp>
#include <boost/coroutine/detail/symmetric_coroutine_object.hpp>
#include <boost/coroutine/detail/symmetric_coroutine_yield.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename Arg >
class symmetric_coroutine_call
{
private:
    template< typename X >
    friend class symmetric_coroutine_yield;

    typedef symmetric_coroutine_impl< Arg >   impl_type;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_call)

    struct dummy {};

    impl_type       *   impl_;

public:
    typedef Arg                                value_type;
    typedef symmetric_coroutine_yield< Arg >   yield_type;

    symmetric_coroutine_call() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( yield_type &);

    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, coroutine_fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename StackAllocator >
    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, coroutine_fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
# endif
    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#else
    template< typename Fn >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#endif

    ~symmetric_coroutine_call()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    symmetric_coroutine_call( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    symmetric_coroutine_call & operator=( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_call tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete() || impl_->is_running(); }

    void swap( symmetric_coroutine_call & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    symmetric_coroutine_call & operator()( Arg arg) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( * this);

        impl_->resume( arg);
        return * this;
    }
};

template< typename Arg >
class symmetric_coroutine_call< Arg & >
{
private:
    template< typename X >
    friend class symmetric_coroutine_yield;

    typedef symmetric_coroutine_impl< Arg & >     impl_type;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_call)

    struct dummy {};

    impl_type       *   impl_;

public:
    typedef Arg                                    value_type;
    typedef symmetric_coroutine_yield< Arg & >     yield_type;

    symmetric_coroutine_call() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( yield_type &);

    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, coroutine_fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename StackAllocator >
    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, coroutine_fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
# endif
    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#else
    template< typename Fn >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< Arg &, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#endif

    ~symmetric_coroutine_call()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    symmetric_coroutine_call( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    symmetric_coroutine_call & operator=( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_call tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete() || impl_->is_running(); }

    void swap( symmetric_coroutine_call & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    symmetric_coroutine_call & operator()( Arg & arg) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( * this);

        impl_->resume( arg);
        return * this;
    }
};

template<>
class symmetric_coroutine_call< void >
{
private:
    template< typename X >
    friend class symmetric_coroutine_yield;

    typedef symmetric_coroutine_impl< void >        impl_type;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( symmetric_coroutine_call)

    struct dummy {};

    impl_type       *   impl_;

public:
    typedef void                                     value_type;
    typedef symmetric_coroutine_yield< void >        yield_type;

    symmetric_coroutine_call() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( yield_type &);

    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, coroutine_fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename StackAllocator >
    explicit symmetric_coroutine_call( coroutine_fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, coroutine_fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< coroutine_fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
# endif
    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    boost::forward< Fn >( fn), attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#else
    template< typename Fn >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( Fn fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs = attributes(),
                                       stack_allocator stack_alloc = stack_allocator() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, stack_allocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }

    template< typename Fn, typename StackAllocator >
    explicit symmetric_coroutine_call( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs,
                                       StackAllocator stack_alloc) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef symmetric_coroutine_object< void, Fn, StackAllocator > object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                    fn, attrs, preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
    }
#endif

    ~symmetric_coroutine_call()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    inline symmetric_coroutine_call( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    inline symmetric_coroutine_call & operator=( BOOST_RV_REF( symmetric_coroutine_call) other) BOOST_NOEXCEPT
    {
        symmetric_coroutine_call tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete() || impl_->is_running(); }

    inline void swap( symmetric_coroutine_call & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    inline symmetric_coroutine_call & operator()() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( * this);

        impl_->resume();
        return * this;
    }
};

template< typename Arg >
void swap( symmetric_coroutine_call< Arg > & l,
           symmetric_coroutine_call< Arg > & r)
{ l.swap( r); }

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_CALL_H
