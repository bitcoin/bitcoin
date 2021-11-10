/*!
@file
Forward declares `boost::hana::Metafunction`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_METAFUNCTION_HPP
#define BOOST_HANA_FWD_CONCEPT_METAFUNCTION_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! @defgroup group-Metafunction Metafunction
    //! A `Metafunction` is a function that takes `hana::type`s as inputs and
    //! returns a `hana::type` as output.
    //!
    //! A `Metafunction` is an object satisfying the [FunctionObject][1]
    //! concept, but with additional requirements. First, it must be possible
    //! to apply a `Metafunction` to arguments whose tag is `type_tag`, and
    //! the result of such an application must be an object whose tag is also
    //! `type_tag`. Note that `hana::type` and `hana::basic_type` are the
    //! only such types.
    //!
    //! Secondly, a `Metafunction` must provide a nested `::%apply` template
    //! which allows performing the same type-level computation as is done by
    //! the call operator. In Boost.MPL parlance, a `Metafunction` `F` is
    //! hence a [MetafunctionClass][2] in addition to being a `FunctionObject`.
    //! Rigorously, the following must be satisfied by any object `f` of type
    //! `F` which is a `Metafunction`, and for arbitrary types `T...`:
    //! @code
    //!     f(hana::type_c<T>...) == hana::type_c<F::apply<T...>::type>
    //! @endcode
    //!
    //! Thirdly, to ease the inter-operation of values and types,
    //! `Metafunction`s must also allow being called with arguments that
    //! are not `hana::type`s. In that case, the result is equivalent to
    //! calling the metafunction on the types of the arguments. Rigorously,
    //! this means that for arbitrary objects `x...`,
    //! @code
    //!     f(x...) == f(hana::type_c<decltype(x)>...)
    //! @endcode
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! The `Metafunction` concept does not have a minimal complete definition
    //! in terms of tag-dispatched methods. Instead, the syntactic requirements
    //! documented above should be satisfied, and the `Metafunction` struct
    //! should be specialized explicitly in Hana's namespace.
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::metafunction`, `hana::metafunction_class`, `hana::template_`
    //!
    //!
    //! Rationale: Why aren't `Metafunction`s `Comparable`?
    //! ---------------------------------------------------
    //! When seeing `hana::template_`, a question that naturally arises is
    //! whether `Metafunction`s should be made `Comparable`. Indeed, it
    //! would seem to make sense to compare two templates `F` and `G` with
    //! `template_<F> == template_<G>`. However, in the case where `F` and/or
    //! `G` are alias templates, it makes sense to talk about two types of
    //! comparisons. The first one is _shallow_ comparison, and it determines
    //! that two alias templates are equal if they are the same alias
    //! template. The second one is _deep_ comparison, and it determines
    //! that two template aliases are equal if they alias the same type for
    //! any template argument. For example, given `F` and `G` defined as
    //! @code
    //!     template <typename T>
    //!     using F = void;
    //!
    //!     template <typename T>
    //!     using G = void;
    //! @endcode
    //!
    //! shallow comparison would determine that `F` and `G` are different
    //! because they are two different template aliases, while deep comparison
    //! would determine that `F` and `G` are equal because they always
    //! expand to the same type, `void`. Unfortunately, deep comparison is
    //! impossible to implement because one would have to check `F` and `G`
    //! on all possible types. On the other hand, shallow comparison is not
    //! satisfactory because `Metafunction`s are nothing but functions on
    //! `type`s, and the equality of two functions is normally defined with
    //! deep comparison. Hence, we adopt a conservative stance and avoid
    //! providing comparison for `Metafunction`s.
    //!
    //! [1]: http://en.cppreference.com/w/cpp/named_req/FunctionObject
    //! [2]: http://www.boost.org/doc/libs/release/libs/mpl/doc/refmanual/metafunction-class.html
    template <typename F>
    struct Metafunction;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_METAFUNCTION_HPP
