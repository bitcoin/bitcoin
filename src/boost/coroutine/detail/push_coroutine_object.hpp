
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_OBJECT_H
#define BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_OBJECT_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/move/move.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/detail/flags.hpp>
#include <boost/coroutine/detail/preallocated.hpp>
#include <boost/coroutine/detail/push_coroutine_impl.hpp>
#include <boost/coroutine/detail/trampoline_push.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/flags.hpp>
#include <boost/coroutine/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4355)
#endif

namespace boost {
namespace coroutines {
namespace detail {

struct push_coroutine_context
{
    coroutine_context   caller;
    coroutine_context   callee;

    template< typename Coro >
    push_coroutine_context( preallocated const& palloc, Coro *) :
        caller(),
        callee( trampoline_push< Coro >, palloc)
    {}
};

struct push_coroutine_context_void
{
    coroutine_context   caller;
    coroutine_context   callee;

    template< typename Coro >
    push_coroutine_context_void( preallocated const& palloc, Coro *) :
        caller(),
        callee( trampoline_push_void< Coro >, palloc)
    {}
};

template< typename PullCoro, typename R, typename Fn, typename StackAllocator >
class push_coroutine_object : private push_coroutine_context,
                              public push_coroutine_impl< R >
{
private:
    typedef push_coroutine_context                                      ctx_t;
    typedef push_coroutine_impl< R >                                    base_t;
    typedef push_coroutine_object< PullCoro, R, Fn, StackAllocator >    obj_t;

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
    push_coroutine_object( Fn fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    push_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run( R * result)
    {
        BOOST_ASSERT( ! base_t::unwind_requested() );

        base_t::flags_ |= flag_started;
        base_t::flags_ |= flag_running;

        // create push_coroutine
        typename PullCoro::synth_type b( & this->callee, & this->caller, false, result);
        PullCoro pull_coro( synthesized_t::syntesized, b);
        try
        { fn_( pull_coro); }
        catch ( forced_unwind const&)
        {}
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
        catch ( abi::__forced_unwind const&)
        { throw; }
#endif
        catch (...)
        { base_t::except_ = current_exception(); }

        base_t::flags_ |= flag_complete;
        base_t::flags_ &= ~flag_running;
        typename base_t::param_type to;
        this->callee.jump(
            this->caller,
            & to);
        BOOST_ASSERT_MSG( false, "pull_coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

template< typename PullCoro, typename R, typename Fn, typename StackAllocator >
class push_coroutine_object< PullCoro, R &, Fn, StackAllocator > : private push_coroutine_context,
                                                                   public push_coroutine_impl< R & >
{
private:
    typedef push_coroutine_context                                          ctx_t;
    typedef push_coroutine_impl< R & >                                      base_t;
    typedef push_coroutine_object< PullCoro, R &, Fn, StackAllocator >      obj_t;

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
    push_coroutine_object( Fn fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    push_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run( R * result)
    {
        BOOST_ASSERT( ! base_t::unwind_requested() );

        base_t::flags_ |= flag_started;
        base_t::flags_ |= flag_running;

        // create push_coroutine
        typename PullCoro::synth_type b( & this->callee, & this->caller, false, result);
        PullCoro push_coro( synthesized_t::syntesized, b);
        try
        { fn_( push_coro); }
        catch ( forced_unwind const&)
        {}
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
        catch ( abi::__forced_unwind const&)
        { throw; }
#endif
        catch (...)
        { base_t::except_ = current_exception(); }

        base_t::flags_ |= flag_complete;
        base_t::flags_ &= ~flag_running;
        typename base_t::param_type to;
        this->callee.jump(
            this->caller,
            & to);
        BOOST_ASSERT_MSG( false, "pull_coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

template< typename PullCoro, typename Fn, typename StackAllocator >
class push_coroutine_object< PullCoro, void, Fn, StackAllocator > : private push_coroutine_context_void,
                                                                    public push_coroutine_impl< void >
{
private:
    typedef push_coroutine_context_void                                     ctx_t;
    typedef push_coroutine_impl< void >                                     base_t;
    typedef push_coroutine_object< PullCoro, void, Fn, StackAllocator >     obj_t;

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
    push_coroutine_object( Fn fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
        fn_( fn),
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}
#endif

    push_coroutine_object( BOOST_RV_REF( Fn) fn, attributes const& attrs,
                           preallocated const& palloc,
                           StackAllocator const& stack_alloc) BOOST_NOEXCEPT :
        ctx_t( palloc, this),
        base_t( & this->caller,
                & this->callee,
                stack_unwind == attrs.do_unwind),
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
        fn_( fn),
#else
        fn_( boost::forward< Fn >( fn) ),
#endif
        stack_ctx_( palloc.sctx),
        stack_alloc_( stack_alloc)
    {}

    void run()
    {
        BOOST_ASSERT( ! base_t::unwind_requested() );

        base_t::flags_ |= flag_started;
        base_t::flags_ |= flag_running;

        // create push_coroutine
        typename PullCoro::synth_type b( & this->callee, & this->caller, false);
        PullCoro push_coro( synthesized_t::syntesized, b);
        try
        { fn_( push_coro); }
        catch ( forced_unwind const&)
        {}
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
        catch ( abi::__forced_unwind const&)
        { throw; }
#endif
        catch (...)
        { base_t::except_ = current_exception(); }

        base_t::flags_ |= flag_complete;
        base_t::flags_ &= ~flag_running;
        typename base_t::param_type to;
        this->callee.jump(
            this->caller,
            & to);
        BOOST_ASSERT_MSG( false, "pull_coroutine is complete");
    }

    void destroy()
    { deallocate_( this); }
};

}}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_OBJECT_H
