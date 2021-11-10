/*!
@file
Forward declares `boost::hana::Foldable`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_FOLDABLE_HPP
#define BOOST_HANA_FWD_CONCEPT_FOLDABLE_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Foldable Foldable
    //! The `Foldable` concept represents data structures that can be reduced
    //! to a single value.
    //!
    //! Generally speaking, folding refers to the concept of summarizing a
    //! complex structure as a single value, by successively applying a
    //! binary operation which reduces two elements of the structure to a
    //! single value. Folds come in many flavors; left folds, right folds,
    //! folds with and without an initial reduction state, and their monadic
    //! variants. This concept is able to express all of these fold variants.
    //!
    //! Another way of seeing `Foldable` is as data structures supporting
    //! internal iteration with the ability to accumulate a result. By
    //! internal iteration, we mean that the _loop control_ is in the hand
    //! of the structure, not the caller. Hence, it is the structure who
    //! decides when the iteration stops, which is normally when the whole
    //! structure has been consumed. Since C++ is an eager language, this
    //! requires `Foldable` structures to be finite, or otherwise one would
    //! need to loop indefinitely to consume the whole structure.
    //!
    //! @note
    //! While the fact that `Foldable` only works for finite structures may
    //! seem overly restrictive in comparison to the Haskell definition of
    //! `Foldable`, a finer grained separation of the concepts should
    //! mitigate the issue. For iterating over possibly infinite data
    //! structures, see the `Iterable` concept. For searching a possibly
    //! infinite data structure, see the `Searchable` concept.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `fold_left` or `unpack`
    //!
    //! However, please note that a minimal complete definition provided
    //! through `unpack` will be much more compile-time efficient than one
    //! provided through `fold_left`.
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::map`, `hana::optional`, `hana::pair`, `hana::set`,
    //! `hana::range`, `hana::tuple`
    //!
    //!
    //! @anchor Foldable-lin
    //! The linearization of a `Foldable`
    //! ---------------------------------
    //! Intuitively, for a `Foldable` structure `xs`, the _linearization_ of
    //! `xs` is the sequence of all the elements in `xs` as if they had been
    //! put in a list:
    //! @code
    //!     linearization(xs) = [x1, x2, ..., xn]
    //! @endcode
    //!
    //! Note that it is always possible to produce such a linearization
    //! for a finite `Foldable` by setting
    //! @code
    //!     linearization(xs) = fold_left(xs, [], flip(prepend))
    //! @endcode
    //! for an appropriate definition of `[]` and `prepend`. The notion of
    //! linearization is useful for expressing various properties of
    //! `Foldable` structures, and is used across the documentation. Also
    //! note that `Iterable`s define an [extended version](@ref Iterable-lin)
    //! of this allowing for infinite structures.
    //!
    //!
    //! Compile-time Foldables
    //! ----------------------
    //! A compile-time `Foldable` is a `Foldable` whose total length is known
    //! at compile-time. In other words, it is a `Foldable` whose `length`
    //! method returns a `Constant` of an unsigned integral type. When
    //! folding a compile-time `Foldable`, the folding can be unrolled,
    //! because the final number of steps of the algorithm is known at
    //! compile-time.
    //!
    //! Additionally, the `unpack` method is only available to compile-time
    //! `Foldable`s. This is because the return _type_ of `unpack` depends
    //! on the number of objects in the structure. Being able to resolve
    //! `unpack`'s return type at compile-time hence requires the length of
    //! the structure to be known at compile-time too.
    //!
    //! __In the current version of the library, only compile-time `Foldable`s
    //! are supported.__ While it would be possible in theory to support
    //! runtime `Foldable`s too, doing so efficiently requires more research.
    //!
    //!
    //! Provided conversion to `Sequence`s
    //! ----------------------------------
    //! Given a tag `S` which is a `Sequence`, an object whose tag is a model
    //! of the `Foldable` concept can be converted to an object of tag `S`.
    //! In other words, a `Foldable` can be converted to a `Sequence` `S`, by
    //! simply taking the linearization of the `Foldable` and creating the
    //! sequence with that. More specifically, given a `Foldable` `xs` with a
    //! linearization of `[x1, ..., xn]` and a `Sequence` tag `S`, `to<S>(xs)`
    //! is equivalent to `make<S>(x1, ..., xn)`.
    //! @include example/foldable/to.cpp
    //!
    //!
    //! Free model for builtin arrays
    //! -----------------------------
    //! Builtin arrays whose size is known can be folded as-if they were
    //! homogeneous tuples. However, note that builtin arrays can't be
    //! made more than `Foldable` (e.g. `Iterable`) because they can't
    //! be empty and they also can't be returned from functions.
    //!
    //!
    //! @anchor monadic-folds
    //! Primer on monadic folds
    //! -----------------------
    //! A monadic fold is a fold in which subsequent calls to the binary
    //! function are chained with the monadic `chain` operator of the
    //! corresponding Monad. This allows a structure to be folded in a
    //! custom monadic context. For example, performing a monadic fold with
    //! the `hana::optional` monad would require the binary function to return
    //! the result as a `hana::optional`, and the fold would abort and return
    //! `nothing` whenever one of the accumulation step would fail (i.e.
    //! return `nothing`). If, however, all the reduction steps succeed,
    //! then `just` the result would be returned. Different monads will of
    //! course result in different effects.
    template <typename T>
    struct Foldable;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_FOLDABLE_HPP
