
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_IPP
#define BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_IPP

#include <algorithm>
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

// pull_coroutine< T >

template< typename T >
pull_coroutine< T >::pull_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename T >
bool
pull_coroutine< T >::has_result_() const noexcept {
    return nullptr != cb_->other->t;
}

template< typename T >
template< typename Fn,
          typename
>
pull_coroutine< T >::pull_coroutine( Fn && fn) :
    pull_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename T >
template< typename StackAllocator, typename Fn >
pull_coroutine< T >::pull_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
    if ( ! cb_->valid() ) {
        cb_->deallocate();
        cb_ = nullptr;
    }
}

template< typename T >
pull_coroutine< T >::~pull_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

template< typename T >
pull_coroutine< T >::pull_coroutine( pull_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

template< typename T >
pull_coroutine< T > & 
pull_coroutine< T >::operator()() {
    cb_->resume();
    return * this;
}

template< typename T >
pull_coroutine< T >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
pull_coroutine< T >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}

template< typename T >
T
pull_coroutine< T >::get() noexcept {
    return std::move( cb_->get() );
}


// pull_coroutine< T & >

template< typename T >
pull_coroutine< T & >::pull_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename T >
bool
pull_coroutine< T & >::has_result_() const noexcept {
    return nullptr != cb_->other->t;
}

template< typename T >
template< typename Fn,
          typename
>
pull_coroutine< T & >::pull_coroutine( Fn && fn) :
    pull_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename T >
template< typename StackAllocator, typename Fn >
pull_coroutine< T & >::pull_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
    if ( ! cb_->valid() ) {
        cb_->deallocate();
        cb_ = nullptr;
    }
}

template< typename T >
pull_coroutine< T & >::~pull_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

template< typename T >
pull_coroutine< T & >::pull_coroutine( pull_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

template< typename T >
pull_coroutine< T & > &
pull_coroutine< T & >::operator()() {
    cb_->resume();
    return * this;
}

template< typename T >
pull_coroutine< T & >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

template< typename T >
bool
pull_coroutine< T & >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}

template< typename T >
T &
pull_coroutine< T & >::get() noexcept {
    return cb_->get();
}


// pull_coroutine< void >

inline
pull_coroutine< void >::pull_coroutine( control_block * cb) noexcept :
    cb_{ cb } {
}

template< typename Fn,
          typename
>
pull_coroutine< void >::pull_coroutine( Fn && fn) :
    pull_coroutine{ default_stack(), std::forward< Fn >( fn) } {
}

template< typename StackAllocator, typename Fn >
pull_coroutine< void >::pull_coroutine( StackAllocator && salloc, Fn && fn) :
    cb_{ create_control_block< control_block >( std::forward< StackAllocator >( salloc), std::forward< Fn >( fn) ) } {
    if ( ! cb_->valid() ) {
        cb_->deallocate();
        cb_ = nullptr;
    }
}

inline
pull_coroutine< void >::~pull_coroutine() {
    if ( nullptr != cb_) {
        cb_->deallocate();
    }
}

inline
pull_coroutine< void >::pull_coroutine( pull_coroutine && other) noexcept :
    cb_{ nullptr } {
    std::swap( cb_, other.cb_);
}

inline
pull_coroutine< void > &
pull_coroutine< void >::operator()() {
    cb_->resume();
    return * this;
}

inline
pull_coroutine< void >::operator bool() const noexcept {
    return nullptr != cb_ && cb_->valid();
}

inline
bool
pull_coroutine< void >::operator!() const noexcept {
    return nullptr == cb_ || ! cb_->valid();
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PULL_COROUTINE_IPP
