/*!
@file
Forward declares `boost::hana::keys`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_FWD_KEYS_HPP
#define BOOST_HANA_FWD_KEYS_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/core/when.hpp>


BOOST_HANA_NAMESPACE_BEGIN
    // Note: This function is documented per datatype/concept only.
    //! @cond
    template <typename T, typename = void>
    struct keys_impl : keys_impl<T, when<true>> { };
    //! @endcond

    struct keys_t {
        template <typename Map>
        constexpr auto operator()(Map&& map) const;
    };

    BOOST_HANA_INLINE_VARIABLE constexpr keys_t keys{};

    //! Returns a `Sequence` containing the name of the members of
    //! the data structure.
    //! @ingroup group-Struct
    //!
    //! Given a `Struct` object, `keys` returns a `Sequence` containing the
    //! name of all the members of the `Struct`, in the same order as they
    //! appear in the `accessors` sequence.
    //!
    //!
    //! Example
    //! -------
    //! @include example/struct/keys.cpp
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    constexpr auto keys = [](auto&& object) {
        return implementation_defined;
    };
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_FWD_KEYS_HPP
