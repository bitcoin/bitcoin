/*
@file
Defines `boost::hana::experimental::print`.

@copyright Louis Dionne 2013-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXPERIMENTAL_PRINTABLE_HPP
#define BOOST_HANA_EXPERIMENTAL_PRINTABLE_HPP

#include <boost/hana/concept/constant.hpp>
#include <boost/hana/concept/product.hpp>
#include <boost/hana/concept/sequence.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/core/to.hpp>
#include <boost/hana/core/dispatch.hpp>
#include <boost/hana/first.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/hana/intersperse.hpp>
#include <boost/hana/second.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/tuple.hpp>

// models for different containers
#include <boost/hana/fwd/map.hpp>
#include <boost/hana/fwd/optional.hpp>
#include <boost/hana/fwd/set.hpp>
#include <boost/hana/fwd/string.hpp>
#include <boost/hana/fwd/type.hpp>

#include <boost/core/demangle.hpp>

#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>


BOOST_HANA_NAMESPACE_BEGIN namespace experimental {
    template <typename T>
    struct Printable;

    //! @cond
    template <typename T, typename = void>
    struct print_impl : print_impl<T, hana::when<true>> { };

    template <typename T, bool condition>
    struct print_impl<T, hana::when<condition>> : hana::default_ {
        template <typename ...Args>
        static constexpr auto apply(Args&& ...) = delete;
    };
    //! @endcond

    //! @ingroup group-experimental
    //! Returns a string representation of the given object.
    //!
    //! This function is defined for most containers provided by Hana, and
    //! also for objects that define an `operator<<` that can be used with
    //! a `std::basic_ostream`. It can recursively print containers within
    //! containers, but do not expect any kind of proper indentation.
    //!
    //! This function requires (the rest of) Boost to be available on the
    //! system. It also requires RTTI to be enabled.
#ifdef BOOST_HANA_DOXYGEN_INVOKED
    auto print = [](auto const& x) -> std::string {
        return tag-dispatched;
    };
#else
    struct print_t {
        template <typename T>
        std::string operator()(T const& t) const {
            using Tag = typename hana::tag_of<T>::type;
            using Print = BOOST_HANA_DISPATCH_IF(print_impl<Tag>,
                hana::experimental::Printable<Tag>::value
            );

        #ifndef BOOST_HANA_CONFIG_DISABLE_CONCEPT_CHECKS
            static_assert(hana::experimental::Printable<Tag>::value,
            "hana::experimental::print(t) requires 't' to be Printable");
        #endif

            return Print::apply(t);
        }
    };

    BOOST_HANA_INLINE_VARIABLE constexpr print_t print{};
#endif

    // Define the `Printable` concept
    template <typename T>
    struct Printable {
        using Tag = typename hana::tag_of<T>::type;
        static constexpr bool value = !hana::is_default<print_impl<Tag>>::value;
    };

    namespace print_detail {
        inline std::string strip_type_junk(std::string const& str) {
            return std::regex_replace(str, std::regex("(?:struct )?([a-z_]+::)*([a-z_]*)_t<((?:struct )?[a-z:<>_]*)>"), "$2<$3>");
        }
    }

    // model for Sequences
    template <typename S>
    struct print_impl<S, hana::when<hana::Sequence<S>::value>> {
        template <typename Xs>
        static std::string apply(Xs const& xs) {
            std::string result = "(";
            auto comma_separated = hana::intersperse(xs, ", ");
            hana::for_each(comma_separated, [&result](auto const& x) {
                result += hana::experimental::print(x);
            });
            result += ")";
            return result;
        }
    };

    // model for OutputStreamable types
    //! @cond
    template <typename S>
    struct print_impl<S, hana::when_valid<decltype(
        std::declval<std::ostringstream&>() << std::declval<S const&>()
    )>> {
        template <typename T>
        static std::string apply(T const& t) {
            std::ostringstream os;
            os << t;
            return os.str();
        }
    };
    //! @endcond

    // model for hana::optional
    template <>
    struct print_impl<hana::optional_tag> {
        template <typename O>
        static std::string apply(O const& optional) {
            return hana::maybe("nothing",
                [](auto const& x) {
                    return "just(" + hana::experimental::print(x) + ")";
                }, optional);
        }
    };

    // model for hana::maps
    template <>
    struct print_impl<hana::map_tag> {
        template <typename M>
        static std::string apply(M const& map) {
            std::string result = "{";
            auto pairs = hana::transform(hana::to_tuple(map),
                [](auto const& pair) {
                    return hana::experimental::print(hana::first(pair))
                        + " => "
                        + hana::experimental::print(hana::second(pair));
                });
            auto comma_separated = hana::intersperse(pairs, ", ");
            hana::for_each(comma_separated, [&result](auto const& element) {
                result += element;
            });
            result += "}";
            return result;
        }
    };

    // model for hana::metafunctions
    template <template <typename ...> class F>
    struct print_impl<hana::metafunction_t<F>> {
        template <typename T>
        static std::string apply(T const&) {
            return print_detail::strip_type_junk(boost::core::demangle(typeid(T).name()));
        }
    };

    // model for hana::metafunction_classes
    template <typename F>
    struct print_impl<hana::metafunction_class_t<F>> {
        template <typename T>
        static std::string apply(T const&) {
            return print_detail::strip_type_junk(boost::core::demangle(typeid(T).name()));
        }
    };

    // model for Constants holding a `Printable`
    template <typename C>
    struct print_impl<C, hana::when<
        hana::Constant<C>::value &&
        Printable<typename C::value_type>::value
    >> {
        template <typename T>
        static std::string apply(T const&) {
            constexpr auto value = hana::value<T>();
            return hana::experimental::print(value);
        }
    };

    // model for Products
    template <typename P>
    struct print_impl<P, hana::when<hana::Product<P>::value>> {
        template <typename T>
        static std::string apply(T const& t) {
            return '(' + hana::experimental::print(hana::first(t))
                    + ", "
                    + hana::experimental::print(hana::second(t)) + ')';
        }
    };

    // model for hana::strings
    template <>
    struct print_impl<hana::string_tag> {
        template <typename S>
        static std::string apply(S const& s) {
            return '"' + std::string{hana::to<char const*>(s)} + '"';
        }
    };

    // model for hana::sets
    template <>
    struct print_impl<hana::set_tag> {
        template <typename S>
        static std::string apply(S const& set) {
            std::string result = "{";
            auto as_tuple = hana::transform(hana::to_tuple(set),
                                hana::experimental::print);
            auto comma_separated = hana::intersperse(as_tuple, ", ");
            hana::for_each(comma_separated, [&result](auto const& element) {
                result += element;
            });
            result += "}";
            return result;
        }
    };

    // model for hana::templates
    template <template <typename ...> class F>
    struct print_impl<template_t<F>> {
        template <typename T>
        static std::string apply(T const&) {
            return print_detail::strip_type_junk(boost::core::demangle(typeid(T).name()));
        }
    };

    // model for hana::types
    template <>
    struct print_impl<hana::type_tag> {
        template <typename T>
        static std::string apply(T const&) {
            using Type = typename T::type;
            return "type<" + boost::core::demangle(typeid(Type).name()) + '>';
        }
    };
} BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXPERIMENTAL_PRINTABLE_HPP
