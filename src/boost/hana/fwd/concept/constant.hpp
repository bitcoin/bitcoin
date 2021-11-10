/*!
@file
Forward declares `boost::hana::Constant`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_CONSTANT_HPP
#define BOOST_HANA_FWD_CONCEPT_CONSTANT_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Constant Constant
    //! The `Constant` concept represents data that can be manipulated at
    //! compile-time.
    //!
    //! At its core, `Constant` is simply a generalization of the principle
    //! behind `std::integral_constant` to all types that can be constructed
    //! at compile-time, i.e. to all types with a `constexpr` constructor
    //! (also called [Literal types][1]). More specifically, a `Constant` is
    //! an object from which a `constexpr` value may be obtained (through the
    //! `value` method) regardless of the `constexpr`ness of the object itself.
    //!
    //! All `Constant`s must be somewhat equivalent, in the following sense.
    //! Let `C(T)` and `D(U)` denote the tags of `Constant`s holding objects
    //! of type `T` and `U`, respectively. Then, an object with tag `D(U)`
    //! must be convertible to an object with tag `C(T)` whenever `U` is
    //! convertible to `T`, as determined by `is_convertible`. The
    //! interpretation here is that a `Constant` is just a box holding
    //! an object of some type, and it should be possible to swap between
    //! boxes whenever the objects inside the boxes can be swapped.
    //!
    //! Because of this last requirement, one could be tempted to think that
    //! specialized "boxes" like `std::integral_constant` are prevented from
    //! being `Constant`s because they are not able to hold objects of any
    //! type `T` (`std::integral_constant` may only hold integral types).
    //! This is false; the requirement should be interpreted as saying that
    //! whenever `C(T)` is _meaningful_ (e.g. only when `T` is integral for
    //! `std::integral_constant`) _and_ there exists a conversion from `U`
    //! to `T`, then a conversion from `D(U)` to `C(T)` should also exist.
    //! The precise requirements for being a `Constant` are embodied in the
    //! following laws.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `value` and `to`, satisfying the laws below.
    //!
    //!
    //! Laws
    //! ----
    //! Let `c` be an object of with tag `C`, which represents a `Constant`
    //! holding an object with tag `T`. The first law ensures that the value
    //! of the wrapped object is always a constant expression by requiring
    //! the following to be well-formed:
    //! @code
    //!     constexpr auto x = hana::value<decltype(c)>();
    //! @endcode
    //!
    //! This means that the `value` function must return an object that can
    //! be constructed at compile-time. It is important to note how `value`
    //! only receives the type of the object and not the object itself.
    //! This is the core of the `Constant` concept; it means that the only
    //! information required to implement `value` must be stored in the _type_
    //! of its argument, and hence be available statically.
    //!
    //! The second law that must be satisfied ensures that `Constant`s are
    //! basically dumb boxes, which makes it possible to provide models for
    //! many concepts without much work from the user. The law simply asks
    //! for the following expression to be valid:
    //! @code
    //!     to<C>(i)
    //! @endcode
    //! where, `i` is an _arbitrary_ `Constant` holding an internal value
    //! with a tag that can be converted to `T`, as determined by the
    //! `hana::is_convertible` metafunction. In other words, whenever `U` is
    //! convertible to `T`, a `Constant` holding a `U` is convertible to
    //! a `Constant` holding a `T`, if such a `Constant` can be created.
    //!
    //! Finally, the tag `C` must provide a nested `value_type` alias to `T`,
    //! which allows us to query the tag of the inner value held by objects
    //! with tag `C`. In other words, the following must be true for any
    //! object `c` with tag `C`:
    //! @code
    //!     std::is_same<
    //!         C::value_type,
    //!         tag_of<decltype(hana::value(c))>::type
    //!     >::value
    //! @endcode
    //!
    //!
    //! Refined concepts
    //! ----------------
    //! In certain cases, a `Constant` can automatically be made a model of
    //! another concept. In particular, if a `Constant` `C` is holding an
    //! object of tag `T`, and if `T` models a concept `X`, then `C` may
    //! in most cases model `X` by simply performing whatever operation is
    //! required on its underlying value, and then wrapping the result back
    //! in a `C`.
    //!
    //! More specifically, if a `Constant` `C` has an underlying value
    //! (`C::value_type`) which is a model of `Comparable`, `Orderable`,
    //! `Logical`, or `Monoid` up to `EuclideanRing`, then `C` must also
    //! be a model of those concepts. In other words, when `C::value_type`
    //! models one of the listed concepts, `C` itself must also model that
    //! concept. However, note that free models are provided for all of
    //! those concepts, so no additional work must be done.
    //!
    //! While it would be possible in theory to provide models for concepts
    //! like `Foldable` too, only a couple of concepts are useful to have as
    //! `Constant` in practice. Providing free models for the concepts listed
    //! above is useful because it allows various types of integral constants
    //! (`std::integral_constant`, `mpl::integral_c`, etc...) to easily have
    //! models for them just by defining the `Constant` concept.
    //!
    //! @remark
    //! An interesting observation is that `Constant` is actually the
    //! canonical embedding of the subcategory of `constexpr` things
    //! into the Hana category, which contains everything in this library.
    //! Hence, whatever is true in that subcategory is also true here, via
    //! this functor. This is why we can provide models of any concept that
    //! works on `constexpr` things for Constants, by simply passing them
    //! through that embedding.
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::integral_constant`
    //!
    //!
    //! Provided conversion to the tag of the underlying value
    //! ------------------------------------------------------
    //! Any `Constant` `c` holding an underlying value of tag `T` is
    //! convertible to any tag `U` such that `T` is convertible to `U`.
    //! Specifically, the conversion is equivalent to
    //! @code
    //!     to<U>(c) == to<U>(value<decltype(c)>())
    //! @endcode
    //!
    //! Also, those conversions are marked as an embedding whenever the
    //! conversion of underlying types is an embedding. This is to allow
    //! Constants to inter-operate with `constexpr` objects easily:
    //! @code
    //!     plus(int_c<1>, 1) == 2
    //! @endcode
    //!
    //! Strictly speaking, __this is sometimes a violation__ of what it means
    //! to be an embedding. Indeed, while there exists an embedding from any
    //! Constant to a `constexpr` object (since Constant is just the canonical
    //! inclusion), there is no embedding from a Constant to a runtime
    //! object since we would lose the ability to define the `value` method
    //! (the `constexpr`ness of the object would have been lost). Since there
    //! is no way to distinguish `constexpr` and non-`constexpr` objects based
    //! on their type, Hana has no way to know whether the conversion is to a
    //! `constexpr` object of not. In other words, the `to` method has no way
    //! to differentiate between
    //! @code
    //!     constexpr int i = hana::to<int>(int_c<1>);
    //! @endcode
    //! which is an embedding, and
    //! @code
    //!     int i = hana::to<int>(int_c<1>);
    //! @endcode
    //!
    //! which isn't. To be on the safer side, we could mark the conversion
    //! as not-an-embedding. However, if e.g. the conversion from
    //! `integral_constant_tag<int>` to `int` was not marked as an embedding,
    //! we would have to write `plus(to<int>(int_c<1>), 1)` instead of just
    //! `plus(int_c<1>, 1)`, which is cumbersome. Hence, the conversion is
    //! marked as an embedding, but this also means that code like
    //! @code
    //!     int i = 1;
    //!     plus(int_c<1>, i);
    //! @endcode
    //! will be considered valid, which implicitly loses the fact that
    //! `int_c<1>` is a Constant, and hence does not follow the usual rules
    //! for cross-type operations in Hana.
    //!
    //!
    //! Provided common data type
    //! -------------------------
    //! Because of the requirement that `Constant`s be interchangeable when
    //! their contents are compatible, two `Constant`s `A` and `B` will have
    //! a common data type whenever `A::value_type` and `B::value_type` have
    //! one. Their common data type is an unspecified `Constant` `C` such
    //! that `C::value_type` is exactly `common_t<A::value_type, B::value_type>`.
    //! A specialization of the `common` metafunction is provided for
    //! `Constant`s to reflect this.
    //!
    //! In the same vein, a common data type is also provided from any
    //! constant `A` to a type `T` such that `A::value_type` and `T` share
    //! a common type. The common type between `A` and `T` is obviously the
    //! common type between `A::value_type` and `T`. As explained above in
    //! the section on conversions, this is sometimes a violation of the
    //! definition of a common type, because there must be an embedding
    //! to the common type, which is not always the case. For the same
    //! reasons as explained above, this common type is still provided.
    //!
    //!
    //! [1]: http://en.cppreference.com/w/cpp/named_req/LiteralType
    template <typename C>
    struct Constant;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_CONSTANT_HPP
