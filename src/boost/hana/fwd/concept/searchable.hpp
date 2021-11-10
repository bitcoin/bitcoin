/*!
@file
Forward declares `boost::hana::Searchable`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_SEARCHABLE_HPP
#define BOOST_HANA_FWD_CONCEPT_SEARCHABLE_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Searchable Searchable
    //! The `Searchable` concept represents structures that can be searched.
    //!
    //! Intuitively, a `Searchable` is any structure, finite or infinite,
    //! containing elements that can be searched using a predicate. Sometimes,
    //! `Searchable`s will associate keys to values; one can search for a key
    //! with a predicate, and the value associated to it is returned. This
    //! gives rise to map-like data structures. Other times, the elements of
    //! the structure that are searched (i.e. those to which the predicate is
    //! applied) are the same that are returned, which gives rise to set-like
    //! data structures. In general, we will refer to the _keys_ of a
    //! `Searchable` structure as those elements that are used for searching,
    //! and to the _values_ of a `Searchable` as those elements that are
    //! returned when a search is successful. As was explained, there is no
    //! requirement that both notions differ, and it is often useful to have
    //! keys and values coincide (think about `std::set`).
    //!
    //! Some methods like `any_of`, `all_of` and `none_of` allow simple queries
    //! to be performed on the keys of the structure, while other methods like
    //! `find` and `find_if` make it possible to find the value associated
    //! to a key. The most specific method should always be used if one
    //! cares about performance, because it is usually the case that heavy
    //! optimizations can be performed in more specific methods. For example,
    //! an associative data structure implemented as a hash table will be much
    //! faster to access using `find` than `find_if`, because in the second
    //! case it will have to do a linear search through all the entries.
    //! Similarly, using `contains` will likely be much faster than `any_of`
    //! with an equivalent predicate.
    //!
    //! > __Insight__\n
    //! > In a lazy evaluation context, any `Foldable` can also become a model
    //! > of `Searchable` because we can search lazily through the structure
    //! > with `fold_right`. However, in the context of C++, some `Searchable`s
    //! > can not be folded; think for example of an infinite set.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `find_if` and `any_of`
    //!
    //! When `find_if` and `any_of` are provided, the other functions are
    //! implemented according to the laws explained below.
    //!
    //! @note
    //! We could implement `any_of(xs, pred)` by checking whether
    //! `find_if(xs, pred)` is an empty `optional` or not, and then reduce
    //! the minimal complete definition to `find_if`. However, this is not
    //! done because that implementation requires the predicate of `any_of`
    //! to return a compile-time `Logical`, which is more restrictive than
    //! what we have right now.
    //!
    //!
    //! Laws
    //! ----
    //! In order for the semantics of the methods to be consistent, some
    //! properties must be satisfied by any model of the `Searchable` concept.
    //! Rigorously, for any `Searchable`s  `xs` and `ys` and any predicate `p`,
    //! the following laws should be satisfied:
    //! @code
    //!     any_of(xs, p) <=> !all_of(xs, negated p)
    //!                   <=> !none_of(xs, p)
    //!
    //!     contains(xs, x) <=> any_of(xs, equal.to(x))
    //!
    //!     find(xs, x) == find_if(xs, equal.to(x))
    //!     find_if(xs, always(false_)) == nothing
    //!
    //!     is_subset(xs, ys) <=> all_of(xs, [](auto x) { return contains(ys, x); })
    //!     is_disjoint(xs, ys) <=> none_of(xs, [](auto x) { return contains(ys, x); })
    //! @endcode
    //!
    //! Additionally, if all the keys of the `Searchable` are `Logical`s,
    //! the following laws should be satisfied:
    //! @code
    //!     any(xs)  <=> any_of(xs, id)
    //!     all(xs)  <=> all_of(xs, id)
    //!     none(xs) <=> none_of(xs, id)
    //! @endcode
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::map`, `hana::optional`, `hana::range`, `hana::set`,
    //! `hana::string`, `hana::tuple`
    //!
    //!
    //! Free model for builtin arrays
    //! -----------------------------
    //! Builtin arrays whose size is known can be searched as-if they were
    //! homogeneous tuples. However, since arrays can only hold objects of
    //! a single type and the predicate to `find_if` must return a compile-time
    //! `Logical`, the `find_if` method is fairly useless. For similar reasons,
    //! the `find` method is also fairly useless. This model is provided mainly
    //! because of the `any_of` method & friends, which are both useful and
    //! compile-time efficient.
    //!
    //!
    //! Structure preserving functions
    //! ------------------------------
    //! Given two `Searchables` `S1` and `S2`, a function
    //! @f$ f : S_1(X) \to S_2(X) @f$ is said to preserve the `Searchable`
    //! structure if for all `xs` of data type `S1(X)` and predicates
    //! @f$ \mathtt{pred} : X \to Bool @f$ (for a `Logical` `Bool`),
    //! @code
    //!     any_of(xs, pred)  if and only if  any_of(f(xs), pred)
    //!     find_if(xs, pred) == find_if(f(xs), pred)
    //! @endcode
    //!
    //! This is really just a generalization of the following, more intuitive
    //! requirements. For all `xs` of data type `S1(X)` and `x` of data type
    //! `X`,
    //! @code
    //!     x ^in^ xs  if and only if  x ^in^ f(xs)
    //!     find(xs, x) == find(f(xs), x)
    //! @endcode
    //!
    //! These requirements can be understood as saying that `f` does not
    //! change the content of `xs`, although it may reorder elements.
    //! As usual, such a structure-preserving transformation is said to
    //! be an embedding if it is also injective, i.e. if it is a lossless
    //! transformation.
    template <typename S>
    struct Searchable;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_SEARCHABLE_HPP
