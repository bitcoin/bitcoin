/*!
@file
Forward declares `boost::hana::Sequence`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_SEQUENCE_HPP
#define BOOST_HANA_FWD_CONCEPT_SEQUENCE_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Sequence Sequence
    //! The `Sequence` concept represents generic index-based sequences.
    //!
    //! Compared to other abstract concepts, the Sequence concept is very
    //! specific. It represents generic index-based sequences. The reason
    //! why such a specific concept is provided is because there are a lot
    //! of models that behave exactly the same while being implemented in
    //! wildly different ways. It is useful to regroup all those data types
    //! under the same umbrella for the purpose of generic programming.
    //!
    //! In fact, models of this concept are not only _similar_. They are
    //! actually _isomorphic_, in a sense that we define below, which is
    //! a fancy way of rigorously saying that they behave exactly the same
    //! to an external observer.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `Iterable`, `Foldable`, and `make`
    //!
    //! The `Sequence` concept does not provide basic methods that could be
    //! used as a minimal complete definition; instead, it borrows methods
    //! from other concepts and add laws to them. For this reason, it is
    //! necessary to specialize the `Sequence` metafunction in Hana's
    //! namespace to tell Hana that a type is indeed a `Sequence`. Explicitly
    //! specializing the `Sequence` metafunction can be seen like a seal
    //! saying "this data type satisfies the additional laws of a `Sequence`",
    //! since those can't be checked by Hana automatically.
    //!
    //!
    //! Laws
    //! ----
    //! The laws for being a `Sequence` are simple, and their goal is to
    //! restrict the semantics that can be associated to the functions
    //! provided by other concepts. First, a `Sequence` must be a finite
    //! `Iterable` (thus a `Foldable` too). Secondly, for a `Sequence` tag
    //! `S`, `make<S>(x1, ..., xn)` must be an object of tag `S` and whose
    //! linearization is `[x1, ..., xn]`. This basically ensures that objects
    //! of tag `S` are equivalent to their linearization, and that they can
    //! be created from such a linearization (with `make`).
    //!
    //! While it would be possible in theory to handle infinite sequences,
    //! doing so complicates the implementation of many algorithms. For
    //! simplicity, the current version of the library only handles finite
    //! sequences. However, note that this does not affect in any way the
    //! potential for having infinite `Searchable`s and `Iterable`s.
    //!
    //!
    //! Refined concepts
    //! ----------------
    //! 1. `Comparable` (definition provided automatically)\n
    //! Two `Sequence`s are equal if and only if they contain the same number
    //! of elements and their elements at any given index are equal.
    //! @include example/sequence/comparable.cpp
    //!
    //! 2. `Orderable` (definition provided automatically)\n
    //! `Sequence`s are ordered using the traditional lexicographical ordering.
    //! @include example/sequence/orderable.cpp
    //!
    //! 3. `Functor` (definition provided automatically)\n
    //! `Sequence`s implement `transform` as the mapping of a function over
    //! each element of the sequence. This is somewhat equivalent to what
    //! `std::transform` does to ranges of iterators. Also note that mapping
    //! a function over an empty sequence returns an empty sequence and never
    //! applies the function, as would be expected.
    //! @include example/sequence/functor.cpp
    //!
    //! 4. `Applicative` (definition provided automatically)\n
    //! First, `lift`ing a value into a `Sequence` is the same as creating a
    //! singleton sequence containing that value. Second, applying a sequence
    //! of functions to a sequence of values will apply each function to
    //! all the values in the sequence, and then return a list of all the
    //! results. In other words,
    //! @code
    //!     ap([f1, ..., fN], [x1, ..., xM]) == [
    //!         f1(x1), ..., f1(xM),
    //!         ...
    //!         fN(x1), ..., fN(xM)
    //!     ]
    //! @endcode
    //! Example:
    //! @include example/sequence/applicative.cpp
    //!
    //! 5. `Monad` (definition provided automatically)\n
    //! First, `flaten`ning a `Sequence` takes a sequence of sequences and
    //! concatenates them to get a larger sequence. In other words,
    //! @code
    //!     flatten([[a1, ..., aN], ..., [z1, ..., zM]]) == [
    //!         a1, ..., aN, ..., z1, ..., zM
    //!     ]
    //! @endcode
    //! This acts like a `std::tuple_cat` function, except it receives a
    //! sequence of sequences instead of a variadic pack of sequences to
    //! flatten.\n
    //! __Example__:
    //! @include example/sequence/monad.ints.cpp
    //! Also note that the model of `Monad` for `Sequence`s can be seen as
    //! modeling nondeterminism. A nondeterministic computation can be
    //! modeled as a function which returns a sequence of possible results.
    //! In this line of thought, `chain`ing a sequence of values into such
    //! a function will return a sequence of all the possible output values,
    //! i.e. a sequence of all the values applied to all the functions in
    //! the sequences.\n
    //! __Example__:
    //! @include example/sequence/monad.types.cpp
    //!
    //! 6. `MonadPlus` (definition provided automatically)\n
    //! `Sequence`s are models of the `MonadPlus` concept by considering the
    //! empty sequence as the unit of `concat`, and sequence concatenation
    //! as `concat`.
    //! @include example/sequence/monad_plus.cpp
    //!
    //! 7. `Foldable`\n
    //! The model of `Foldable` for `Sequence`s is uniquely determined by the
    //! model of `Iterable`.
    //! @include example/sequence/foldable.cpp
    //!
    //! 8. `Iterable`\n
    //! The model of `Iterable` for `Sequence`s corresponds to iteration over
    //! each element of the sequence, in order. This model is not provided
    //! automatically, and it is in fact part of the minimal complete
    //! definition for the `Sequence` concept.
    //! @include example/sequence/iterable.cpp
    //!
    //! 9. `Searchable` (definition provided automatically)\n
    //! Searching through a `Sequence` is equivalent to just searching through
    //! a list of the values it contains. The keys and the values on which
    //! the search is performed are both the elements of the sequence.
    //! @include example/sequence/searchable.cpp
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::tuple`
    //!
    //!
    //! [1]: http://en.wikipedia.org/wiki/Isomorphism#Isomorphism_vs._bijective_morphism
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    template <typename S>
    struct Sequence;
#else
    template <typename S, typename = void>
    struct Sequence : Sequence<S, when<true>> { };
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_SEQUENCE_HPP
