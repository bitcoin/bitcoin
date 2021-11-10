/*!
@file
Forward declares `boost::hana::MonadPlus`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_MONAD_PLUS_HPP
#define BOOST_HANA_FWD_CONCEPT_MONAD_PLUS_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-MonadPlus MonadPlus
    //! The `MonadPlus` concept represents Monads with a monoidal structure.
    //!
    //! Intuitively, whereas a Monad can be seen as some kind of container
    //! or context, a MonadPlus can be seen as a container or a context that
    //! can be concatenated with other containers or contexts. There must
    //! also be an identity element for this combining operation. For example,
    //! a tuple is a MonadPlus, because tuples can be concatenated and the
    //! empty tuple would act as an identity for concatenation. How is this
    //! different from a Monad which is also a Monoid? The answer is that the
    //! monoidal structure on a MonadPlus must _not_ depend of the contents
    //! of the structure; it must not require the contents to be a Monoid
    //! in order to work.
    //!
    //! While sequences are not the only possible model for MonadPlus, the
    //! method names used here refer to the MonadPlus of sequences under
    //! concatenation. Several useful functions generalizing operations on
    //! sequences are included with this concept, like `append`, `prepend`
    //! and `filter`.
    //!
    //! @note
    //! This documentation does not go into much details about the nature
    //! of the MonadPlus concept. However, there is a nice Haskell-oriented
    //! [WikiBook][1] going into further details.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! `concat` and `empty`
    //!
    //!
    //! Laws
    //! ----
    //! First, a MonadPlus is required to have a monoidal structure. Hence, it
    //! is no surprise that for any MonadPlus `M`, we require `M(T)` to be a
    //! valid monoid. However, we do not enforce that `M(T)` actually models
    //! the Monoid concept provided by Hana. Further, for all objects `a, b, c`
    //! of data type `M(T)`,
    //! @code
    //!     // identity
    //!     concat(empty<M(T)>(), a) == a
    //!     concat(a, empty<M(T)>()) == a
    //!
    //!     // associativity
    //!     concat(a, concat(b, c)) == concat(concat(a, b), c)
    //! @endcode
    //!
    //! Secondly, a MonadPlus is also required to obey the following laws,
    //! which represent the fact that `empty<M(T)>()` must be some kind of
    //! absorbing element for the `chain` operation. For all objects `a` of
    //! data type `M(T)` and functions @f$ f : T \to M(U) @f$,
    //! @code
    //!     chain(empty<M(T)>(), f)         == empty<M(U)>()
    //!     chain(a, always(empty<M(T)>())) == empty<M(U)>()
    //! @endcode
    //!
    //!
    //! Refined concepts
    //! ----------------
    //! `Functor`, `Applicative` and `Monad`
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::optional`, `hana::tuple`
    //!
    //! [1]: https://en.wikibooks.org/wiki/Haskell/MonadPlus
    template <typename M>
    struct MonadPlus;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_MONAD_PLUS_HPP
