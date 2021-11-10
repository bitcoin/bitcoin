/*!
@file
Defines `boost::hana::optional`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_OPTIONAL_HPP
#define BOOST_HANA_OPTIONAL_HPP

#include <boost/hana/fwd/optional.hpp>

#include <boost/hana/bool.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/tag_of.hpp>
#include <boost/hana/detail/decay.hpp>
#include <boost/hana/detail/operators/adl.hpp>
#include <boost/hana/detail/operators/comparable.hpp>
#include <boost/hana/detail/operators/monad.hpp>
#include <boost/hana/detail/operators/orderable.hpp>
#include <boost/hana/detail/wrong.hpp>
#include <boost/hana/functional/partial.hpp>
#include <boost/hana/fwd/any_of.hpp>
#include <boost/hana/fwd/ap.hpp>
#include <boost/hana/fwd/concat.hpp>
#include <boost/hana/fwd/core/make.hpp>
#include <boost/hana/fwd/empty.hpp>
#include <boost/hana/fwd/equal.hpp>
#include <boost/hana/fwd/find_if.hpp>
#include <boost/hana/fwd/flatten.hpp>
#include <boost/hana/fwd/less.hpp>
#include <boost/hana/fwd/lift.hpp>
#include <boost/hana/fwd/transform.hpp>
#include <boost/hana/fwd/type.hpp>
#include <boost/hana/fwd/unpack.hpp>

#include <cstddef> // std::nullptr_t
#include <type_traits>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN
    //////////////////////////////////////////////////////////////////////////
    // optional<>
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename T, typename = typename hana::tag_of<T>::type>
        struct nested_type { };

        template <typename T>
        struct nested_type<T, type_tag> { using type = typename T::type; };
    }

    template <typename T>
    struct optional<T> : detail::operators::adl<>, detail::nested_type<T> {
        // 5.3.1, Constructors
        constexpr optional() = default;
        constexpr optional(optional const&) = default;
        constexpr optional(optional&&) = default;

        constexpr optional(T const& t)
            : value_(t)
        { }

        constexpr optional(T&& t)
            : value_(static_cast<T&&>(t))
        { }

        // 5.3.3, Assignment
        constexpr optional& operator=(optional const&) = default;
        constexpr optional& operator=(optional&&) = default;

        // 5.3.5, Observers
        constexpr T const* operator->() const { return &value_; }
        constexpr T* operator->() { return &value_; }

        constexpr T&        value() & { return value_; }
        constexpr T const&  value() const& { return value_; }
        constexpr T&&       value() && { return static_cast<T&&>(value_); }
        constexpr T const&& value() const&& { return static_cast<T const&&>(value_); }

        constexpr T&        operator*() & { return value_; }
        constexpr T const&  operator*() const& { return value_; }
        constexpr T&&       operator*() && { return static_cast<T&&>(value_); }
        constexpr T const&& operator*() const&& { return static_cast<T const&&>(value_); }

        template <typename U> constexpr T&        value_or(U&&) & { return value_; }
        template <typename U> constexpr T const&  value_or(U&&) const& { return value_; }
        template <typename U> constexpr T&&       value_or(U&&) && { return static_cast<T&&>(value_); }
        template <typename U> constexpr T const&& value_or(U&&) const&& { return static_cast<T const&&>(value_); }

        // We leave this public because it simplifies the implementation, but
        // this should be considered private by users.
        T value_;
    };

    //! @cond
    template <typename ...dummy>
    constexpr auto optional<>::value() const {
        static_assert(detail::wrong<dummy...>{},
        "hana::optional::value() requires a non-empty optional");
    }

    template <typename ...dummy>
    constexpr auto optional<>::operator*() const {
        static_assert(detail::wrong<dummy...>{},
        "hana::optional::operator* requires a non-empty optional");
    }

    template <typename U>
    constexpr U&& optional<>::value_or(U&& u) const {
        return static_cast<U&&>(u);
    }

    template <typename T>
    constexpr auto make_just_t::operator()(T&& t) const {
        return hana::optional<typename detail::decay<T>::type>(static_cast<T&&>(t));
    }
    //! @endcond

    template <typename ...T>
    struct tag_of<optional<T...>> {
        using type = optional_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // make<optional_tag>
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct make_impl<optional_tag> {
        template <typename X>
        static constexpr auto apply(X&& x)
        { return hana::just(static_cast<X&&>(x)); }

        static constexpr auto apply()
        { return hana::nothing; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Operators
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <>
        struct comparable_operators<optional_tag> {
            static constexpr bool value = true;
        };
        template <>
        struct orderable_operators<optional_tag> {
            static constexpr bool value = true;
        };
        template <>
        struct monad_operators<optional_tag> {
            static constexpr bool value = true;
        };
    }

    //////////////////////////////////////////////////////////////////////////
    // is_just and is_nothing
    //////////////////////////////////////////////////////////////////////////
    //! @cond
    template <typename ...T>
    constexpr auto is_just_t::operator()(optional<T...> const&) const
    { return hana::bool_c<sizeof...(T) != 0>; }

    template <typename ...T>
    constexpr auto is_nothing_t::operator()(optional<T...> const&) const
    { return hana::bool_c<sizeof...(T) == 0>; }
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // sfinae
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        struct sfinae_impl {
            template <typename F, typename ...X, typename = decltype(
                std::declval<F>()(std::declval<X>()...)
            )>
            constexpr decltype(auto) operator()(int, F&& f, X&& ...x) const {
                using Return = decltype(static_cast<F&&>(f)(static_cast<X&&>(x)...));
                static_assert(!std::is_same<Return, void>::value,
                "hana::sfinae(f)(args...) requires f(args...) to be non-void");

                return hana::just(static_cast<F&&>(f)(static_cast<X&&>(x)...));
            }

            template <typename F, typename ...X>
            constexpr auto operator()(long, F&&, X&& ...) const
            { return hana::nothing; }
        };
    }

    //! @cond
    template <typename F>
    constexpr decltype(auto) sfinae_t::operator()(F&& f) const {
        return hana::partial(detail::sfinae_impl{}, int{},
                             static_cast<F&&>(f));
    }
    //! @endcond

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<optional_tag, optional_tag> {
        template <typename T, typename U>
        static constexpr auto apply(hana::optional<T> const& t, hana::optional<U> const& u)
        { return hana::equal(t.value_, u.value_); }

        static constexpr hana::true_ apply(hana::optional<> const&, hana::optional<> const&)
        { return {}; }

        template <typename T, typename U>
        static constexpr hana::false_ apply(T const&, U const&)
        { return {}; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Orderable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct less_impl<optional_tag, optional_tag> {
        template <typename T>
        static constexpr hana::true_ apply(hana::optional<> const&, hana::optional<T> const&)
        { return {}; }

        static constexpr hana::false_ apply(hana::optional<> const&, hana::optional<> const&)
        { return {}; }

        template <typename T>
        static constexpr hana::false_ apply(hana::optional<T> const&, hana::optional<> const&)
        { return {}; }

        template <typename T, typename U>
        static constexpr auto apply(hana::optional<T> const& x, hana::optional<U> const& y)
        { return hana::less(x.value_, y.value_); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Functor
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct transform_impl<optional_tag> {
        template <typename F>
        static constexpr auto apply(optional<> const&, F&&)
        { return hana::nothing; }

        template <typename T, typename F>
        static constexpr auto apply(optional<T> const& opt, F&& f)
        { return hana::just(static_cast<F&&>(f)(opt.value_)); }

        template <typename T, typename F>
        static constexpr auto apply(optional<T>& opt, F&& f)
        { return hana::just(static_cast<F&&>(f)(opt.value_)); }

        template <typename T, typename F>
        static constexpr auto apply(optional<T>&& opt, F&& f)
        { return hana::just(static_cast<F&&>(f)(static_cast<T&&>(opt.value_))); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Applicative
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct lift_impl<optional_tag> {
        template <typename X>
        static constexpr auto apply(X&& x)
        { return hana::just(static_cast<X&&>(x)); }
    };

    template <>
    struct ap_impl<optional_tag> {
        template <typename F, typename X>
        static constexpr auto ap_helper(F&&, X&&, ...)
        { return hana::nothing; }

        template <typename F, typename X>
        static constexpr auto ap_helper(F&& f, X&& x, hana::true_, hana::true_)
        { return hana::just(static_cast<F&&>(f).value_(static_cast<X&&>(x).value_)); }

        template <typename F, typename X>
        static constexpr auto apply(F&& f, X&& x) {
            return ap_impl::ap_helper(static_cast<F&&>(f), static_cast<X&&>(x),
                                      hana::is_just(f), hana::is_just(x));
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Monad
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct flatten_impl<optional_tag> {
        static constexpr auto apply(optional<> const&)
        { return hana::nothing; }

        static constexpr auto apply(optional<optional<>> const&)
        { return hana::nothing; }

        template <typename T>
        static constexpr auto apply(optional<optional<T>> const& opt)
        { return hana::just(opt.value_.value_); }

        template <typename T>
        static constexpr auto apply(optional<optional<T>>&& opt)
        { return hana::just(static_cast<T&&>(opt.value_.value_)); }
    };

    //////////////////////////////////////////////////////////////////////////
    // MonadPlus
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct concat_impl<optional_tag> {
        template <typename Y>
        static constexpr auto apply(hana::optional<>&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename Y>
        static constexpr auto apply(hana::optional<>&&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename Y>
        static constexpr auto apply(hana::optional<> const&, Y&& y)
        { return static_cast<Y&&>(y); }

        template <typename X, typename Y>
        static constexpr auto apply(X&& x, Y&&)
        { return static_cast<X&&>(x); }
    };

    template <>
    struct empty_impl<optional_tag> {
        static constexpr auto apply()
        { return hana::nothing; }
    };

    //////////////////////////////////////////////////////////////////////////
    // Foldable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct unpack_impl<optional_tag> {
        template <typename T, typename F>
        static constexpr decltype(auto) apply(optional<T>&& opt, F&& f)
        { return static_cast<F&&>(f)(static_cast<T&&>(opt.value_)); }

        template <typename T, typename F>
        static constexpr decltype(auto) apply(optional<T> const& opt, F&& f)
        { return static_cast<F&&>(f)(opt.value_); }

        template <typename T, typename F>
        static constexpr decltype(auto) apply(optional<T>& opt, F&& f)
        { return static_cast<F&&>(f)(opt.value_); }

        template <typename F>
        static constexpr decltype(auto) apply(optional<> const&, F&& f)
        { return static_cast<F&&>(f)(); }
    };

    //////////////////////////////////////////////////////////////////////////
    // Searchable
    //////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <bool>
        struct optional_find_if {
            template <typename T>
            static constexpr auto apply(T const&)
            { return hana::nothing; }
        };

        template <>
        struct optional_find_if<true> {
            template <typename T>
            static constexpr auto apply(T&& t)
            { return hana::just(static_cast<T&&>(t)); }
        };
    }

    template <>
    struct find_if_impl<optional_tag> {
        template <typename T, typename Pred>
        static constexpr auto apply(hana::optional<T> const& opt, Pred&& pred) {
            constexpr bool found = decltype(static_cast<Pred&&>(pred)(opt.value_))::value;
            return detail::optional_find_if<found>::apply(opt.value_);
        }

        template <typename T, typename Pred>
        static constexpr auto apply(hana::optional<T>& opt, Pred&& pred) {
            constexpr bool found = decltype(static_cast<Pred&&>(pred)(opt.value_))::value;
            return detail::optional_find_if<found>::apply(opt.value_);
        }

        template <typename T, typename Pred>
        static constexpr auto apply(hana::optional<T>&& opt, Pred&& pred) {
            constexpr bool found = decltype(
                static_cast<Pred&&>(pred)(static_cast<T&&>(opt.value_))
            )::value;
            return detail::optional_find_if<found>::apply(static_cast<T&&>(opt.value_));
        }

        template <typename Pred>
        static constexpr auto apply(hana::optional<> const&, Pred&&)
        { return hana::nothing; }
    };

    template <>
    struct any_of_impl<optional_tag> {
        template <typename T, typename Pred>
        static constexpr auto apply(hana::optional<T> const& opt, Pred&& pred)
        { return static_cast<Pred&&>(pred)(opt.value_); }

        template <typename Pred>
        static constexpr hana::false_ apply(hana::optional<> const&, Pred&&)
        { return {}; }
    };
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_OPTIONAL_HPP
