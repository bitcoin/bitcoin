
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
#define BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP

#include <algorithm>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/detail/config.hpp>

#include <boost/context/fiber.hpp>

#include <boost/coroutine2/detail/config.hpp>
#include <boost/coroutine2/detail/wrap.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines2 {
namespace detail {

// push_coroutine< T >

template< typename T >
void
push_coroutine< T >::control_block::destroy( control_block * cb) noexcept {
    boost::context::fiber c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::control_block::control_block( context::preallocated palloc, StackAllocator && salloc,
                                                   Fn && fn) :
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::fiber && c) mutable {
               // create synthesized pull_coroutine< T >
               typename pull_coroutine< T >::control_block synthesized_cb{ this, c };
               pull_coroutine< T > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = std::move( other->c).resume();
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
                   } catch ( abi::__forced_unwind const&) {
                       throw;
#endif
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
               other->c = std::move( other->c).resume();
               return std::move( other->c);
            },
            std::forward< Fn >( fn) ) },
#else
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       [this,fn_=std::forward< Fn >( fn)](boost::context::fiber && c) mutable {
          // create synthesized pull_coroutine< T >
          typename pull_coroutine< T >::control_block synthesized_cb{ this, c };
          pull_coroutine< T > synthesized{ & synthesized_cb };
          other = & synthesized_cb;
          other->c = std::move( other->c).resume();
          if ( state_t::none == ( state & state_t::destroy) ) {
              try {
                  auto fn = std::move( fn_);
                  // call coroutine-fn with synthesized pull_coroutine as argument
                  fn( synthesized);
              } catch ( boost::context::detail::forced_unwind const&) {
                  throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
              } catch ( abi::__forced_unwind const&) {
                  throw;
#endif
              } catch (...) {
                  // store other exceptions in exception-pointer
                  except = std::current_exception();
              }
          }
          // set termination flags
          state |= state_t::complete;
          // jump back
          return std::move( other->c).resume();
       } },
#endif
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
        c = std::move( c).resume();
}

template< typename T >
push_coroutine< T >::control_block::control_block( typename pull_coroutine< T >::control_block * cb,
                                                   boost::context::fiber & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

template< typename T >
void
push_coroutine< T >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T const& data) {
    // pass data to other context
    other->set( data);
    // resume other context
    c = std::move( c).resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
void
push_coroutine< T >::control_block::resume( T && data) {
    // pass data to other context
    other->set( std::move( data) );
    // resume other context
    c = std::move( c).resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
bool
push_coroutine< T >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}


// push_coroutine< T & >

template< typename T >
void
push_coroutine< T & >::control_block::destroy( control_block * cb) noexcept {
    boost::context::fiber c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T & >::control_block::control_block( context::preallocated palloc, StackAllocator && salloc,
                                                     Fn && fn) :
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::fiber && c) mutable {
               // create synthesized pull_coroutine< T & >
               typename pull_coroutine< T & >::control_block synthesized_cb{ this, c };
               pull_coroutine< T & > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = std::move( other->c).resume();
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
                   } catch ( abi::__forced_unwind const&) {
                       throw;
#endif
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
              other->c = std::move( other->c).resume();
              return std::move( other->c);
            },
            std::forward< Fn >( fn) ) },
#else
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       [this,fn_=std::forward< Fn >( fn)](boost::context::fiber && c) mutable {
          // create synthesized pull_coroutine< T & >
          typename pull_coroutine< T & >::control_block synthesized_cb{ this, c };
          pull_coroutine< T & > synthesized{ & synthesized_cb };
          other = & synthesized_cb;
          other->c = std::move( other->c).resume();
          if ( state_t::none == ( state & state_t::destroy) ) {
              try {
                  auto fn = std::move( fn_);
                  // call coroutine-fn with synthesized pull_coroutine as argument
                  fn( synthesized);
              } catch ( boost::context::detail::forced_unwind const&) {
                  throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
              } catch ( abi::__forced_unwind const&) {
                  throw;
#endif
              } catch (...) {
                  // store other exceptions in exception-pointer
                  except = std::current_exception();
              }
          }
          // set termination flags
          state |= state_t::complete;
          // jump back
          other->c = std::move( other->c).resume();
          return std::move( other->c);
       } },
#endif
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
        c = std::move( c).resume();
}

template< typename T >
push_coroutine< T & >::control_block::control_block( typename pull_coroutine< T & >::control_block * cb,
                                                     boost::context::fiber & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

template< typename T >
void
push_coroutine< T & >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

template< typename T >
void
push_coroutine< T & >::control_block::resume( T & data) {
    // pass data to other context
    other->set( data);
    // resume other context
    c = std::move( c).resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

template< typename T >
bool
push_coroutine< T & >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}


// push_coroutine< void >

inline
void
push_coroutine< void >::control_block::destroy( control_block * cb) noexcept {
    boost::context::fiber c = std::move( cb->c);
    // destroy control structure
    cb->~control_block();
    // destroy coroutine's stack
    cb->state |= state_t::destroy;
}

template< typename StackAllocator, typename Fn >
push_coroutine< void >::control_block::control_block( context::preallocated palloc, StackAllocator && salloc, Fn && fn) :
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       wrap( [this](typename std::decay< Fn >::type & fn_,boost::context::fiber && c) mutable {
               // create synthesized pull_coroutine< void >
               typename pull_coroutine< void >::control_block synthesized_cb{ this, c };
               pull_coroutine< void > synthesized{ & synthesized_cb };
               other = & synthesized_cb;
               other->c = std::move( other->c).resume();
               if ( state_t::none == ( state & state_t::destroy) ) {
                   try {
                       auto fn = std::move( fn_);
                       // call coroutine-fn with synthesized pull_coroutine as argument
                       fn( synthesized);
                   } catch ( boost::context::detail::forced_unwind const&) {
                       throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
                   } catch ( abi::__forced_unwind const&) {
                       throw;
#endif
                   } catch (...) {
                       // store other exceptions in exception-pointer
                       except = std::current_exception();
                   }
               }
               // set termination flags
               state |= state_t::complete;
               // jump back
               other->c = std::move( other->c).resume();
               return std::move( other->c);
            },
            std::forward< Fn >( fn) ) },
#else
    c{ std::allocator_arg, palloc, std::forward< StackAllocator >( salloc),
       [this,fn_=std::forward< Fn >( fn)](boost::context::fiber && c) mutable {
          // create synthesized pull_coroutine< void >
          typename pull_coroutine< void >::control_block synthesized_cb{ this, c};
          pull_coroutine< void > synthesized{ & synthesized_cb };
          other = & synthesized_cb;
          other->c = std::move( other->c).resume();
          if ( state_t::none == ( state & state_t::destroy) ) {
              try {
                  auto fn = std::move( fn_);
                  // call coroutine-fn with synthesized pull_coroutine as argument
                  fn( synthesized);
              } catch ( boost::context::detail::forced_unwind const&) {
                  throw;
#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
              } catch ( abi::__forced_unwind const&) {
                  throw;
#endif
              } catch (...) {
                  // store other exceptions in exception-pointer
                  except = std::current_exception();
              }
          }
          // set termination flags
          state |= state_t::complete;
          // jump back
          other->c = std::move( other->c).resume();
          return std::move( other->c);
       } },
#endif
    other{ nullptr },
    state{ state_t::unwind },
    except{} {
        c = std::move( c).resume();
}

inline
push_coroutine< void >::control_block::control_block( pull_coroutine< void >::control_block * cb,
                                                      boost::context::fiber & c_) noexcept :
    c{ std::move( c_) },
    other{ cb },
    state{ state_t::none },
    except{} {
}

inline
void
push_coroutine< void >::control_block::deallocate() noexcept {
    if ( state_t::none != ( state & state_t::unwind) ) {
        destroy( this);
    }
}

inline
void
push_coroutine< void >::control_block::resume() {
    c = std::move( c).resume();
    if ( except) {
        std::rethrow_exception( except);
    }
}

inline
bool
push_coroutine< void >::control_block::valid() const noexcept {
    return state_t::none == ( state & state_t::complete );
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES2_DETAIL_PUSH_CONTROL_BLOCK_IPP
