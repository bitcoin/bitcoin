//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/detail/combine_standard_layout.hpp>
#include <immer/detail/type_traits.hpp>
#include <immer/detail/util.hpp>

#include <cstddef>
#include <limits>

namespace immer {
namespace detail {
namespace arrays {

template <typename T, typename MemoryPolicy>
struct node
{
    using memory     = MemoryPolicy;
    using heap       = typename MemoryPolicy::heap::type;
    using transience = typename memory::transience_t;
    using refs_t     = typename memory::refcount;
    using ownee_t    = typename transience::ownee;
    using node_t     = node;
    using edit_t     = typename transience::edit;

    struct data_t
    {
        aligned_storage_for<T> buffer;
    };

    using impl_t = combine_standard_layout_t<data_t, refs_t, ownee_t>;

    impl_t impl;

    constexpr static std::size_t sizeof_n(size_t count)
    {
        return std::max(immer_offsetof(impl_t, d.buffer) + sizeof(T) * count,
                        sizeof(node));
    }

    refs_t& refs() const { return auto_const_cast(get<refs_t>(impl)); }

    const ownee_t& ownee() const { return get<ownee_t>(impl); }
    ownee_t& ownee() { return get<ownee_t>(impl); }

    const T* data() const { return reinterpret_cast<const T*>(&impl.d.buffer); }
    T* data() { return reinterpret_cast<T*>(&impl.d.buffer); }

    bool can_mutate(edit_t e) const
    {
        return refs().unique() || ownee().can_mutate(e);
    }

    static void delete_n(node_t* p, size_t sz, size_t cap)
    {
        detail::destroy_n(p->data(), sz);
        heap::deallocate(sizeof_n(cap), p);
    }

    static node_t* make_n(size_t n)
    {
        return new (heap::allocate(sizeof_n(n))) node_t{};
    }

    static node_t* make_e(edit_t e, size_t n)
    {
        auto p     = make_n(n);
        p->ownee() = e;
        return p;
    }

    static node_t* fill_n(size_t n, T v)
    {
        auto p = make_n(n);
        IMMER_TRY {
            std::uninitialized_fill_n(p->data(), n, v);
            return p;
        }
        IMMER_CATCH (...) {
            heap::deallocate(sizeof_n(n), p);
            IMMER_RETHROW;
        }
    }

    template <typename Iter,
              typename Sent,
              std::enable_if_t<detail::compatible_sentinel_v<Iter, Sent>,
                               bool> = true>
    static node_t* copy_n(size_t n, Iter first, Sent last)
    {
        auto p = make_n(n);
        IMMER_TRY {
            detail::uninitialized_copy(first, last, p->data());
            return p;
        }
        IMMER_CATCH (...) {
            heap::deallocate(sizeof_n(n), p);
            IMMER_RETHROW;
        }
    }

    static node_t* copy_n(size_t n, node_t* p, size_t count)
    {
        return copy_n(n, p->data(), p->data() + count);
    }

    template <typename Iter>
    static node_t* copy_e(edit_t e, size_t n, Iter first, Iter last)
    {
        auto p     = copy_n(n, first, last);
        p->ownee() = e;
        return p;
    }

    static node_t* copy_e(edit_t e, size_t n, node_t* p, size_t count)
    {
        return copy_e(e, n, p->data(), p->data() + count);
    }
};

} // namespace arrays
} // namespace detail
} // namespace immer
