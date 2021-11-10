//  Copyright Matt Borland 2021.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  https://en.cppreference.com/w/cpp/experimental/is_detected

#ifndef BOOST_MATH_TOOLS_IS_DETECTED_HPP
#define BOOST_MATH_TOOLS_IS_DETECTED_HPP

#include <type_traits>

namespace boost { namespace math { namespace tools {

template <typename...>
using void_t = void;

namespace detail {

template <typename Default, typename AlwaysVoid, template<typename...> class Op, typename... Args>
struct detector
{
    using value_t = std::false_type;
    using type = Default;
};

template <typename Default, template<typename...> class Op, typename... Args>
struct detector<Default, void_t<Op<Args...>>, Op, Args...>
{
    using value_t = std::true_type;
    using type = Op<Args...>;
};

} // Namespace detail

// Special type to indicate detection failure
struct nonesuch
{
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    void operator=(const nonesuch&) = delete;
};

template <template<typename...> class Op, typename... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template<typename...> class Op, typename... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <typename Default, template<typename...> class Op, typename... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

}}} // Namespaces boost math tools

#endif // BOOST_MATH_TOOLS_IS_DETECTED_HPP
