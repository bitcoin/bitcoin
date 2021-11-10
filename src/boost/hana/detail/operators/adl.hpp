/*!
@file
Defines `boost::hana::detail::operators::adl`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_DETAIL_OPERATORS_ADL_HPP
#define BOOST_HANA_DETAIL_OPERATORS_ADL_HPP

#include <boost/hana/config.hpp>


BOOST_HANA_NAMESPACE_BEGIN namespace detail { namespace operators {
    //! @ingroup group-details
    //! Enables [ADL](http://en.cppreference.com/w/cpp/language/adl) in the
    //! `hana::detail::operators` namespace.
    //!
    //! This is used by containers in Hana as a quick way to automatically
    //! define the operators associated to some concepts, in conjunction
    //! with the `detail::xxx_operators` family of metafunctions.
    //!
    //! Note that `adl` can be passed template arguments to make it unique
    //! amongst a set of derived classes. This allows a set of derived classes
    //! not to possess a common base class, which would disable the EBO when
    //! many of these derived classes are stored in a Hana container. If EBO
    //! is not a concern, `adl<>` can simply be used.
    template <typename ...>
    struct adl { };
}} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_DETAIL_OPERATORS_ADL_HPP
