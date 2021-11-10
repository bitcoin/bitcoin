//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_FIBERS_ALGO_ALGORITHM_H
#define BOOST_FIBERS_ALGO_ALGORITHM_H

#include <atomic>
#include <chrono>
#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/properties.hpp>
#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace fibers {

class context;

namespace algo {

class BOOST_FIBERS_DECL algorithm {
private:
    std::atomic< std::size_t >    use_count_{ 0 };

public:
    typedef intrusive_ptr< algorithm >  ptr_t;

    virtual ~algorithm() = default;

    virtual void awakened( context *) noexcept = 0;

    virtual context * pick_next() noexcept = 0;

    virtual bool has_ready_fibers() const noexcept = 0;

    virtual void suspend_until( std::chrono::steady_clock::time_point const&) noexcept = 0;

    virtual void notify() noexcept = 0;

    #if !defined(BOOST_EMBTC)
      
    friend void intrusive_ptr_add_ref( algorithm * algo) noexcept {
        BOOST_ASSERT( nullptr != algo);
        algo->use_count_.fetch_add( 1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release( algorithm * algo) noexcept {
        BOOST_ASSERT( nullptr != algo);
        if ( 1 == algo->use_count_.fetch_sub( 1, std::memory_order_release) ) {
            std::atomic_thread_fence( std::memory_order_acquire);
            delete algo;
        }
    }
    
    #else
      
    friend void intrusive_ptr_add_ref( algorithm * algo) noexcept;
    friend void intrusive_ptr_release( algorithm * algo) noexcept;
    
    #endif
      
};

#if defined(BOOST_EMBTC)

    inline void intrusive_ptr_add_ref( algorithm * algo) noexcept {
        BOOST_ASSERT( nullptr != algo);
        algo->use_count_.fetch_add( 1, std::memory_order_relaxed);
    }

    inline void intrusive_ptr_release( algorithm * algo) noexcept {
        BOOST_ASSERT( nullptr != algo);
        if ( 1 == algo->use_count_.fetch_sub( 1, std::memory_order_release) ) {
            std::atomic_thread_fence( std::memory_order_acquire);
            delete algo;
        }
    }
    
#endif
    
class BOOST_FIBERS_DECL algorithm_with_properties_base : public algorithm {
public:
    // called by fiber_properties::notify() -- don't directly call
    virtual void property_change_( context * ctx, fiber_properties * props) noexcept = 0;

protected:
    static fiber_properties* get_properties( context * ctx) noexcept;
    static void set_properties( context * ctx, fiber_properties * p) noexcept;
};

template< typename PROPS >
struct algorithm_with_properties : public algorithm_with_properties_base {
    typedef algorithm_with_properties_base super;

    // Mark this override 'final': algorithm_with_properties subclasses
    // must override awakened() with properties parameter instead. Otherwise
    // you'd have to remember to start every subclass awakened() override
    // with: algorithm_with_properties<PROPS>::awakened(fb);
    void awakened( context * ctx) noexcept final {
        fiber_properties * props = super::get_properties( ctx);
        if ( BOOST_LIKELY( nullptr == props) ) {
            // TODO: would be great if PROPS could be allocated on the new
            // fiber's stack somehow
            props = new_properties( ctx);
            // It is not good for new_properties() to return 0.
            BOOST_ASSERT_MSG( props, "new_properties() must return non-NULL");
            // new_properties() must return instance of (a subclass of) PROPS
            BOOST_ASSERT_MSG( dynamic_cast< PROPS * >( props),
                              "new_properties() must return properties class");
            super::set_properties( ctx, props);
        }
        // Set algo_ again every time this fiber becomes READY. That
        // handles the case of a fiber migrating to a new thread with a new
        // algorithm subclass instance.
        props->set_algorithm( this);

        // Okay, now forward the call to subclass override.
        awakened( ctx, properties( ctx) );
    }

    // subclasses override this method instead of the original awakened()
    virtual void awakened( context *, PROPS &) noexcept = 0;

    // used for all internal calls
    PROPS & properties( context * ctx) noexcept {
        return static_cast< PROPS & >( * super::get_properties( ctx) );
    }

    // override this to be notified by PROPS::notify()
    virtual void property_change( context * /* ctx */, PROPS & /* props */) noexcept {
    }

    // implementation for algorithm_with_properties_base method
    void property_change_( context * ctx, fiber_properties * props) noexcept final {
        property_change( ctx, * static_cast< PROPS * >( props) );
    }

    // Override this to customize instantiation of PROPS, e.g. use a different
    // allocator. Each PROPS instance is associated with a particular
    // context.
    virtual fiber_properties * new_properties( context * ctx) {
        return new PROPS( ctx);
    }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_ALGO_ALGORITHM_H
