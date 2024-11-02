//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/box.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <type_traits>

namespace immer {

namespace detail {

template <typename T, typename MemoryPolicy>
struct refcount_atom_impl
{
    using box_type      = box<T, MemoryPolicy>;
    using value_type    = T;
    using memory_policy = MemoryPolicy;
    using lock_t        = typename MemoryPolicy::lock;
    using scoped_lock_t = typename lock_t::scoped_lock;

    refcount_atom_impl(const refcount_atom_impl&) = delete;
    refcount_atom_impl(refcount_atom_impl&&)      = delete;
    refcount_atom_impl& operator=(const refcount_atom_impl&) = delete;
    refcount_atom_impl& operator=(refcount_atom_impl&&) = delete;

    refcount_atom_impl(box_type b)
        : impl_{std::move(b)}
    {}

    box_type load() const
    {
        scoped_lock_t lock{lock_};
        return impl_;
    }

    void store(box_type b)
    {
        scoped_lock_t lock{lock_};
        impl_ = std::move(b);
    }

    box_type exchange(box_type b)
    {
        {
            scoped_lock_t lock{lock_};
            swap(b, impl_);
        }
        return b;
    }

    template <typename Fn>
    box_type update(Fn&& fn)
    {
        while (true) {
            auto oldv = load();
            auto newv = oldv.update(fn);
            {
                scoped_lock_t lock{lock_};
                if (oldv.impl_ == impl_.impl_) {
                    impl_ = newv;
                    return {newv};
                }
            }
        }
    }

private:
    mutable lock_t lock_;
    box_type impl_;
};

template <typename T, typename MemoryPolicy>
struct gc_atom_impl
{
    using box_type      = box<T, MemoryPolicy>;
    using value_type    = T;
    using memory_policy = MemoryPolicy;

    static_assert(std::is_same<typename MemoryPolicy::refcount,
                               no_refcount_policy>::value,
                  "gc_atom_impl can only be used when there is no refcount!");

    gc_atom_impl(const gc_atom_impl&) = delete;
    gc_atom_impl(gc_atom_impl&&)      = delete;
    gc_atom_impl& operator=(const gc_atom_impl&) = delete;
    gc_atom_impl& operator=(gc_atom_impl&&) = delete;

    gc_atom_impl(box_type b)
        : impl_{b.impl_}
    {}

    box_type load() const { return {impl_.load()}; }

    void store(box_type b) { impl_.store(b.impl_); }

    box_type exchange(box_type b) { return {impl_.exchange(b.impl_)}; }

    template <typename Fn>
    box_type update(Fn&& fn)
    {
        while (true) {
            auto oldv = box_type{impl_.load()};
            auto newv = oldv.update(fn);
            if (impl_.compare_exchange_weak(oldv.impl_, newv.impl_))
                return {newv};
        }
    }

private:
    std::atomic<typename box_type::holder*> impl_;
};

} // namespace detail

/*!
 * Stores for boxed values of type `T` in a thread-safe manner.
 *
 * @see box
 *
 * @rst
 *
 * .. warning:: If memory policy used includes thread unsafe reference counting,
 *    no thread safety is assumed, and the atom becomes thread unsafe too!
 *
 * .. note:: ``box<T>`` provides a value based box of type ``T``, this is, we
 *    can think about it as a value-based version of ``std::shared_ptr``.  In a
 *    similar fashion, ``atom<T>`` is in spirit the value-based equivalent of
 *    C++20 ``std::atomic_shared_ptr``.  However, the API does not follow
 *    ``std::atomic`` interface closely, since it attempts to be a higher level
 *    construction, most similar to Clojure's ``(atom)``.  It is remarkable in
 *    particular that, since ``box<T>`` underlying object is immutable, using
 *    ``atom<T>`` is fully thread-safe in ways that ``std::atomic_shared_ptr`` is
 *    not. This is so because dereferencing the underlying pointer in a
 *    ``std::atomic_share_ptr`` may require further synchronization, in
 *    particular when invoking non-const methods.
 *
 * @endrst
 */
template <typename T, typename MemoryPolicy = default_memory_policy>
class atom
{
public:
    using box_type      = box<T, MemoryPolicy>;
    using value_type    = T;
    using memory_policy = MemoryPolicy;

    atom(const atom&) = delete;
    atom(atom&&)      = delete;
    void operator=(const atom&) = delete;
    void operator=(atom&&) = delete;

    /*!
     * Constructs an atom holding a value `b`;
     */
    atom(box_type v = {})
        : impl_{std::move(v)}
    {}

    /*!
     * Sets a new value in the atom.
     */
    atom& operator=(box_type b)
    {
        impl_.store(std::move(b));
        return *this;
    }

    /*!
     * Reads the currently stored value in a thread-safe manner.
     */
    operator box_type() const { return impl_.load(); }

    /*!
     * Reads the currently stored value in a thread-safe manner.
     */
    operator value_type() const { return *impl_.load(); }

    /*!
     * Reads the currently stored value in a thread-safe manner.
     */
    IMMER_NODISCARD box_type load() const { return impl_.load(); }

    /*!
     * Stores a new value in a thread-safe manner.
     */
    void store(box_type b) { impl_.store(std::move(b)); }

    /*!
     * Stores a new value and returns the old value, in a thread-safe manner.
     */
    IMMER_NODISCARD box_type exchange(box_type b)
    {
        return impl_.exchange(std::move(b));
    }

    /*!
     * Stores the result of applying `fn` to the current value atomically and
     * returns the new resulting value.
     *
     * @rst
     *
     * .. warning:: ``fn`` must be a pure function and have no side effects! The
     *    function might be evaluated multiple times when multiple threads
     *    content to update the value.
     *
     * @endrst
     */
    template <typename Fn>
    box_type update(Fn&& fn)
    {
        return impl_.update(std::forward<Fn>(fn));
    }

private:
    struct get_refcount_atom_impl
    {
        template <typename U, typename MP>
        struct apply
        {
            using type = detail::refcount_atom_impl<U, MP>;
        };
    };

    struct get_gc_atom_impl
    {
        template <typename U, typename MP>
        struct apply
        {
            using type = detail::gc_atom_impl<U, MP>;
        };
    };

    // If we are using "real" garbage collection (we assume this when we use
    // `no_refcount_policy`), we just store the pointer in an atomic.  If we use
    // reference counting, we rely on the reference counting spinlock.
    using impl_t = typename std::conditional_t<
        std::is_same<typename MemoryPolicy::refcount,
                     no_refcount_policy>::value,
        get_gc_atom_impl,
        get_refcount_atom_impl>::template apply<T, MemoryPolicy>::type;

    impl_t impl_;
};

} // namespace immer
