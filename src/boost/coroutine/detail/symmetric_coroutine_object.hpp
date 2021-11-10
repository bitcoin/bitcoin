
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_OBJECT_H
#define BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_OBJECT_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/flags.hpp>
#include <boost/coroutine/detail/preallocated.hpp>
#include <boost/coroutine/detail/symmetric_coroutine_impl.hpp>
#include <boost/coroutine/detail/symmetric_coroutine_yield.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {

struct stack_context;

namespace detail {

template< typename R, typename Fn, typename StackAllocator >
class symmetric_coroutine_object : public symmetric_coroutine_impl< R >
{
private:
    typedef symmetric_coroutine_impl< R >                       impl_t;
    typedef symmetric_coroutine_object< R, Fn, StackAllocator > obj_t;

    Fn                  fn_;
    stack_context       stack_ctx_;
    StackAllocator      stack_alloc_;

    static void deallocate_( obj_t * obj)
    {
        stack_context stack_ctx( obj->stack_ctx_);
        StackAllocator stack_alloc( obj->stack_alloc_);
        obj->unwind_stack();
        obj->~obj_t();
        stack_alloc.deallocate( stack_ctx);
    }

public:
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    symmetric_coroutine_object( Fn fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    symmetric_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run( R * r) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! impl_t::unwind_requested() );

        impl_t::flags_ |= flag_started;
        impl_t::flags_ |= flag_running;
        try
        {
            symmetric_coroutine_yield< R > yc( this, r);
            fn_( yc);
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { std::terminate(); }

        impl_t::flags_ |= flag_complete;
        impl_t::flags_ &= ~flag_running;
        typename impl_t::param_type to;
        impl_t::callee_.jump(
            impl_t::caller_, 
            & to);
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

template< typename R, typename Fn, typename StackAllocator >
class symmetric_coroutine_object< R &, Fn, StackAllocator > : public symmetric_coroutine_impl< R & >
{
private:
    typedef symmetric_coroutine_impl< R & >                         impl_t;
    typedef symmetric_coroutine_object< R &, Fn, StackAllocator >   obj_t;

    Fn                  fn_;
    stack_context       stack_ctx_;
    StackAllocator      stack_alloc_;

    static void deallocate_( obj_t * obj)
    {
        stack_context stack_ctx( obj->stack_ctx_);
        StackAllocator stack_alloc( obj->stack_alloc_);
        obj->unwind_stack();
        obj->~obj_t();
        stack_alloc.deallocate( stack_ctx);
    }

public:
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    symmetric_coroutine_object( Fn fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    symmetric_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run( R * r) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! impl_t::unwind_requested() );

        impl_t::flags_ |= flag_started;
        impl_t::flags_ |= flag_running;
        try
        {
            symmetric_coroutine_yield< R & > yc( this, r);
            fn_( yc);
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { std::terminate(); }

        impl_t::flags_ |= flag_complete;
        impl_t::flags_ &= ~flag_running;
        typename impl_t::param_type to;
        impl_t::callee_.jump(
            impl_t::caller_, 
            & to);
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

template< typename Fn, typename StackAllocator >
class symmetric_coroutine_object< void, Fn, StackAllocator > : public symmetric_coroutine_impl< void >
{
private:
    typedef symmetric_coroutine_impl< void >                        impl_t;
    typedef symmetric_coroutine_object< void, Fn, StackAllocator >  obj_t;

    Fn                  fn_;
    stack_context       stack_ctx_;
    StackAllocator      stack_alloc_;

    static void deallocate_( obj_t * obj)
    {
        stack_context stack_ctx( obj->stack_ctx_);
        StackAllocator stack_alloc( obj->stack_alloc_);
        obj->unwind_stack();
        obj->~obj_t();
        stack_alloc.deallocate( stack_ctx);
    }

public:
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    symmetric_coroutine_object( Fn fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    symmetric_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                                preallocated const& palloc,
                                StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        impl_t( palloc,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! impl_t::unwind_requested() );

        impl_t::flags_ |= flag_started;
        impl_t::flags_ |= flag_running;
        try
        {
            symmetric_coroutine_yield< void > yc( this);
            fn_( yc);
        }
        catch ( forced_unwind const&)
        {}
        catch (...)
        { std::terminate(); }

        impl_t::flags_ |= flag_complete;
        impl_t::flags_ &= ~flag_running;
        typename impl_t::param_type to;
        impl_t::callee_.jump(
            impl_t::caller_, 
            & to);
        BOOST_ASSERT_MSG( false, "coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_OBJECT_H
