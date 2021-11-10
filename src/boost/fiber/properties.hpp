//          Copyright Nat Goodspeed 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Define fiber_properties, a base class from which a library consumer can
// derive a subclass with specific properties important to a user-coded
// scheduler.

#ifndef BOOST_FIBERS_PROPERTIES_HPP
#define BOOST_FIBERS_PROPERTIES_HPP

#include <boost/fiber/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

# if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable:4275)
# endif

namespace boost {
namespace fibers {

class context;

namespace algo {

class algorithm;

}

class BOOST_FIBERS_DECL fiber_properties {
protected:
    // initialized by constructor
    context         *   ctx_;
    // set every time this fiber becomes READY
    algo::algorithm *   algo_{ nullptr };

    // Inform the relevant algorithm instance that something important
    // has changed, so it can (presumably) adjust its data structures
    // accordingly.
    void notify() noexcept;

public:
    // Any specific property setter method, after updating the relevant
    // instance variable, can/should call notify().

    // fiber_properties, and by implication every subclass, must accept a back
    // pointer to its context.
    explicit fiber_properties( context * ctx) noexcept :
        ctx_{ ctx } {
    }

    // We need a virtual destructor (hence a vtable) because fiber_properties
    // is stored polymorphically (as fiber_properties*) in context, and
    // destroyed via that pointer.
    virtual ~fiber_properties() = default;

    // not really intended for public use, but algorithm_with_properties
    // must be able to call this
    void set_algorithm( algo::algorithm * algo) noexcept {
        algo_ = algo;
    }
};

}} // namespace boost::fibers

# if defined(BOOST_MSVC)
# pragma warning(pop)
# endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_FIBERS_PROPERTIES_HPP
