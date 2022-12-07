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
#include <immer/detail/hamts/bits.hpp>
#include <immer/detail/util.hpp>

#include <cassert>
#include <cstddef>

namespace immer {
namespace detail {
namespace hamts {

template <typename T,
          typename Hash,
          typename Equal,
          typename MemoryPolicy,
          bits_t B>
struct node
{
    using node_t = node;

    using memory      = MemoryPolicy;
    using heap_policy = typename memory::heap;
    using heap        = typename heap_policy::type;
    using transience  = typename memory::transience_t;
    using refs_t      = typename memory::refcount;
    using ownee_t     = typename transience::ownee;
    using edit_t      = typename transience::edit;
    using value_t     = T;
    using bitmap_t    = typename get_bitmap_type<B>::type;

    enum class kind_t
    {
        collision,
        inner
    };

    struct collision_t
    {
        count_t count;
        aligned_storage_for<T> buffer;
    };

    struct values_data_t
    {
        aligned_storage_for<T> buffer;
    };

    using values_t = combine_standard_layout_t<values_data_t, refs_t>;

    struct inner_t
    {
        bitmap_t nodemap;
        bitmap_t datamap;
        values_t* values;
        aligned_storage_for<node_t*> buffer;
    };

    union data_t
    {
        inner_t inner;
        collision_t collision;
    };

    struct impl_data_t
    {
#if IMMER_TAGGED_NODE
        kind_t kind;
#endif
        data_t data;
    };

    using impl_t = combine_standard_layout_t<impl_data_t, refs_t>;

    impl_t impl;

    constexpr static std::size_t sizeof_values_n(count_t count)
    {
        return std::max(sizeof(values_t),
                        immer_offsetof(values_t, d.buffer) +
                            sizeof(values_data_t::buffer) * count);
    }

    constexpr static std::size_t sizeof_collision_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.collision.buffer) +
               sizeof(collision_t::buffer) * count;
    }

    constexpr static std::size_t sizeof_inner_n(count_t count)
    {
        return immer_offsetof(impl_t, d.data.inner.buffer) +
               sizeof(inner_t::buffer) * count;
    }

#if IMMER_TAGGED_NODE
    kind_t kind() const { return impl.d.kind; }
#endif

    auto values()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        assert(impl.d.data.inner.values);
        return (T*) &impl.d.data.inner.values->d.buffer;
    }

    auto values() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        assert(impl.d.data.inner.values);
        return (const T*) &impl.d.data.inner.values->d.buffer;
    }

    auto children()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return (node_t**) &impl.d.data.inner.buffer;
    }

    auto children() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return (const node_t* const*) &impl.d.data.inner.buffer;
    }

    auto datamap() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.datamap;
    }

    auto nodemap() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return impl.d.data.inner.nodemap;
    }

    auto data_count() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return popcount(datamap());
    }

    auto data_count(bitmap_t bit) const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return popcount(static_cast<bitmap_t>(datamap() & (bit - 1)));
    }

    auto children_count() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return popcount(nodemap());
    }

    auto children_count(bitmap_t bit) const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::inner);
        return popcount(static_cast<bitmap_t>(nodemap() & (bit - 1)));
    }

    auto collision_count() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::collision);
        return impl.d.data.collision.count;
    }

    T* collisions()
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::collision);
        return (T*) &impl.d.data.collision.buffer;
    }

    const T* collisions() const
    {
        IMMER_ASSERT_TAGGED(kind() == kind_t::collision);
        return (const T*) &impl.d.data.collision.buffer;
    }

    static refs_t& refs(const values_t* x)
    {
        return auto_const_cast(get<refs_t>(*x));
    }
    static const ownee_t& ownee(const values_t* x) { return get<ownee_t>(*x); }
    static ownee_t& ownee(values_t* x) { return get<ownee_t>(*x); }

    static refs_t& refs(const node_t* x)
    {
        return auto_const_cast(get<refs_t>(x->impl));
    }
    static const ownee_t& ownee(const node_t* x)
    {
        return get<ownee_t>(x->impl);
    }
    static ownee_t& ownee(node_t* x) { return get<ownee_t>(x->impl); }

    static node_t* make_inner_n(count_t n)
    {
        assert(n <= branches<B>);
        auto m = heap::allocate(sizeof_inner_n(n));
        auto p = new (m) node_t;
        assert(p == (node_t*) m);
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::inner;
#endif
        p->impl.d.data.inner.nodemap = 0;
        p->impl.d.data.inner.datamap = 0;
        p->impl.d.data.inner.values  = nullptr;
        return p;
    }

    static node_t* make_inner_n(count_t n, values_t* values)
    {
        auto p = make_inner_n(n);
        if (values) {
            p->impl.d.data.inner.values = values;
            refs(values).inc();
        }
        return p;
    }

    static node_t* make_inner_n(count_t n, count_t nv)
    {
        assert(nv <= branches<B>);
        auto p = make_inner_n(n);
        if (nv) {
            IMMER_TRY {
                p->impl.d.data.inner.values =
                    new (heap::allocate(sizeof_values_n(nv))) values_t{};
            }
            IMMER_CATCH (...) {
                deallocate_inner(p, n);
                IMMER_RETHROW;
            }
        }
        return p;
    }

    static node_t* make_inner_n(count_t n, count_t idx, node_t* child)
    {
        assert(n >= 1);
        auto p                       = make_inner_n(n);
        p->impl.d.data.inner.nodemap = bitmap_t{1u} << idx;
        p->children()[0]             = child;
        return p;
    }

    static node_t* make_inner_n(count_t n, bitmap_t bitmap, T x)
    {
        auto p                       = make_inner_n(n, 1);
        p->impl.d.data.inner.datamap = bitmap;
        IMMER_TRY {
            new (p->values()) T{std::move(x)};
        }
        IMMER_CATCH (...) {
            deallocate_inner(p, n, 1);
            IMMER_RETHROW;
        }
        return p;
    }

    static node_t*
    make_inner_n(count_t n, count_t idx1, T x1, count_t idx2, T x2)
    {
        assert(idx1 != idx2);
        auto p = make_inner_n(n, 2);
        p->impl.d.data.inner.datamap =
            (bitmap_t{1u} << idx1) | (bitmap_t{1u} << idx2);
        auto assign = [&](auto&& x1, auto&& x2) {
            auto vp = p->values();
            IMMER_TRY {
                new (vp) T{std::move(x1)};
                IMMER_TRY {
                    new (vp + 1) T{std::move(x2)};
                }
                IMMER_CATCH (...) {
                    vp->~T();
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                deallocate_inner(p, n, 2);
                IMMER_RETHROW;
            }
        };
        if (idx1 < idx2)
            assign(x1, x2);
        else
            assign(x2, x1);
        return p;
    }

    static node_t* make_collision_n(count_t n)
    {
        auto m = heap::allocate(sizeof_collision_n(n));
        auto p = new (m) node_t;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::collision;
#endif
        p->impl.d.data.collision.count = n;
        return p;
    }

    static node_t* make_collision(T v1, T v2)
    {
        auto m = heap::allocate(sizeof_collision_n(2));
        auto p = new (m) node_t;
#if IMMER_TAGGED_NODE
        p->impl.d.kind = node_t::kind_t::collision;
#endif
        p->impl.d.data.collision.count = 2;
        auto cols                      = p->collisions();
        IMMER_TRY {
            new (cols) T{std::move(v1)};
            IMMER_TRY {
                new (cols + 1) T{std::move(v2)};
            }
            IMMER_CATCH (...) {
                cols->~T();
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_collision(p, 2);
            IMMER_RETHROW;
        }
        return p;
    }

    static node_t* copy_collision_insert(node_t* src, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::collision);
        auto n    = src->collision_count();
        auto dst  = make_collision_n(n + 1);
        auto srcp = src->collisions();
        auto dstp = dst->collisions();
        IMMER_TRY {
            new (dstp) T{std::move(v)};
            IMMER_TRY {
                std::uninitialized_copy(srcp, srcp + n, dstp + 1);
            }
            IMMER_CATCH (...) {
                dstp->~T();
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_collision(dst, n + 1);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_collision_remove(node_t* src, T* v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::collision);
        assert(src->collision_count() > 1);
        auto n    = src->collision_count();
        auto dst  = make_collision_n(n - 1);
        auto srcp = src->collisions();
        auto dstp = dst->collisions();
        IMMER_TRY {
            dstp = std::uninitialized_copy(srcp, v, dstp);
            IMMER_TRY {
                std::uninitialized_copy(v + 1, srcp + n, dstp);
            }
            IMMER_CATCH (...) {
                destroy(dst->collisions(), dstp);
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_collision(dst, n - 1);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t* copy_collision_replace(node_t* src, T* pos, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::collision);
        auto n    = src->collision_count();
        auto dst  = make_collision_n(n);
        auto srcp = src->collisions();
        auto dstp = dst->collisions();
        assert(pos >= srcp && pos < srcp + n);
        IMMER_TRY {
            new (dstp) T{std::move(v)};
            IMMER_TRY {
                dstp = std::uninitialized_copy(srcp, pos, dstp + 1);
                IMMER_TRY {
                    std::uninitialized_copy(pos + 1, srcp + n, dstp);
                }
                IMMER_CATCH (...) {
                    destroy(dst->collisions(), dstp);
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                dst->collisions()->~T();
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_collision(dst, n);
            IMMER_RETHROW;
        }
        return dst;
    }

    static node_t*
    copy_inner_replace(node_t* src, count_t offset, node_t* child)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n    = src->children_count();
        auto dst  = make_inner_n(n, src->impl.d.data.inner.values);
        auto srcp = src->children();
        auto dstp = dst->children();
        dst->impl.d.data.inner.datamap = src->datamap();
        dst->impl.d.data.inner.nodemap = src->nodemap();
        std::uninitialized_copy(srcp, srcp + n, dstp);
        inc_nodes(srcp, n);
        srcp[offset]->dec_unsafe();
        dstp[offset] = child;
        return dst;
    }

    static node_t* copy_inner_replace_value(node_t* src, count_t offset, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        assert(offset < src->data_count());
        auto n                         = src->children_count();
        auto nv                        = src->data_count();
        auto dst                       = make_inner_n(n, nv);
        dst->impl.d.data.inner.datamap = src->datamap();
        dst->impl.d.data.inner.nodemap = src->nodemap();
        IMMER_TRY {
            std::uninitialized_copy(
                src->values(), src->values() + nv, dst->values());
            IMMER_TRY {
                dst->values()[offset] = std::move(v);
            }
            IMMER_CATCH (...) {
                destroy_n(dst->values(), nv);
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst, n, nv);
            IMMER_RETHROW;
        }
        inc_nodes(src->children(), n);
        std::uninitialized_copy(
            src->children(), src->children() + n, dst->children());
        return dst;
    }

    static node_t* copy_inner_replace_merged(node_t* src,
                                             bitmap_t bit,
                                             count_t voffset,
                                             node_t* node)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        assert(!(src->nodemap() & bit));
        assert(src->datamap() & bit);
        assert(voffset == src->data_count(bit));
        auto n                         = src->children_count();
        auto nv                        = src->data_count();
        auto dst                       = make_inner_n(n + 1, nv - 1);
        auto noffset                   = src->children_count(bit);
        dst->impl.d.data.inner.datamap = src->datamap() & ~bit;
        dst->impl.d.data.inner.nodemap = src->nodemap() | bit;
        if (nv > 1) {
            IMMER_TRY {
                std::uninitialized_copy(
                    src->values(), src->values() + voffset, dst->values());
                IMMER_TRY {
                    std::uninitialized_copy(src->values() + voffset + 1,
                                            src->values() + nv,
                                            dst->values() + voffset);
                }
                IMMER_CATCH (...) {
                    destroy_n(dst->values(), voffset);
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                deallocate_inner(dst, n + 1, nv - 1);
                IMMER_RETHROW;
            }
        }
        inc_nodes(src->children(), n);
        std::uninitialized_copy(
            src->children(), src->children() + noffset, dst->children());
        std::uninitialized_copy(src->children() + noffset,
                                src->children() + n,
                                dst->children() + noffset + 1);
        dst->children()[noffset] = node;
        return dst;
    }

    static node_t* copy_inner_replace_inline(node_t* src,
                                             bitmap_t bit,
                                             count_t noffset,
                                             T value)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        assert(!(src->datamap() & bit));
        assert(src->nodemap() & bit);
        assert(noffset == src->children_count(bit));
        auto n                         = src->children_count();
        auto nv                        = src->data_count();
        auto dst                       = make_inner_n(n - 1, nv + 1);
        auto voffset                   = src->data_count(bit);
        dst->impl.d.data.inner.nodemap = src->nodemap() & ~bit;
        dst->impl.d.data.inner.datamap = src->datamap() | bit;
        IMMER_TRY {
            if (nv)
                std::uninitialized_copy(
                    src->values(), src->values() + voffset, dst->values());
            IMMER_TRY {
                new (dst->values() + voffset) T{std::move(value)};
                IMMER_TRY {
                    if (nv)
                        std::uninitialized_copy(src->values() + voffset,
                                                src->values() + nv,
                                                dst->values() + voffset + 1);
                }
                IMMER_CATCH (...) {
                    dst->values()[voffset].~T();
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                destroy_n(dst->values(), voffset);
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst, n - 1, nv + 1);
            IMMER_RETHROW;
        }
        inc_nodes(src->children(), n);
        src->children()[noffset]->dec_unsafe();
        std::uninitialized_copy(
            src->children(), src->children() + noffset, dst->children());
        std::uninitialized_copy(src->children() + noffset + 1,
                                src->children() + n,
                                dst->children() + noffset);
        return dst;
    }

    static node_t*
    copy_inner_remove_value(node_t* src, bitmap_t bit, count_t voffset)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        assert(!(src->nodemap() & bit));
        assert(src->datamap() & bit);
        assert(voffset == src->data_count(bit));
        auto n                         = src->children_count();
        auto nv                        = src->data_count();
        auto dst                       = make_inner_n(n, nv - 1);
        dst->impl.d.data.inner.datamap = src->datamap() & ~bit;
        dst->impl.d.data.inner.nodemap = src->nodemap();
        if (nv > 1) {
            IMMER_TRY {
                std::uninitialized_copy(
                    src->values(), src->values() + voffset, dst->values());
                IMMER_TRY {
                    std::uninitialized_copy(src->values() + voffset + 1,
                                            src->values() + nv,
                                            dst->values() + voffset);
                }
                IMMER_CATCH (...) {
                    destroy_n(dst->values(), voffset);
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                deallocate_inner(dst, n, nv - 1);
                IMMER_RETHROW;
            }
        }
        inc_nodes(src->children(), n);
        std::uninitialized_copy(
            src->children(), src->children() + n, dst->children());
        return dst;
    }

    static node_t* copy_inner_insert_value(node_t* src, bitmap_t bit, T v)
    {
        IMMER_ASSERT_TAGGED(src->kind() == kind_t::inner);
        auto n                         = src->children_count();
        auto nv                        = src->data_count();
        auto offset                    = src->data_count(bit);
        auto dst                       = make_inner_n(n, nv + 1);
        dst->impl.d.data.inner.datamap = src->datamap() | bit;
        dst->impl.d.data.inner.nodemap = src->nodemap();
        IMMER_TRY {
            if (nv)
                std::uninitialized_copy(
                    src->values(), src->values() + offset, dst->values());
            IMMER_TRY {
                new (dst->values() + offset) T{std::move(v)};
                IMMER_TRY {
                    if (nv)
                        std::uninitialized_copy(src->values() + offset,
                                                src->values() + nv,
                                                dst->values() + offset + 1);
                }
                IMMER_CATCH (...) {
                    dst->values()[offset].~T();
                    IMMER_RETHROW;
                }
            }
            IMMER_CATCH (...) {
                destroy_n(dst->values(), offset);
                IMMER_RETHROW;
            }
        }
        IMMER_CATCH (...) {
            deallocate_inner(dst, n, nv + 1);
            IMMER_RETHROW;
        }
        inc_nodes(src->children(), n);
        std::uninitialized_copy(
            src->children(), src->children() + n, dst->children());
        return dst;
    }

    static node_t*
    make_merged(shift_t shift, T v1, hash_t hash1, T v2, hash_t hash2)
    {
        if (shift < max_shift<B>) {
            auto idx1 = hash1 & (mask<B> << shift);
            auto idx2 = hash2 & (mask<B> << shift);
            if (idx1 == idx2) {
                auto merged = make_merged(
                    shift + B, std::move(v1), hash1, std::move(v2), hash2);
                IMMER_TRY {
                    return make_inner_n(1, idx1 >> shift, merged);
                }
                IMMER_CATCH (...) {
                    delete_deep_shift(merged, shift + B);
                    IMMER_RETHROW;
                }
            } else {
                return make_inner_n(0,
                                    idx1 >> shift,
                                    std::move(v1),
                                    idx2 >> shift,
                                    std::move(v2));
            }
        } else {
            return make_collision(std::move(v1), std::move(v2));
        }
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
    void dec_unsafe() const { refs(this).dec_unsafe(); }

    static void inc_nodes(node_t** p, count_t n)
    {
        for (auto i = p, e = i + n; i != e; ++i)
            refs(*i).inc();
    }

    static void delete_values(values_t* p, count_t n)
    {
        assert(p);
        deallocate_values(p, n);
    }

    static void delete_inner(node_t* p)
    {
        assert(p);
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::inner);
        auto vp = p->impl.d.data.inner.values;
        if (vp && refs(vp).dec())
            delete_values(vp, p->data_count());
        deallocate_inner(p, p->children_count());
    }

    static void delete_collision(node_t* p)
    {
        assert(p);
        IMMER_ASSERT_TAGGED(p->kind() == kind_t::collision);
        auto n = p->collision_count();
        deallocate_collision(p, n);
    }

    static void delete_deep(node_t* p, shift_t s)
    {
        if (s == max_depth<B>)
            delete_collision(p);
        else {
            auto fst = p->children();
            auto lst = fst + p->children_count();
            for (; fst != lst; ++fst)
                if ((*fst)->dec())
                    delete_deep(*fst, s + 1);
            delete_inner(p);
        }
    }

    static void delete_deep_shift(node_t* p, shift_t s)
    {
        if (s == max_shift<B>)
            delete_collision(p);
        else {
            auto fst = p->children();
            auto lst = fst + p->children_count();
            for (; fst != lst; ++fst)
                if ((*fst)->dec())
                    delete_deep_shift(*fst, s + B);
            delete_inner(p);
        }
    }

    static void deallocate_values(values_t* p, count_t n)
    {
        destroy_n((T*) &p->d.buffer, n);
        heap::deallocate(node_t::sizeof_values_n(n), p);
    }

    static void deallocate_collision(node_t* p, count_t n)
    {
        destroy_n(p->collisions(), n);
        heap::deallocate(node_t::sizeof_collision_n(n), p);
    }

    static void deallocate_inner(node_t* p, count_t n)
    {
        heap::deallocate(node_t::sizeof_inner_n(n), p);
    }

    static void deallocate_inner(node_t* p, count_t n, count_t nv)
    {
        assert(nv);
        heap::deallocate(node_t::sizeof_values_n(nv),
                         p->impl.d.data.inner.values);
        heap::deallocate(node_t::sizeof_inner_n(n), p);
    }
};

} // namespace hamts
} // namespace detail
} // namespace immer
