//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/heap/tags.hpp>

#include <atomic>
#include <memory>
#include <utility>

namespace immer {

/*!
 * Provides transience ownership tracking when a *tracing garbage
 * collector* is used instead of reference counting.
 *
 * @rst
 *
 * .. warning:: Using this policy without an allocation scheme that
 *    includes automatic tracing garbage collection may cause memory
 *    leaks.
 *
 * @endrst
 */
struct gc_transience_policy
{
    template <typename HeapPolicy>
    struct apply
    {
        struct type
        {
            using heap_ = typename HeapPolicy::type;

            struct edit
            {
                void* v;
                edit(void* v_)
                    : v{v_}
                {}
                edit() = delete;
                bool operator==(edit x) const { return v == x.v; }
                bool operator!=(edit x) const { return v != x.v; }
            };

            struct owner
            {
                void* make_token_()
                {
                    return heap_::allocate(1, norefs_tag{});
                };

                mutable std::atomic<void*> token_;

                operator edit() { return {token_}; }

                owner()
                    : token_{make_token_()}
                {}
                owner(const owner& o)
                    : token_{make_token_()}
                {
                    o.token_ = make_token_();
                }
                owner(owner&& o) noexcept
                    : token_{o.token_.load()}
                {}
                owner& operator=(const owner& o)
                {
                    o.token_ = make_token_();
                    token_   = make_token_();
                    return *this;
                }
                owner& operator=(owner&& o) noexcept
                {
                    token_ = o.token_.load();
                    return *this;
                }
            };

            struct ownee
            {
                edit token_{nullptr};

                ownee& operator=(edit e)
                {
                    assert(e != noone);
                    // This would be a nice safety plug but it sadly
                    // does not hold during transient concatenation.
                    // assert(token_ == e || token_ == edit{nullptr});
                    token_ = e;
                    return *this;
                }

                bool can_mutate(edit t) const { return token_ == t; }
                bool owned() const { return token_ != edit{nullptr}; }
            };

            static owner noone;
        };
    };
};

template <typename HP>
typename gc_transience_policy::apply<HP>::type::owner
    gc_transience_policy::apply<HP>::type::noone = {};

} // namespace immer
