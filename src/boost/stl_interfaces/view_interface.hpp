// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_VIEW_INTERFACE_HPP
#define BOOST_STL_INTERFACES_VIEW_INTERFACE_HPP

#include <boost/stl_interfaces/fwd.hpp>


namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V1 {

    /** A CRTP template that one may derive from to make it easier to define
        `std::ranges::view`-like types with a container-like interface.  This
        is a pre-C++20 version of C++20's `view_interface` (see
        [view.interface] in the C++ standard).

        The template parameter `D` for `view_interface` may be an incomplete
        type.  Before any member of the resulting specialization of
        `view_interface` other than special member functions is referenced,
        `D` shall be complete, and model both
        `std::derived_from<view_interface<D>>` and `std::view`. */
    template<
        typename Derived,
        element_layout Contiguity = element_layout::discontiguous
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E = std::enable_if_t<
            std::is_class<Derived>::value &&
            std::is_same<Derived, std::remove_cv_t<Derived>>::value>
#endif
        >
    struct view_interface;

    namespace v1_dtl {
        template<typename D, element_layout Contiguity>
        void derived_view(view_interface<D, Contiguity> const &);
    }

    template<
        typename Derived,
        element_layout Contiguity
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E
#endif
        >
    struct view_interface
    {
#ifndef BOOST_STL_INTERFACES_DOXYGEN
    private:
        constexpr Derived & derived() noexcept
        {
            return static_cast<Derived &>(*this);
        }
        constexpr const Derived & derived() const noexcept
        {
            return static_cast<Derived const &>(*this);
        }
#endif

    public:
        template<typename D = Derived>
        constexpr auto empty() noexcept(
            noexcept(std::declval<D &>().begin() == std::declval<D &>().end()))
            -> decltype(
                std::declval<D &>().begin() == std::declval<D &>().end())
        {
            return derived().begin() == derived().end();
        }
        template<typename D = Derived>
        constexpr auto empty() const noexcept(noexcept(
            std::declval<D const &>().begin() ==
            std::declval<D const &>().end()))
            -> decltype(
                std::declval<D const &>().begin() ==
                std::declval<D const &>().end())
        {
            return derived().begin() == derived().end();
        }

        template<
            typename D = Derived,
            typename R = decltype(std::declval<D &>().empty())>
        constexpr explicit
        operator bool() noexcept(noexcept(std::declval<D &>().empty()))
        {
            return !derived().empty();
        }
        template<
            typename D = Derived,
            typename R = decltype(std::declval<D const &>().empty())>
        constexpr explicit operator bool() const
            noexcept(noexcept(std::declval<D const &>().empty()))
        {
            return !derived().empty();
        }

        template<
            typename D = Derived,
            element_layout C = Contiguity,
            typename Enable = std::enable_if_t<C == element_layout::contiguous>>
        constexpr auto data() noexcept(noexcept(std::declval<D &>().begin()))
            -> decltype(std::addressof(*std::declval<D &>().begin()))
        {
            return std::addressof(*derived().begin());
        }
        template<
            typename D = Derived,
            element_layout C = Contiguity,
            typename Enable = std::enable_if_t<C == element_layout::contiguous>>
        constexpr auto data() const
            noexcept(noexcept(std::declval<D const &>().begin()))
                -> decltype(std::addressof(*std::declval<D const &>().begin()))
        {
            return std::addressof(*derived().begin());
        }

        template<typename D = Derived>
        constexpr auto size() noexcept(
            noexcept(std::declval<D &>().end() - std::declval<D &>().begin()))
            -> decltype(std::declval<D &>().end() - std::declval<D &>().begin())
        {
            return derived().end() - derived().begin();
        }
        template<typename D = Derived>
        constexpr auto size() const noexcept(noexcept(
            std::declval<D const &>().end() -
            std::declval<D const &>().begin()))
            -> decltype(
                std::declval<D const &>().end() -
                std::declval<D const &>().begin())
        {
            return derived().end() - derived().begin();
        }

        template<typename D = Derived>
        constexpr auto front() noexcept(noexcept(*std::declval<D &>().begin()))
            -> decltype(*std::declval<D &>().begin())
        {
            return *derived().begin();
        }
        template<typename D = Derived>
        constexpr auto front() const
            noexcept(noexcept(*std::declval<D const &>().begin()))
                -> decltype(*std::declval<D const &>().begin())
        {
            return *derived().begin();
        }

        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<
                v1_dtl::decrementable_sentinel<D>::value &&
                v1_dtl::common_range<D>::value>>
        constexpr auto
        back() noexcept(noexcept(*std::prev(std::declval<D &>().end())))
            -> decltype(*std::prev(std::declval<D &>().end()))
        {
            return *std::prev(derived().end());
        }
        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<
                v1_dtl::decrementable_sentinel<D>::value &&
                v1_dtl::common_range<D>::value>>
        constexpr auto back() const
            noexcept(noexcept(*std::prev(std::declval<D const &>().end())))
                -> decltype(*std::prev(std::declval<D const &>().end()))
        {
            return *std::prev(derived().end());
        }

        template<typename D = Derived>
        constexpr auto operator[](v1_dtl::range_difference_t<D> n) noexcept(
            noexcept(std::declval<D &>().begin()[n]))
            -> decltype(std::declval<D &>().begin()[n])
        {
            return derived().begin()[n];
        }
        template<typename D = Derived>
        constexpr auto operator[](v1_dtl::range_difference_t<D> n) const
            noexcept(noexcept(std::declval<D const &>().begin()[n]))
                -> decltype(std::declval<D const &>().begin()[n])
        {
            return derived().begin()[n];
        }
    };

    /** Implementation of `operator!=()` for all views derived from
        `view_interface`. */
    template<typename ViewInterface>
    constexpr auto operator!=(ViewInterface lhs, ViewInterface rhs) noexcept(
        noexcept(lhs == rhs))
        -> decltype(v1_dtl::derived_view(lhs), !(lhs == rhs))
    {
        return !(lhs == rhs);
    }

}}}


#if defined(BOOST_STL_INTERFACES_DOXYGEN) || BOOST_STL_INTERFACES_USE_CONCEPTS

namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V2 {

    /** A template alias for `std::ranges::view_interface`.  This only exists
        to make migration from Boost.STLInterfaces to C++20 easier; switch to
        the one in `std` as soon as you can. */
    template<typename D, element_layout = element_layout::discontiguous>
    using view_interface = std::ranges::view_interface<D>;

}}}

#endif

#endif
