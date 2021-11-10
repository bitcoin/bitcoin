
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_SYNTHESIZED_H
#define BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_SYNTHESIZED_H

#include <boost/config.hpp>

#include <boost/coroutine/detail/config.hpp>
#include <boost/coroutine/detail/coroutine_context.hpp>
#include <boost/coroutine/detail/push_coroutine_impl.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace coroutines {
namespace detail {

template< typename R >
class push_coroutine_synthesized : public push_coroutine_impl< R >
{
private:
    typedef push_coroutine_impl< R >                                    impl_t;

public:
    push_coroutine_synthesized( coroutine_context * caller,
                                coroutine_context * callee,
                                bool unwind) :
        impl_t( caller, callee, unwind)
    {}

    void destroy() {}
};

template< typename R >
class push_coroutine_synthesized< R & > : public push_coroutine_impl< R & >
{
private:
    typedef push_coroutine_impl< R & >                                    impl_t;

public:
    push_coroutine_synthesized( coroutine_context * caller,
                                coroutine_context * callee,
                                bool unwind) :
        impl_t( caller, callee, unwind)
    {}

    void destroy() {}
};

template<>
class push_coroutine_synthesized< void > : public push_coroutine_impl< void >
{
private:
    typedef push_coroutine_impl< void >                                    impl_t;

public:
    push_coroutine_synthesized( coroutine_context * caller,
                                coroutine_context * callee,
                                bool unwind) :
        impl_t( caller, callee, unwind)
    {}

    inline void destroy() {}
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_COROUTINES_DETAIL_PUSH_COROUTINE_SYNTHESIZED_H
