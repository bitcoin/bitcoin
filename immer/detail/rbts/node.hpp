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
#include <immer/detail/rbts/bits.hpp>
#include <immer/detail/util.hpp>
#include <immer/heap/tags.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace immer {
namespace detail {
namespace rbts {

template <typename T, typename MemoryPolicy, bits_t B, bits_t BL>
struct node
{
    static constexpr auto bits      = B;
    static constexpr auto bits_leaf = BL;

    using node_t      = node;
    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;

    static constexpr bool embed_relaxed = memory::prefer_fewer_bigger_objects;

    enum class kind_t
    {
        leaf,
        inner
    };

    struct relaxed_data_t
    {
        count_t count;
        size_t sizes[branches<B>];
    };

    using relaxed_data_with_meta_t =
        combine_standard_layout_t<relaxed_data_t, refs_t, ownee_t>;

    using relaxed_data_no_meta_t = combine_standard_layout_t<relaxed_data_t>;

    using relaxed_t = std::conditional_t<embed_relaxed,
                                         relaxed_data_no_meta_t,
                                         relaxed_data_with_meta_t>;

    struct leaf_t
    {
        aligned_storage_for<T> buffer;
    };

    struct inner_t
    {
        relaxed_t* relaxed;
        aligned_storage_for<node_t*> buffer;
    };

    union data_t
    {
        inner_t inner;
        leaf_t leaf;
    };

    struct impl_data_t
    {
#if IMMER_TAGGED_NODE
        kind_t kind;
#endif
        data_t data;
    };

    using impl_t = combine_standard_layout_t<impl_data_t, refs_t, ownee_t>;

    impl_t impl;

    // assume that we need to keep headroom space in the node when we
    // are doing reference counting, since any node may become
    // transient when it has only one reference
    constexpr static bool keep_headroom = !std::is_empty<refs_t>{};

    constexpr static std::size_t sizeof_packed_leaf_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.leaf.buffer) +
               sizeof(leaf_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_packed_inner_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.inner.buffer) +
               sizeof(inner_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_packed_relaxed_n(count_t count)
    {
        return immer_offsetof(relaxed_t, d.sizes) + sizeof(size_t) * count;
    }

    constexpr static std::size_t sizeof_packed_inner_r_n(count_t count)
    {
        return embed_relaxed ? sizeof_packed_inner_n(count) +
                                   sizeof_packed_relaxed_n(count)
                             : sizeof_packed_inner_n(count);
    }

    constexpr static std::size_t max_sizeof_leaf =
        sizeof_packed_leaf_n(branches<BL>);

    constexpr static std::size_t max_sizeof_inner =
        sizeof_packed_inner_n(branches<B>);

    constexpr static std::size_t max_sizeof_relaxed =
        sizeof_packed_relaxed_n(branches<B>);

    constexpr static std::size_t max_sizeof_inner_r =
        sizeof_packed_inner_r_n(branches<B>);

    constexpr static std::size_t sizeof_inner_n(count_t n)
    {
        return keep_headroom ? max_sizeof_inner : sizeof_packed_inner_n(n);
    }

    constexpr static std::size_t sizeof_inner_r_n(count_t n)
    {
        return keep_headroom ? max_sizeof_inner_r : sizeof_packed_inner_r_n(n);
    }

    constexpr static std::size_t sizeof_relaxed_n(count_t n)
    {
        return keep_headroom ? max_sizeof_relaxed : sizeof_packed_relaxed_n(n);
    }

    constexpr static std::size_t sizeof_leaf_n(count_t n)
    {
        return keep_headroom ? max_sizeof_leaf : sizeof_packed_leaf_n(n);
    }

    using heap =
        typename heap_policy::template optimized<max_sizeof_inner>::type;

#if IMMER_TAGGED_NODE
    kind_t kind() const { return impl.d.kind; }
#endif

    relaxed_t* relaxed()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.relaxed;
    }

    const relaxed_t* relaxed() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.relaxed;
    }

    node_t** inner()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return reinterpret_cast<node_t**>(&impl.d.data.inner.buffer);
    }

    T* leaf()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::leaf);
        return reinterpret_cast<T*>(&impl.d.data.leaf.buffer);
    }

    static refs_t& refs(const relaxed_t* x)
    {
        return auto_const_cast(get<refs_t>(*x));
    }
    static const ownee_t& ownee(const relaxed_t* x) { return get<ownee_t>(*x); }
    static ownee_t& ownee(relaxed_t* x) { return get<ownee_t>(*x); }

    static refs_t& refs(const node_t* x)
    {
        return auto_const_cast(get<refs_t>(x->impl));
    }
    static const ownee_t& ownee(const node_t* x)
    {
        return get<ownee_t>(x->impl);
    }
    static ownee_t& ownee(node_t* x) { return get<ownee_t>(x->impl); }

    static node_t* make_inner_n_into(void* buffer, std::size_t size, count_t n)
    {
        assert(n <= branches<B>);
        assert(size >= sizeof_inner_n(n));
        auto p                       = new (buffer) node_t;
        p->impl.d.data.inner.relaxed = nullptr;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        return p;
    }
    static node_t* make_inner_n(count_t n)
    {
        assert(n <= branches<B>);
        auto m = heap::allocate(sizeof_inner_n(n));
        return make_inner_n_into(m, sizeof_inner_n(n), n);
    }

    static node_t* make_inner_e(edit_t e)
    {
        auto m                       = heap::allocate(max_sizeof_inner);
        auto p                       = new (m) node_t;
        ownee(p)                     = e;
        p->impl.d.data.inner.relaxed = nullptr;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_r_n(count_t n)
    {
        assert(n <= branches<B>);
        auto mp = heap::allocate(sizeof_inner_r_n(n));
        auto mr = static_cast<void*>(nullptr);
        if (embed_relaxed) {
            mr = reinterpret_cast<unsigned char*>(mp) + sizeof_inner_n(n);
        } else {
            IMMER_TRY {
                mr = heap::allocate(sizeof_relaxed_n(n), norefs_tag{});
            }
            IMMER_CATCH (...) {
                heap::deallocate(sizeof_inner_r_n(n), mp);
                IMMER_RETHROW;
            }
        }
        auto p                       = new (mp) node_t;
        auto r                       = new (mr) relaxed_t;
        r->d.count                   = 0;
        p->impl.d.data.inner.relaxed = r;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_sr_n(count_t n, relaxed_t* r)
    {
        return static_if<embed_relaxed, node_t*>(
            [&](auto) { return node_t::make_inner_r_n(n); },
            [&](auto) {
                auto p =
                    new (heap::allocate(node_t::sizeof_inner_r_n(n))) node_t;
                assert(r->d.count >= n);
                node_t::refs(r).inc();
                p->impl.d.data.inner.relaxed = r;
#if IMMER_TAGGED_NODE
                p->impl.d.kind = node_t::kind_t::inner;
#endif
                return p;
            });
    }

    static node_t* make_inner_r_e(edit_t e)
    {
        auto mp = heap::allocate(max_sizeof_inner_r);
        auto mr = static_cast<void*>(nullptr);
        if (embed_relaxed) {
            mr = reinterpret_cast<unsigned char*>(mp) + max_sizeof_inner;
        } else {
            IMMER_TRY {
                mr = heap::allocate(max_sizeof_relaxed, norefs_tag{});
            }
            IMMER_CATCH (...) {
                heap::deallocate(max_sizeof_inner_r, mp);
                IMMER_RETHROW;
            }
        }
        auto p   = new (mp) node_t;
        auto r   = new (mr) relaxed_t;
        ownee(p) = e;
        static_if<!embed_relaxed>([&](auto) { node_t::ownee(r) = e; });
        r->d.count                   = 0;
        p->impl.d.data.inner.relaxed = r;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        return p;
    }

    static node_t* make_inner_sr_e(edit_t e, relaxed_t* r)
    {
        return static_if<embed_relaxed, node_t*>(
            [&](auto) { return node_t::make_inner_r_e(e); },
            [&](auto) {
                auto p =
                    new (heap::allocate(node_t::max_sizeof_inner_r)) node_t;
                node_t::refs(r).inc();
                p->impl.d.data.inner.relaxed = r;
                node_t::ownee(p)             = e;
#if IMMER_TAGGED_NODE
                p->impl.d.kind = node_t::kind_t::inner;
#endif
                return p;
            });
    }

    static node_t* make_leaf_n_into(void* buffer, std::size_t size, count_t n)
    {
        assert(n <= branches<BL>);
        assert(size >= sizeof_leaf_n(n));
        auto p = new (buffer) node_t;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::leaf;
#endif
        return p;
    }

    static node_t* make_leaf_n(count_t n)
    {
        assert(n <= branches<BL>);
        auto m = heap::allocate(sizeof_leaf_n(n));
        return make_leaf_n_into(m, sizeof_leaf_n(n), n);
    }

    static node_t* make_leaf_e(edit_t e)
    {
        auto p   = new (heap::allocate(max_sizeof_leaf)) node_t;
        ownee(p) = e;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::leaf;
#endif
        return p;
    }

    static node_t* make_inner_n(count_t n, node_t* x)
    {
        assert(n >= 1);
        auto p        = make_inner_n(n);
        p->inner()[0] = x;
        return p;
    }

    static node_t* make_inner_n(edit_t n, node_t* x)
    {
        assert(n >= 1);
        auto p        = make_inner_n(n);
        p->inner()[0] = x;
        return p;
    }

    static node_t* make_inner_n(count_t n, node_t* x, node_t* y)
    {
        assert(n >= 2);
        auto p        = make_inner_n(n);
        p->inner()[0] = x;
        p->inner()[1] = y;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x)
    {
        assert(n >= 1);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        r->d.count    = 1;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x, size_t xs)
    {
        assert(n >= 1);
        assert(xs);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        r->d.sizes[0] = xs;
        r->d.count    = 1;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x, node_t* y)
    {
        assert(n >= 2);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        p->inner()[1] = y;
        r->d.count    = 2;
        return p;
    }

    static node_t* make_inner_r_n(count_t n, node_t* x, size_t xs, node_t* y)
    {
        assert(n >= 2);
        assert(xs);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        p->inner()[1] = y;
        r->d.sizes[0] = xs;
        r->d.count    = 2;
        return p;
    }

    static node_t*
    make_inner_r_n(count_t n, node_t* x, size_t xs, node_t* y, size_t ys)
    {
        assert(n >= 2);
        assert(xs);
        assert(ys);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        p->inner()[1] = y;
        r->d.sizes[0] = xs;
        r->d.sizes[1] = xs + ys;
        r->d.count    = 2;
        return p;
    }

    static node_t* make_inner_r_n(count_t n,
                                  node_t* x,
                                  size_t xs,
                                  node_t* y,
                                  size_t ys,
                                  node_t* z,
                                  size_t zs)
    {
        assert(n >= 3);
        assert(xs);
        assert(ys);
        assert(zs);
        auto p        = make_inner_r_n(n);
        auto r        = p->relaxed();
        p->inner()[0] = x;
        p->inner()[1] = y;
        p->inner()[2] = z;
        r->d.sizes[0] = xs;
        r->d.sizes[1] = xs + ys;
        r->d.sizes[2] = xs + ys + zs;
        r->d.count    = 3;
        return p;
    }

    template <typename U>
    static node_t* make_leaf_n(count_t n, U&& x)
    {
        assert(n >= 1);
        auto p = make_leaf_n(n);
        IMMER_TRY {
            new (p->leaf()) T{std::forward<U>(x)};
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::sizeof_leaf_n(n), p);
            IMMER_RETHROW;
        }
        return p;
    }

    template <typename U>
    static node_t* make_leaf_e(edit_t e, U&& x)
    {
        auto p = make_leaf_e(e);
        IMMER_TRY {
            new (p->leaf()) T{std::forward<U>(x)};
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::max_sizeof_leaf, p);
            IMMER_RETHROW;
        }
        return p;
    }

    static node_t* make_path(shift_t shift, node_t* node)
    {
        IMMER_ASSERT_TAGGED(node->kind() == kind_t::leaf);
        if (shift == endshift<B, BL>)
            return node;
        else {
            auto n = node_t::make_inner_n(1);
            IMMER_TRY {
                n->inner()[0] = make_path(shift - B, node);
            }
            IMMER_CATCH (...) {
                heap::deallocate(node_t::sizeof_inner_n(1), n);
                IMMER_RETHROW;
            }
            return n;
        }
    }

    static node_t* make_path_e(edit_t e, shift_t shift, node_t* node)
    {
        IMMER_ASSERT_TAGGED(node->kind() == kind_t::leaf);
        if (shift == endshift<B, BL>)
            return node;
        else {
            auto n = node_t::make_inner_e(e);
            IMMER_TRY {
                n->inner()[0] = make_path_e(e, shift - B, node);
            }
            IMMER_CATCH (...) {
                heap::deallocate(node_t::max_sizeof_inner, n);
                IMMER_RETHROW;
            }
            return n;
        }
    }

    static node_t* copy_inner(node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_n(n);
        inc_nodes(src->inner(), n);
        std::copy(src->inner(), src->inner() + n, dst->inner());
        return dst;
    }

    static node_t* copy_inner_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_n(allocn);
        return do_copy_inner(dst, src, n);
    }

    static node_t* copy_inner_e(edit_t e, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_e(e);
        return do_copy_inner(dst, src, n);
    }

    static node_t* do_copy_inner(node_t* dst, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(dst->kind() == kind_t::inner);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto p = src->inner();
        inc_nodes(p, n);
        std::copy(p, p + n, dst->inner());
        return dst;
    }

    static node_t* do_copy_inner_replace(
        node_t* dst, node_t* src, count_t n, count_t offset, node_t* child)
    {
        IMMER_ASSERT_TAGGED(dst->kind() == kind_t::inner);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto p = src->inner();
        inc_nodes(p, offset);
        inc_nodes(p + offset + 1, n - offset - 1);
        std::copy(p, p + n, dst->inner());
        dst->inner()[offset] = child;
        return dst;
    }

    static node_t* copy_inner_r(node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(n);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* copy_inner_r_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_r_n(allocn);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* copy_inner_r_e(edit_t e, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_r_e(e);
        return do_copy_inner_r(dst, src, n);
    }

    static node_t* copy_inner_sr_e(edit_t e, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto dst = make_inner_sr_e(e, src->relaxed());
        return do_copy_inner_sr(dst, src, n);
    }

    static node_t* do_copy_inner_r(node_t* dst, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(dst->kind() == kind_t::inner);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        inc_nodes(src->inner(), n);
        std::copy(src->inner(), src->inner() + n, dst->inner());
        std::copy(src_r->d.sizes, src_r->d.sizes + n, dst_r->d.sizes);
        dst_r->d.count = n;
        return dst;
    }

    static node_t* do_copy_inner_replace_r(
        node_t* dst, node_t* src, count_t n, count_t offset, node_t* child)
    {
        IMMER_ASSERT_TAGGED(dst->kind() == kind_t::inner);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto src_r = src->relaxed();
        auto dst_r = dst->relaxed();
        auto p     = src->inner();
        inc_nodes(p, offset);
        inc_nodes(p + offset + 1, n - offset - 1);
        std::copy(src->inner(), src->inner() + n, dst->inner());
        std::copy(src_r->d.sizes, src_r->d.sizes + n, dst_r->d.sizes);
        dst_r->d.count       = n;
        dst->inner()[offset] = child;
        return dst;
    }

    static node_t* do_copy_inner_sr(node_t* dst, node_t* src, count_t n)
    {
        if (embed_relaxed)
            return do_copy_inner_r(dst, src, n);
        else {
            inc_nodes(src->inner(), n);
            std::copy(src->inner(), src->inner() + n, dst->inner());
            return dst;
        }
    }

    static node_t* do_copy_inner_replace_sr(
        node_t* dst, node_t* src, count_t n, count_t offset, node_t* child)
    {
        if (embed_relaxed)
            return do_copy_inner_replace_r(dst, src, n, offset, child);
        else {
            auto p = src->inner();
            inc_nodes(p, offset);
            inc_nodes(p + offset + 1, n - offset - 1);
            std::copy(p, p + n, dst->inner());
            dst->inner()[offset] = child;
            return dst;
        }
    }

    static node_t* copy_leaf(node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->leaf(), src->leaf() + n, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::sizeof_leaf_n(n), dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf_e(edit_t e, node_t* src, count_t n)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto dst = make_leaf_e(e);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->leaf(), src->leaf() + n, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::max_sizeof_leaf, dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf_n(count_t allocn, node_t* src, count_t n)
    {
        assert(allocn >= n);
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(allocn);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->leaf(), src->leaf() + n, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::sizeof_leaf_n(allocn), dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf(node_t* src1, count_t n1, node_t* src2, count_t n2)
    {
        IMMER_ASSERT_TAGGED(src1->kind() == kind_t::leaf);
        IMMER_ASSERT_TAGGED(src2->kind() == kind_t::leaf);
        auto dst = make_leaf_n(n1 + n2);
        IMMER_TRY {
            detail::uninitialized_copy(
                src1->leaf(), src1->leaf() + n1, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::sizeof_leaf_n(n1 + n2), dst);
            IMMER_RETHROW;
        }
        IMMER_TRY {
            detail::uninitialized_copy(
                src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        }
        IMMER_CATCH (...) {
            detail::destroy_n(dst->leaf(), n1);
            heap::deallocate(node_t::sizeof_leaf_n(n1 + n2), dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t*
    copy_leaf_e(edit_t e, node_t* src1, count_t n1, node_t* src2, count_t n2)
    {
        IMMER_ASSERT_TAGGED(src1->kind() == kind_t::leaf);
        IMMER_ASSERT_TAGGED(src2->kind() == kind_t::leaf);
        auto dst = make_leaf_e(e);
        IMMER_TRY {
            detail::uninitialized_copy(
                src1->leaf(), src1->leaf() + n1, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(max_sizeof_leaf, dst);
            IMMER_RETHROW;
        }
        IMMER_TRY {
            detail::uninitialized_copy(
                src2->leaf(), src2->leaf() + n2, dst->leaf() + n1);
        }
        IMMER_CATCH (...) {
            detail::destroy_n(dst->leaf(), n1);
            heap::deallocate(max_sizeof_leaf, dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf_e(edit_t e, node_t* src, count_t idx, count_t last)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto dst = make_leaf_e(e);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->leaf() + idx, src->leaf() + last, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(max_sizeof_leaf, dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_leaf(node_t* src, count_t idx, count_t last)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::leaf);
        auto dst = make_leaf_n(last - idx);
        IMMER_TRY {
            detail::uninitialized_copy(
                src->leaf() + idx, src->leaf() + last, dst->leaf());
        }
        IMMER_CATCH (...) {
            heap::deallocate(node_t::sizeof_leaf_n(last - idx), dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    template <typename U>
    static node_t* copy_leaf_emplace(node_t* src, count_t n, U&& x)
    {
        auto dst = copy_leaf_n(n + 1, src, n);
        IMMER_TRY {
            new (dst->leaf() + n) T{std::forward<U>(x)};
        }
        IMMER_CATCH (...) {
            detail::destroy_n(dst->leaf(), n);
            heap::deallocate(node_t::sizeof_leaf_n(n + 1), dst);
            IMMER_RETHROW;
        }
        return dst;
    }

    static void delete_inner(node_t* p, count_t n)
    {
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        assert(!p->relaxed());
        heap::deallocate(ownee(p).owned() ? node_t::max_sizeof_inner
                                          : node_t::sizeof_inner_n(n),
                         p);
    }

    static void delete_inner_e(node_t* p)
    {
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        assert(!p->relaxed());
        heap::deallocate(node_t::max_sizeof_inner, p);
    }

    static void delete_inner_any(node_t* p, count_t n)
    {
        if (p->relaxed())
            delete_inner_r(p, n);
        else
            delete_inner(p, n);
    }

    static void delete_inner_r(node_t* p, count_t n)
    {
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        auto r = p->relaxed();
        assert(r);
        static_if<!embed_relaxed>([&](auto) {
            if (node_t::refs(r).dec())
                heap::deallocate(node_t::ownee(r).owned()
                                     ? node_t::max_sizeof_relaxed
                                     : node_t::sizeof_relaxed_n(n),
                                 r);
        });
        heap::deallocate(ownee(p).owned() ? node_t::max_sizeof_inner_r
                                          : node_t::sizeof_inner_r_n(n),
                         p);
    }

    static void delete_inner_r_e(node_t* p)
    {
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        auto r = p->relaxed();
        assert(r);
        static_if<!embed_relaxed>([&](auto) {
            if (node_t::refs(r).dec())
                heap::deallocate(node_t::max_sizeof_relaxed, r);
        });
        heap::deallocate(node_t::max_sizeof_inner_r, p);
    }

    static void delete_leaf(node_t* p, count_t n)
    {
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::leaf);
        detail::destroy_n(p->leaf(), n);
        heap::deallocate(ownee(p).owned() ? node_t::max_sizeof_leaf
                                          : node_t::sizeof_leaf_n(n),
                         p);
    }

    bool can_mutate(edit_t e) const
    {
        return refs(this).unique() || ownee(this).can_mutate(e);
    }

    bool can_relax() const { return !embed_relaxed || relaxed(); }

    relaxed_t* ensure_mutable_relaxed(edit_t e)
    {
        auto src_r = relaxed();
        return static_if<embed_relaxed, relaxed_t*>(
            [&](auto) { return src_r; },
            [&](auto) {
                if (node_t::refs(src_r).unique() ||
                    node_t::ownee(src_r).can_mutate(e))
                    return src_r;
                else {
                    auto dst_r = impl.d.data.inner.relaxed =
                        new (heap::allocate(max_sizeof_relaxed)) relaxed_t;
                    if (src_r) {
                        auto n = dst_r->d.count = src_r->d.count;
                        std::copy(
                            src_r->d.sizes, src_r->d.sizes + n, dst_r->d.sizes);
                        if (node_t::refs(src_r).dec())
                            heap::deallocate(node_t::sizeof_inner_r_n(n),
                                             src_r);
                    }
                    node_t::ownee(dst_r) = e;
                    return dst_r;
                }
            });
    }

    relaxed_t* ensure_mutable_relaxed_e(edit_t e, edit_t ec)
    {
        auto src_r = relaxed();
        return static_if<embed_relaxed, relaxed_t*>(
            [&](auto) { return src_r; },
            [&](auto) {
                if (src_r && (node_t::refs(src_r).unique() ||
                              node_t::ownee(src_r).can_mutate(e))) {
                    node_t::ownee(src_r) = ec;
                    return src_r;
                } else {
                    auto dst_r = impl.d.data.inner.relaxed =
                        new (heap::allocate(max_sizeof_relaxed)) relaxed_t;
                    if (src_r) {
                        auto n = dst_r->d.count = src_r->d.count;
                        std::copy(
                            src_r->d.sizes, src_r->d.sizes + n, dst_r->d.sizes);
                        if (node_t::refs(src_r).dec())
                            heap::deallocate(node_t::sizeof_inner_r_n(n),
                                             src_r);
                    }
                    node_t::ownee(dst_r) = ec;
                    return dst_r;
                }
            });
    }

    relaxed_t* ensure_mutable_relaxed_n(edit_t e, count_t n)
    {
        auto src_r = relaxed();
        return static_if<embed_relaxed, relaxed_t*>(
            [&](auto) { return src_r; },
            [&](auto) {
                if (node_t::refs(src_r).unique() ||
                    node_t::ownee(src_r).can_mutate(e))
                    return src_r;
                else {
                    auto dst_r =
                        new (heap::allocate(max_sizeof_relaxed)) relaxed_t;
                    if (src_r) {
                        std::copy(
                            src_r->d.sizes, src_r->d.sizes + n, dst_r->d.sizes);
                        if (node_t::refs(src_r).dec())
                            heap::deallocate(node_t::sizeof_inner_r_n(n),
                                             src_r);
                    }
                    dst_r->d.count                   = n;
                    node_t::ownee(dst_r)             = e;
                    return impl.d.data.inner.relaxed = dst_r;
                }
            });
    }

    node_t* inc()
    {
        refs(this).inc();
        return this;
    }

    const node_t* inc() const
    {
        refs(this).inc();
        return this;
    }

    bool dec() const { return refs(this).dec(); }

    static void inc_nodes(node_t** p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refs(*i).inc();
    }

#if IMMER_TAGGED_NODE
    shift_t compute_shift()
    {
        if (kind() == kind_t::leaf)
            return endshift<B, BL>;
        else
            return B + inner()[0]->compute_shift();
    }
#endif

    bool check(shift_t shift, size_t size)
    {
#if IMMER_DEBUG_DEEP_CHECK
        assert(size > 0);
        if (shift == endshift<B, BL>) {
            IMMER_ASSERT_TAGGED(kind() == kind_t::leaf);
            assert(size <= branches<BL>);
        } else if (auto r = relaxed()) {
            auto count = r->d.count;
            assert(count > 0);
            assert(count <= branches<B>);
            if (r->d.sizes[count - 1] != size) {
                IMMER_TRACE_F("check");
                IMMER_TRACE_E(r->d.sizes[count - 1]);
                IMMER_TRACE_E(size);
            }
            assert(r->d.sizes[count - 1] == size);
            for (auto i = 1u; i < count; ++i)
                assert(r->d.sizes[i - 1] < r->d.sizes[i]);
            auto last_size = size_t{};
            for (auto i = 0u; i < count; ++i) {
                assert(inner()[i]->check(shift - B, r->d.sizes[i] - last_size));
                last_size = r->d.sizes[i];
            }
        } else {
            assert(size <= branches<B> << shift);
            auto count =
                (size >> shift) + (size - ((size >> shift) << shift) > 0);
            assert(count <= branches<B>);
            if (count) {
                for (auto i = 1u; i < count - 1; ++i)
                    assert(inner()[i]->check(shift - B, 1 << shift));
                assert(inner()[count - 1]->check(
                    shift - B, size - ((count - 1) << shift)));
            }
        }
#endif // IMMER_DEBUG_DEEP_CHECK
        return true;
    }
};

template <typename T, typename MP, bits_t B>
constexpr bits_t derive_bits_leaf_aux()
{
    using node_t               = node<T, MP, B, B>;
    constexpr auto sizeof_elem = sizeof(T);
    constexpr auto space =
        node_t::max_sizeof_inner - node_t::sizeof_packed_leaf_n(0);
    constexpr auto full_elems = space / sizeof_elem;
    constexpr auto BL         = log2(full_elems);
    return BL;
}

template <typename T, typename MP, bits_t B>
constexpr bits_t derive_bits_leaf = derive_bits_leaf_aux<T, MP, B>();

} // namespace rbts
} // namespace detail
} // namespace immer
