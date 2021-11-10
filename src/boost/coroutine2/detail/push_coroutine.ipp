
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP

#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/detail/create_control_block.ipp>
#include <boost/coroutine2/detail/disable_overload.hpp>
#include <boost/coroutine2/fixedsize_stack.hpp>
#include <boost/coroutine2/segmented_stack.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
push_coroutine< T >::push_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename T >
template< typename Fn,
          typename
>
push_coroutine< T >::push_coroutine( Fn && fn) :
    push_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::push_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
}

template< typename T >
push_coroutine< T >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

template< typename T >
push_coroutine< T >::push_coroutine( push_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

template< typename T >
push_coroutine< T > &
push_coroutine< T >::operator()( T const& t) {
    cb_->resume( t);
    return * this;
}

template< typename T >
push_coroutine< T > &
push_coroutine< T >::operator()( T && t) {
    cb_->resume( std::forward< T >( t) );
    return * this;
}

template< typename T >
push_coroutine< T >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
push_coroutine< T >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}


// push_coroutine< T & >

template< typename T >
push_coroutine< T & >::push_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename T >
template< typename Fn,
          typename
>
push_coroutine< T & >::push_coroutine( Fn && fn) :
    push_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::push_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
}

template< typename T >
push_coroutine< T & >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

template< typename T >
push_coroutine< T & >::push_coroutine( push_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

template< typename T >
push_coroutine< T & > &
push_coroutine< T & >::operator()( T & t) {
    cb_->resume( t);
    return * this;
}

template< typename T >
push_coroutine< T & >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
push_coroutine< T & >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}


// push_coroutine< void >

inline
push_coroutine< void >::push_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename Fn,
          typename
>
push_coroutine< void >::push_coroutine( Fn && fn) :
    push_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename StackAllocator, typename Fn >
push_coroutine< void >::push_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
}

inline
push_coroutine< void >::~push_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

inline
push_coroutine< void >::push_coroutine( push_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

inline
push_coroutine< void > &
push_coroutine< void >::operator()() {
    cb_->resume();
    return * this;
}

inline
push_coroutine< void >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

inline
bool
push_coroutine< void >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_COROUTINE_IPP
