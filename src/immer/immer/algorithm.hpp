//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <numeric>
#include <type_traits>

namespace immer {

/**
 * @defgroup algorithm
 * @{
 */

/*@{*/
// Right now these algorithms dispatch directly to the vector
// implementations unconditionally.  This will be changed in the
// future to support other kinds of containers.

/*!
 * Apply operation `fn` for every contiguous *chunk* of data in the
 * range sequentially.  Each time, `Fn` is passed two `value_type`
 * pointers describing a range over a part of the vector.  This allows
 * iterating over the elements in the most efficient way.
 *
 * @rst
 *
 * .. tip:: This is a low level method. Most of the time, :doc:`other
 *    wrapper algorithms <algorithms>` should be used instead.
 *
 * @endrst
 */
template <typename Range, typename Fn>
void for_each_chunk(const Range& r, Fn&& fn)
{
    r.impl().for_each_chunk(std::forward<Fn>(fn));
}

template <typename Iterator, typename Fn>
void for_each_chunk(const Iterator& first, const Iterator& last, Fn&& fn)
{
    assert(&first.impl() == &last.impl());
    first.impl().for_each_chunk(
        first.index(), last.index(), std::forward<Fn>(fn));
}

template <typename T, typename Fn>
void for_each_chunk(const T* first, const T* last, Fn&& fn)
{
    std::forward<Fn>(fn)(first, last);
}

/*!
 * Apply operation `fn` for every contiguous *chunk* of data in the
 * range sequentially, until `fn` returns `false`.  Each time, `Fn` is
 * passed two `value_type` pointers describing a range over a part of
 * the vector.  This allows iterating over the elements in the most
 * efficient way.
 *
 * @rst
 *
 * .. tip:: This is a low level method. Most of the time, :doc:`other
 *    wrapper algorithms <algorithms>` should be used instead.
 *
 * @endrst
 */
template <typename Range, typename Fn>
bool for_each_chunk_p(const Range& r, Fn&& fn)
{
    return r.impl().for_each_chunk_p(std::forward<Fn>(fn));
}

template <typename Iterator, typename Fn>
bool for_each_chunk_p(const Iterator& first, const Iterator& last, Fn&& fn)
{
    assert(&first.impl() == &last.impl());
    return first.impl().for_each_chunk_p(
        first.index(), last.index(), std::forward<Fn>(fn));
}

template <typename T, typename Fn>
bool for_each_chunk_p(const T* first, const T* last, Fn&& fn)
{
    return std::forward<Fn>(fn)(first, last);
}

namespace detail {

template <class Iter, class T>
T accumulate_move(Iter first, Iter last, T init)
{
    for (; first != last; ++first)
        init = std::move(init) + *first;
    return init;
}

template <class Iter, class T, class Fn>
T accumulate_move(Iter first, Iter last, T init, Fn op)
{
    for (; first != last; ++first)
        init = op(std::move(init), *first);
    return init;
}

} // namespace detail

/*!
 * Equivalent of `std::accumulate` applied to the range `r`.
 */
template <typename Range, typename T>
T accumulate(Range&& r, T init)
{
    for_each_chunk(r, [&](auto first, auto last) {
        init = detail::accumulate_move(first, last, init);
    });
    return init;
}

template <typename Range, typename T, typename Fn>
T accumulate(Range&& r, T init, Fn fn)
{
    for_each_chunk(r, [&](auto first, auto last) {
        init = detail::accumulate_move(first, last, init, fn);
    });
    return init;
}

/*!
 * Equivalent of `std::accumulate` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename Iterator, typename T>
T accumulate(Iterator first, Iterator last, T init)
{
    for_each_chunk(first, last, [&](auto first, auto last) {
        init = detail::accumulate_move(first, last, init);
    });
    return init;
}

template <typename Iterator, typename T, typename Fn>
T accumulate(Iterator first, Iterator last, T init, Fn fn)
{
    for_each_chunk(first, last, [&](auto first, auto last) {
        init = detail::accumulate_move(first, last, init, fn);
    });
    return init;
}

/*!
 * Equivalent of `std::for_each` applied to the range `r`.
 */
template <typename Range, typename Fn>
Fn&& for_each(Range&& r, Fn&& fn)
{
    for_each_chunk(r, [&](auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

/*!
 * Equivalent of `std::for_each` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename Iterator, typename Fn>
Fn&& for_each(Iterator first, Iterator last, Fn&& fn)
{
    for_each_chunk(first, last, [&](auto first, auto last) {
        for (; first != last; ++first)
            fn(*first);
    });
    return std::forward<Fn>(fn);
}

/*!
 * Equivalent of `std::copy` applied to the range `r`.
 */
template <typename Range, typename OutIter>
OutIter copy(Range&& r, OutIter out)
{
    for_each_chunk(
        r, [&](auto first, auto last) { out = std::copy(first, last, out); });
    return out;
}

/*!
 * Equivalent of `std::copy` applied to the range @f$ [first,
 * last) @f$.
 */
template <typename InIter, typename OutIter>
OutIter copy(InIter first, InIter last, OutIter out)
{
    for_each_chunk(first, last, [&](auto first, auto last) {
        out = std::copy(first, last, out);
    });
    return out;
}

/*!
 * Equivalent of `std::all_of` applied to the range `r`.
 */
template <typename Range, typename Pred>
bool all_of(Range&& r, Pred p)
{
    return for_each_chunk_p(
        r, [&](auto first, auto last) { return std::all_of(first, last, p); });
}

/*!
 * Equivalent of `std::all_of` applied to the range @f$ [first, last)
 * @f$.
 */
template <typename Iter, typename Pred>
bool all_of(Iter first, Iter last, Pred p)
{
    return for_each_chunk_p(first, last, [&](auto first, auto last) {
        return std::all_of(first, last, p);
    });
}

/*!
 * Object that can be used to process changes as computed by the @a diff
 * algorithm.
 *
 * @tparam AddedFn Unary function that is be called whenever an added element is
 *         found. It is called with the added element as argument.
 *
 * @tparam RemovedFn Unary function that is called whenever a removed element is
 *         found.  It is called with the removed element as argument.
 *
 * @tparam ChangedFn Unary function that is called whenever a changed element is
 *         found.  It is called with the changed element as argument.
 */
template <class AddedFn, class RemovedFn, class ChangedFn>
struct differ
{
    AddedFn added;
    RemovedFn removed;
    ChangedFn changed;
};

/*!
 * Produces a @a differ object with `added`, `removed` and `changed` functions.
 */
template <class AddedFn, class RemovedFn, class ChangedFn>
auto make_differ(AddedFn&& added, RemovedFn&& removed, ChangedFn&& changed)
    -> differ<std::decay_t<AddedFn>,
              std::decay_t<RemovedFn>,
              std::decay_t<ChangedFn>>
{
    return {std::forward<AddedFn>(added),
            std::forward<RemovedFn>(removed),
            std::forward<ChangedFn>(changed)};
}

/*!
 * Produces a @a differ object with `added` and `removed` functions and no
 * `changed` function.
 */
template <class AddedFn, class RemovedFn>
auto make_differ(AddedFn&& added, RemovedFn&& removed)
{
    return make_differ(std::forward<AddedFn>(added),
                       std::forward<RemovedFn>(removed),
                       [](auto&&...) {});
}

/*!
 * Compute the differences between `a` and `b`.
 *
 * Changes detected are notified via the differ object, which should support the
 * following expressions:
 *
 *   - `differ.added(x)`, invoked when element `x` is found in `b` but not in
 *      `a`.
 *
 *   - `differ.removed(x)`, invoked when element `x` is found in `a` but not in
 *      `b`.
 *
 *   - `differ.changed(x, y)`, invoked when element `x` and `y` from `a` and `b`
 *      share the same key but map to a different value.
 *
 * This method leverages structural sharing to offer a complexity @f$ O(|diff|)
 * @f$ when `b` is derived from `a` by performing @f$ |diff| @f$ updates.  This
 * is, this function can detect changes in effectively constant time per update,
 * as oposed to the @f$ O(|a|+|b|) @f$ complexity of a trivial implementation.
 *
 * @rst
 *
 * .. note:: This method is only implemented for ``map`` and ``set``. When sets
 *           are diffed, the ``changed`` function is never called.
 *
 * @endrst
 */
template <typename T, typename Differ>
void diff(const T& a, const T& b, Differ&& differ)
{
    a.impl().template diff<std::equal_to<typename T::value_type>>(
        b.impl(), std::forward<Differ>(differ));
}

/*!
 * Compute the differences between `a` and `b` using the callbacks in `fns` as
 * differ.  Equivalent to `diff(a, b, make_differ(fns)...)`.
 */
template <typename T, typename... Fns>
void diff(const T& a, const T& b, Fns&&... fns)
{
    diff(a, b, make_differ(std::forward<Fns>(fns)...));
}

/** @} */ // group: algorithm

} // namespace immer
