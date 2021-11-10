/*!
@file
Forward declares `boost::hana::IntegralConstant`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_CONCEPT_INTEGRAL_CONSTANT_HPP
#define BOOST_HANA_FWD_CONCEPT_INTEGRAL_CONSTANT_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    //! @ingroup group-concepts
    //! The `IntegralConstant` concept represents compile-time integral values.
    //!
    //! The `IntegralConstant` concept represents objects that hold a
    //! `constexpr` value of an integral type. In other words, it describes
    //! the essential functionality provided by `std::integral_constant`.
    //! An `IntegralConstant` is also just a special kind of `Constant`
    //! whose inner value is of an integral type.
    //!
    //!
    //! Minimal complete definition
    //! ---------------------------
    //! The requirements for being an `IntegralConstant` are quite simple.
    //! First, an `IntegralConstant` `C` must be a `Constant` such that
    //! `Tag::value_type` is an integral type, where `Tag` is the tag of `C`.
    //!
    //! Secondly, `C` must have a nested `static constexpr` member named
    //! `value`, such that the following code is valid:
    //! @code
    //!     constexpr auto v = C::value;
    //! @endcode
    //! Because of the requirement that `Tag::value_type` be an integral type,
    //! it follows that `C::value` must be an integral value.
    //!
    //! Finally, it is necessary to specialize the `IntegralConstant` template
    //! in the `boost::hana` namespace to tell Hana that a type is a model
    //! of `IntegralConstant`:
    //! @code
    //!     namespace boost { namespace hana {
    //!         template <>
    //!         struct IntegralConstant<your_custom_tag> {
    //!             static constexpr bool value = true;
    //!         };
    //!     }}
    //! @endcode
    //!
    //!
    //! Refined concept
    //! ---------------
    //! 1. `Constant` (free implementation of `value`)\n
    //! The `value` function required to be a `Constant` can be implemented
    //! as follows for `IntegralConstant`s:
    //! @code
    //!     value<C>() == C::value
    //! @endcode
    //! The `to` function must still be provided explicitly for the model
    //! of `Constant` to be complete.
    //!
    //!
    //! Concrete models
    //! ---------------
    //! `hana::integral_constant`
    template <typename C>
    struct IntegralConstant;
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_CONCEPT_INTEGRAL_CONSTANT_HPP
