
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_IMPL_H
#define BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_IMPL_H

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/utility.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/detail/flags.hpp>
#include <boost/coroutine/detail/parameters.hpp>
#include <boost/coroutine/detail/preallocated.hpp>
#include <boost/coroutine/detail/trampoline.hpp>
#include <boost/coroutine/exceptions.hpp>
#include <boost/coroutine/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename R >
class symmetric_coroutine_impl : private noncopyable
{
public:
    typedef parameters< R >                           param_type;

    symmetric_coroutine_impl( preallocated const& palloc,
                              bool unwind) BOOST_NOEXCEPT :
        flags_( 0),
        caller_(),
        callee_( trampoline< symmetric_coroutine_impl< R > >, palloc)
    {
        if ( unwind) flags_ |= flag_force_unwind;
    }

    virtual ~symmetric_coroutine_impl() {}

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool is_started() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_started); }

    bool is_running() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_running); }

    bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    void unwind_stack() BOOST_NOEXCEPT
    {
        if ( is_started() && ! is_complete() && force_unwind() )
        {
            flags_ |= flag_unwind_stack;
            flags_ |= flag_running;
            param_type to( unwind_t::force_unwind);
            caller_.jump(
                callee_,
                & to);
            flags_ &= ~flag_running;
            flags_ &= ~flag_unwind_stack;

            BOOST_ASSERT( is_complete() );
        }
    }

    void resume( R r) BOOST_NOEXCEPT
    {
        param_type to( const_cast< R * >( & r), this);
        resume_( & to);
    }

    R * yield()
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );

        flags_ &= ~flag_running;
        param_type to;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                    caller_,
                    & to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
        BOOST_ASSERT( from->data);
        return from->data;
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X > * other, X x)
    {
        typename symmetric_coroutine_impl< X >::param_type to( & x, other);
        return yield_to_( other, & to);
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X & > * other, X & x)
    {
        typename symmetric_coroutine_impl< X & >::param_type to( & x, other);
        return yield_to_( other, & to);
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X > * other)
    {
        typename symmetric_coroutine_impl< X >::param_type to( other);
        return yield_to_( other, & to);
    }

    virtual void run( R *) BOOST_NOEXCEPT = 0;

    virtual void destroy() = 0;

protected:
    template< typename X >
    friend class symmetric_coroutine_impl;

    int                 flags_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    void resume_( param_type * to) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! is_running() );
        BOOST_ASSERT( ! is_complete() );

        flags_ |= flag_running;
        caller_.jump(
            callee_,
            to);
        flags_ &= ~flag_running;
    }

    template< typename Other >
    R * yield_to_( Other * other, typename Other::param_type * to)
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! other->is_running() );
        BOOST_ASSERT( ! other->is_complete() );

        other->caller_ = caller_;
        flags_ &= ~flag_running;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                    other->callee_,
                    to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
        BOOST_ASSERT( from->data);
        return from->data;
    }
};

template< typename R >
class symmetric_coroutine_impl< R & > : private noncopyable
{
public:
    typedef parameters< R & >                         param_type;

    symmetric_coroutine_impl( preallocated const& palloc,
                              bool unwind) BOOST_NOEXCEPT :
        flags_( 0),
        caller_(),
        callee_( trampoline< symmetric_coroutine_impl< R > >, palloc)
    {
        if ( unwind) flags_ |= flag_force_unwind;
    }

    virtual ~symmetric_coroutine_impl() {}

    bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    bool is_started() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_started); }

    bool is_running() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_running); }

    bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    void unwind_stack() BOOST_NOEXCEPT
    {
        if ( is_started() && ! is_complete() && force_unwind() )
        {
            flags_ |= flag_unwind_stack;
            flags_ |= flag_running;
            param_type to( unwind_t::force_unwind);
            caller_.jump(
                callee_,
                & to);
            flags_ &= ~flag_running;
            flags_ &= ~flag_unwind_stack;

            BOOST_ASSERT( is_complete() );
        }
    }

    void resume( R & arg) BOOST_NOEXCEPT
    {
        param_type to( & arg, this);
        resume_( & to);
    }

    R * yield()
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );

        flags_ &= ~flag_running;
        param_type to;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                    caller_,
                    & to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
        BOOST_ASSERT( from->data);
        return from->data;
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X > * other, X x)
    {
        typename symmetric_coroutine_impl< X >::param_type to( & x, other);
        return yield_to_( other, & to);
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X & > * other, X & x)
    {
        typename symmetric_coroutine_impl< X & >::param_type to( & x, other);
        return yield_to_( other, & to);
    }

    template< typename X >
    R * yield_to( symmetric_coroutine_impl< X > * other)
    {
        typename symmetric_coroutine_impl< X >::param_type to( other);
        return yield_to_( other, & to);
    }

    virtual void run( R *) BOOST_NOEXCEPT = 0;

    virtual void destroy() = 0;

protected:
    template< typename X >
    friend class symmetric_coroutine_impl;

    int                 flags_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    void resume_( param_type * to) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! is_running() );
        BOOST_ASSERT( ! is_complete() );

        flags_ |= flag_running;
        caller_.jump(
            callee_,
            to);
        flags_ &= ~flag_running;
    }

    template< typename Other >
    R * yield_to_( Other * other, typename Other::param_type * to)
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! other->is_running() );
        BOOST_ASSERT( ! other->is_complete() );

        other->caller_ = caller_;
        flags_ &= ~flag_running;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                    other->callee_,
                    to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
        BOOST_ASSERT( from->data);
        return from->data;
    }
};

template<>
class symmetric_coroutine_impl< void > : private noncopyable
{
public:
    typedef parameters< void >                          param_type;

    symmetric_coroutine_impl( preallocated const& palloc,
                              bool unwind) BOOST_NOEXCEPT :
        flags_( 0),
        caller_(),
        callee_( trampoline_void< symmetric_coroutine_impl< void > >, palloc)
    {
        if ( unwind) flags_ |= flag_force_unwind;
    }

    virtual ~symmetric_coroutine_impl() {}

    inline bool force_unwind() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_force_unwind); }

    inline bool unwind_requested() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_unwind_stack); }

    inline bool is_started() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_started); }

    inline bool is_running() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_running); }

    inline bool is_complete() const BOOST_NOEXCEPT
    { return 0 != ( flags_ & flag_complete); }

    inline void unwind_stack() BOOST_NOEXCEPT
    {
        if ( is_started() && ! is_complete() && force_unwind() )
        {
            flags_ |= flag_unwind_stack;
            flags_ |= flag_running;
            param_type to( unwind_t::force_unwind);
            caller_.jump(
                callee_,
                & to);
            flags_ &= ~flag_running;
            flags_ &= ~flag_unwind_stack;

            BOOST_ASSERT( is_complete() );
        }
    }

    inline void resume() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! is_running() );
        BOOST_ASSERT( ! is_complete() );

        param_type to( this);
        flags_ |= flag_running;
        caller_.jump(
            callee_,
            & to);
        flags_ &= ~flag_running;
    }

    inline void yield()
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );

        flags_ &= ~flag_running;
        param_type to;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                     caller_,
                    & to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
    }

    template< typename X >
    void yield_to( symmetric_coroutine_impl< X > * other, X x)
    {
        typename symmetric_coroutine_impl< X >::param_type to( & x, other);
        yield_to_( other, & to);
    }

    template< typename X >
    void yield_to( symmetric_coroutine_impl< X & > * other, X & x)
    {
        typename symmetric_coroutine_impl< X & >::param_type to( & x, other);
        yield_to_( other, & to);
    }

    template< typename X >
    void yield_to( symmetric_coroutine_impl< X > * other)
    {
        typename symmetric_coroutine_impl< X >::param_type to( other);
        yield_to_( other, & to);
    }

    virtual void run() BOOST_NOEXCEPT = 0;

    virtual void destroy() = 0;

protected:
    template< typename X >
    friend class symmetric_coroutine_impl;

    int                 flags_;
    coroutine_context   caller_;
    coroutine_context   callee_;

    template< typename Other >
    void yield_to_( Other * other, typename Other::param_type * to)
    {
        BOOST_ASSERT( is_running() );
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! other->is_running() );
        BOOST_ASSERT( ! other->is_complete() );

        other->caller_ = caller_;
        flags_ &= ~flag_running;
        param_type * from(
            static_cast< param_type * >(
                callee_.jump(
                    other->callee_,
                    to) ) );
        flags_ |= flag_running;
        if ( from->do_unwind) throw forced_unwind();
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_SYMMETRIC_COROUTINE_IMPL_H
