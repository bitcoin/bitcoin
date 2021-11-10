
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_ASYMMETRIC_COROUTINE_H
#define BOOST_COROUTINES_ASYMMETRIC_COROUTINE_H

#include <cstddef>
#include <iterator>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#include <boost/coroutine/attributes.hpp>
#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/detail/parameters.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <boost/coroutine/detail/pull_coroutine_impl.hpp>
#include <boost/coroutine/detail/pull_coroutine_object.hpp>
#include <boost/coroutine/detail/pull_coroutine_synthesized.hpp>
#include <boost/coroutine/detail/push_coroutine_impl.hpp>
#include <boost/coroutine/detail/push_coroutine_object.hpp>
#include <boost/coroutine/detail/push_coroutine_synthesized.hpp>
#include <boost/coroutine/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

template< typename R >
class pull_coroutine;

template< typename Arg >
class push_coroutine
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_impl< Arg >          impl_type;
    typedef detail::push_coroutine_synthesized< Arg >   synth_type;
    typedef detail::parameters< Arg >                   param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    explicit push_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( pull_coroutine< Arg > &);

    explicit push_coroutine( coroutine_fn,
                             attributes const& = attributes() );

    template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn,
                             attributes const&,
                             StackAllocator);
# endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#else
    template< typename Fn >
    explicit push_coroutine( Fn fn,
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn fn,
                             attributes const&,
                             StackAllocator);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#endif

    ~push_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    push_coroutine & operator()( Arg arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( arg);
        return * this;
    }

    class iterator
    {
    private:
       push_coroutine< Arg >    *   c_;

    public:
        typedef std::output_iterator_tag iterator_category;
        typedef void value_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;

        iterator() :
           c_( 0)
        {}

        explicit iterator( push_coroutine< Arg > * c) :
            c_( c)
        {}

        iterator & operator=( Arg a)
        {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( a) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other) const
        { return other.c_ == c_; }

        bool operator!=( iterator const& other) const
        { return other.c_ != c_; }

        iterator & operator*()
        { return * this; }

        iterator & operator++()
        { return * this; }
    };

    struct const_iterator;
};

template< typename Arg >
class push_coroutine< Arg & >
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_impl< Arg & >          impl_type;
    typedef detail::push_coroutine_synthesized< Arg & >   synth_type;
    typedef detail::parameters< Arg & >                   param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    explicit push_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( pull_coroutine< Arg & > &);

    explicit push_coroutine( coroutine_fn,
                             attributes const& = attributes() );

    template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn,
                             attributes const&,
                             StackAllocator);
# endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#else
    template< typename Fn >
    explicit push_coroutine( Fn,
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn,
                             attributes const&,
                             StackAllocator);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#endif

    ~push_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    void swap( push_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    push_coroutine & operator()( Arg & arg)
    {
        BOOST_ASSERT( * this);

        impl_->push( arg);
        return * this;
    }

    class iterator
    {
    private:
       push_coroutine< Arg & >  *   c_;

    public:
        typedef std::output_iterator_tag iterator_category;
        typedef void value_type;
        typedef void difference_type;
        typedef void pointer;
        typedef void reference;

        iterator() :
           c_( 0)
        {}

        explicit iterator( push_coroutine< Arg & > * c) :
            c_( c)
        {}

        iterator & operator=( Arg & a)
        {
            BOOST_ASSERT( c_);
            if ( ! ( * c_)( a) ) c_ = 0;
            return * this;
        }

        bool operator==( iterator const& other) const
        { return other.c_ == c_; }

        bool operator!=( iterator const& other) const
        { return other.c_ != c_; }

        iterator & operator*()
        { return * this; }

        iterator & operator++()
        { return * this; }
    };

    struct const_iterator;
};

template<>
class push_coroutine< void >
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::pull_coroutine_object;

    typedef detail::push_coroutine_impl< void >          impl_type;
    typedef detail::push_coroutine_synthesized< void >   synth_type;
    typedef detail::parameters< void >                   param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( push_coroutine)

    explicit push_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    push_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( pull_coroutine< void > &);

    explicit push_coroutine( coroutine_fn,
                             attributes const& = attributes() );

    template< typename StackAllocator >
    explicit push_coroutine( coroutine_fn,
                             attributes const&,
                             StackAllocator);
# endif
    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#else
    template< typename Fn >
    explicit push_coroutine( Fn,
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( Fn,
                             attributes const&,
                             StackAllocator);

    template< typename Fn >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const& = attributes() );

    template< typename Fn, typename StackAllocator >
    explicit push_coroutine( BOOST_RV_REF( Fn),
                             attributes const&,
                             StackAllocator);
#endif

    ~push_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    inline push_coroutine( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    inline push_coroutine & operator=( BOOST_RV_REF( push_coroutine) other) BOOST_NOEXCEPT
    {
        push_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    inline void swap( push_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    inline push_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->push();
        return * this;
    }

    struct iterator;
    struct const_iterator;
};



template< typename R >
class pull_coroutine
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_impl< R >            impl_type;
    typedef detail::pull_coroutine_synthesized< R >     synth_type;
    typedef detail::parameters< R >                     param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    explicit pull_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( push_coroutine< R > &);

    explicit pull_coroutine( coroutine_fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, coroutine_fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, coroutine_fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
# endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R >, R, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#endif

    ~pull_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    R get() const
    {
        BOOST_ASSERT( 0 != impl_);

        return impl_->get();
    }

    class iterator
    {
    private:
        pull_coroutine< R > *   c_;
        R                   *   val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! ( * c_) )
            {
                c_ = 0;
                val_ = 0;
                return;
            }
            val_ = c_->impl_->get_pointer();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef std::input_iterator_tag iterator_category;
        typedef typename remove_reference< R >::type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

        typedef pointer   pointer_t;
        typedef reference reference_t;

        iterator() :
            c_( 0), val_( 0)
        {}

        explicit iterator( pull_coroutine< R > * c) :
            c_( c), val_( 0)
        { fetch_(); }

        iterator( iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        iterator & operator=( iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( iterator const& other) const
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( iterator const& other) const
        { return other.c_ != c_ || other.val_ != val_; }

        iterator & operator++()
        {
            increment_();
            return * this;
        }

        iterator operator++( int);

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return * val_;
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_;
        }
    };

    class const_iterator
    {
    private:
        pull_coroutine< R > *   c_;
        R                   *   val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! ( * c_) )
            {
                c_ = 0;
                val_ = 0;
                return;
            }
            val_ = c_->impl_->get_pointer();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef std::input_iterator_tag iterator_category;
        typedef const typename remove_reference< R >::type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

        typedef pointer   pointer_t;
        typedef reference reference_t;

        const_iterator() :
            c_( 0), val_( 0)
        {}

        explicit const_iterator( pull_coroutine< R > const* c) :
            c_( const_cast< pull_coroutine< R > * >( c) ),
            val_( 0)
        { fetch_(); }

        const_iterator( const_iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        const_iterator & operator=( const_iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( const_iterator const& other) const
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( const_iterator const& other) const
        { return other.c_ != c_ || other.val_ != val_; }

        const_iterator & operator++()
        {
            increment_();
            return * this;
        }

        const_iterator operator++( int);

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return * val_;
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_;
        }
    };

    friend class iterator;
    friend class const_iterator;
};

template< typename R >
class pull_coroutine< R & >
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_impl< R & >            impl_type;
    typedef detail::pull_coroutine_synthesized< R & >     synth_type;
    typedef detail::parameters< R & >                     param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    explicit pull_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( push_coroutine< R & > &);

    explicit pull_coroutine( coroutine_fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, coroutine_fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, coroutine_fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
# endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, stack_allocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< R & >, R &, Fn, StackAllocator
        >                                                        object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#endif

    ~pull_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    R & get() const
    { return impl_->get(); }

    class iterator
    {
    private:
        pull_coroutine< R & >   *   c_;
        R                       *   val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! ( * c_) )
            {
                c_ = 0;
                val_ = 0;
                return;
            }
            val_ = c_->impl_->get_pointer();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef std::input_iterator_tag iterator_category;
        typedef typename remove_reference< R >::type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

        typedef pointer   pointer_t;
        typedef reference reference_t;

        iterator() :
            c_( 0), val_( 0)
        {}

        explicit iterator( pull_coroutine< R & > * c) :
            c_( c), val_( 0)
        { fetch_(); }

        iterator( iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        iterator & operator=( iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( iterator const& other) const
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( iterator const& other) const
        { return other.c_ != c_ || other.val_ != val_; }

        iterator & operator++()
        {
            increment_();
            return * this;
        }

        iterator operator++( int);

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return * val_;
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_;
        }
    };

    class const_iterator
    {
    private:
        pull_coroutine< R & >   *   c_;
        R                       *   val_;

        void fetch_()
        {
            BOOST_ASSERT( c_);

            if ( ! ( * c_) )
            {
                c_ = 0;
                val_ = 0;
                return;
            }
            val_ = c_->impl_->get_pointer();
        }

        void increment_()
        {
            BOOST_ASSERT( c_);
            BOOST_ASSERT( * c_);

            ( * c_)();
            fetch_();
        }

    public:
        typedef std::input_iterator_tag iterator_category;
        typedef const typename remove_reference< R >::type value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type * pointer;
        typedef value_type & reference;

        typedef pointer   pointer_t;
        typedef reference reference_t;

        const_iterator() :
            c_( 0), val_( 0)
        {}

        explicit const_iterator( pull_coroutine< R & > const* c) :
            c_( const_cast< pull_coroutine< R & > * >( c) ),
            val_( 0)
        { fetch_(); }

        const_iterator( const_iterator const& other) :
            c_( other.c_), val_( other.val_)
        {}

        const_iterator & operator=( const_iterator const& other)
        {
            if ( this == & other) return * this;
            c_ = other.c_;
            val_ = other.val_;
            return * this;
        }

        bool operator==( const_iterator const& other) const
        { return other.c_ == c_ && other.val_ == val_; }

        bool operator!=( const_iterator const& other) const
        { return other.c_ != c_ || other.val_ != val_; }

        const_iterator & operator++()
        {
            increment_();
            return * this;
        }

        const_iterator operator++( int);

        reference_t operator*() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return * val_;
        }

        pointer_t operator->() const
        {
            if ( ! val_)
                boost::throw_exception(
                    invalid_result() );
            return val_;
        }
    };

    friend class iterator;
    friend class const_iterator;
};

template<>
class pull_coroutine< void >
{
private:
    template< typename V, typename X, typename Y, typename Z >
    friend class detail::push_coroutine_object;

    typedef detail::pull_coroutine_impl< void >            impl_type;
    typedef detail::pull_coroutine_synthesized< void >     synth_type;
    typedef detail::parameters< void >                     param_type;

    struct dummy {};

    impl_type       *   impl_;

    BOOST_MOVABLE_BUT_NOT_COPYABLE( pull_coroutine)

    explicit pull_coroutine( detail::synthesized_t::flag_t, impl_type & impl) :
        impl_( & impl)
    { BOOST_ASSERT( impl_); }

public:
    pull_coroutine() BOOST_NOEXCEPT :
        impl_( 0)
    {}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
    typedef void ( * coroutine_fn)( push_coroutine< void > &);

    explicit pull_coroutine( coroutine_fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, coroutine_fn, stack_allocator
        >                                       object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename StackAllocator >
    explicit pull_coroutine( coroutine_fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, coroutine_fn, StackAllocator
        >                                                                   object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
# endif
    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, stack_allocator
        >                                                       object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, StackAllocator
        >                                                       object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#else
    template< typename Fn >
    explicit pull_coroutine( Fn fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, stack_allocator
        >                                                       object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( Fn fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, StackAllocator
        >                                                       object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
                             attributes const& attrs = attributes() ) :
        impl_( 0)
    {
        // create a stack-context
        stack_context stack_ctx;
        stack_allocator stack_alloc;
        // allocate the coroutine-stack
        stack_alloc.allocate( stack_ctx, attrs.size);
        BOOST_ASSERT( 0 != stack_ctx.sp);
        // typedef of internal coroutine-type
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, stack_allocator
        >                                           object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }

    template< typename Fn, typename StackAllocator >
    explicit pull_coroutine( BOOST_RV_REF( Fn) fn,
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
        typedef detail::pull_coroutine_object<
            push_coroutine< void >, void, Fn, StackAllocator
        >                                           object_t;
        // reserve space on top of coroutine-stack for internal coroutine-type
        std::size_t size = stack_ctx.size - sizeof( object_t);
        BOOST_ASSERT( 0 != size);
        void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
        BOOST_ASSERT( 0 != sp);
        // placement new for internal coroutine
        impl_ = new ( sp) object_t(
                fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
        BOOST_ASSERT( impl_);
        impl_->pull();
    }
#endif

    ~pull_coroutine()
    {
        if ( 0 != impl_)
        {
            impl_->destroy();
            impl_ = 0;
        }
    }

    inline pull_coroutine( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT :
        impl_( 0)
    { swap( other); }

    inline pull_coroutine & operator=( BOOST_RV_REF( pull_coroutine) other) BOOST_NOEXCEPT
    {
        pull_coroutine tmp( boost::move( other) );
        swap( tmp);
        return * this;
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    inline bool operator!() const BOOST_NOEXCEPT
    { return 0 == impl_ || impl_->is_complete(); }

    inline void swap( pull_coroutine & other) BOOST_NOEXCEPT
    { std::swap( impl_, other.impl_); }

    inline pull_coroutine & operator()()
    {
        BOOST_ASSERT( * this);

        impl_->pull();
        return * this;
    }

    struct iterator;
    struct const_iterator;
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
# ifdef BOOST_MSVC
template< typename Arg >
push_coroutine< Arg >::push_coroutine( coroutine_fn fn,
                                       attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, coroutine_fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename StackAllocator >
push_coroutine< Arg >::push_coroutine( coroutine_fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, coroutine_fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
push_coroutine< Arg & >::push_coroutine( coroutine_fn fn,
                                         attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, coroutine_fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( coroutine_fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, coroutine_fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

inline push_coroutine< void >::push_coroutine( coroutine_fn fn,
                                               attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, coroutine_fn, stack_allocator
    >                                                               object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename StackAllocator >
push_coroutine< void >::push_coroutine( coroutine_fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, coroutine_fn, StackAllocator
    >                                                               object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< coroutine_fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}
# endif
template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, stack_allocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, StackAllocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                         attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                        attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            boost::forward< Fn >( fn), attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}
#else
template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( Fn fn,
                                       attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, stack_allocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( Fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, StackAllocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( Fn fn,
                                         attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( Fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn >
push_coroutine< void >::push_coroutine( Fn fn,
                                        attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( Fn fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                       attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, stack_allocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg >, Arg, Fn, StackAllocator
    >                                                    object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                         attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Arg >
template< typename Fn, typename StackAllocator >
push_coroutine< Arg & >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< Arg & >, Arg &, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn,
                                        attributes const& attrs) :
    impl_( 0)
{
    // create a stack-context
    stack_context stack_ctx;
    stack_allocator stack_alloc;
    // allocate the coroutine-stack
    stack_alloc.allocate( stack_ctx, attrs.size);
    BOOST_ASSERT( 0 != stack_ctx.sp);
    // typedef of internal coroutine-type
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, stack_allocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}

template< typename Fn, typename StackAllocator >
push_coroutine< void >::push_coroutine( BOOST_RV_REF( Fn) fn,
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
    typedef detail::push_coroutine_object<
        pull_coroutine< void >, void, Fn, StackAllocator
    >                                                            object_t;
    // reserve space on top of coroutine-stack for internal coroutine-type
    std::size_t size = stack_ctx.size - sizeof( object_t);
    BOOST_ASSERT( 0 != size);
    void * sp = static_cast< char * >( stack_ctx.sp) - sizeof( object_t);
    BOOST_ASSERT( 0 != sp);
    // placement new for internal coroutine
    impl_ = new ( sp) object_t(
            fn, attrs, detail::preallocated( sp, size, stack_ctx), stack_alloc); 
    BOOST_ASSERT( impl_);
}
#endif

template< typename R >
void swap( pull_coroutine< R > & l, pull_coroutine< R > & r) BOOST_NOEXCEPT
{ l.swap( r); }

template< typename Arg >
void swap( push_coroutine< Arg > & l, push_coroutine< Arg > & r) BOOST_NOEXCEPT
{ l.swap( r); }

template< typename R >
typename pull_coroutine< R >::iterator
range_begin( pull_coroutine< R > & c)
{ return typename pull_coroutine< R >::iterator( & c); }

template< typename R >
typename pull_coroutine< R >::const_iterator
range_begin( pull_coroutine< R > const& c)
{ return typename pull_coroutine< R >::const_iterator( & c); }

template< typename R >
typename pull_coroutine< R >::iterator
range_end( pull_coroutine< R > &)
{ return typename pull_coroutine< R >::iterator(); }

template< typename R >
typename pull_coroutine< R >::const_iterator
range_end( pull_coroutine< R > const&)
{ return typename pull_coroutine< R >::const_iterator(); }

template< typename Arg >
typename push_coroutine< Arg >::iterator
range_begin( push_coroutine< Arg > & c)
{ return typename push_coroutine< Arg >::iterator( & c); }

template< typename Arg >
typename push_coroutine< Arg >::iterator
range_end( push_coroutine< Arg > &)
{ return typename push_coroutine< Arg >::iterator(); }

template< typename T >
struct asymmetric_coroutine
{
    typedef push_coroutine< T > push_type;
    typedef pull_coroutine< T > pull_type;
};

// deprecated
template< typename T >
struct coroutine
{
    typedef push_coroutine< T > push_type;
    typedef pull_coroutine< T > pull_type;
};

template< typename R >
typename pull_coroutine< R >::iterator
begin( pull_coroutine< R > & c)
{ return coroutines::range_begin( c); }

template< typename R >
typename pull_coroutine< R >::const_iterator
begin( pull_coroutine< R > const& c)
{ return coroutines::range_begin( c); }

template< typename R >
typename pull_coroutine< R >::iterator
end( pull_coroutine< R > & c)
{ return coroutines::range_end( c); }

template< typename R >
typename pull_coroutine< R >::const_iterator
end( pull_coroutine< R > const& c)
{ return coroutines::range_end( c); }

template< typename R >
typename push_coroutine< R >::iterator
begin( push_coroutine< R > & c)
{ return coroutines::range_begin( c); }

template< typename R >
typename push_coroutine< R >::iterator
end( push_coroutine< R > & c)
{ return coroutines::range_end( c); }

}

// forward declaration of Boost.Range traits to break dependency on it
template<typename C, typename Enabler>
struct range_mutable_iterator;

template<typename C, typename Enabler>
struct range_const_iterator;

template< typename Arg >
struct range_mutable_iterator< coroutines::push_coroutine< Arg >, void >
{ typedef typename coroutines::push_coroutine< Arg >::iterator type; };

template< typename R >
struct range_mutable_iterator< coroutines::pull_coroutine< R >, void >
{ typedef typename coroutines::pull_coroutine< R >::iterator type; };

}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_ASYMMETRIC_COROUTINE_H
