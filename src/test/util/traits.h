// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TRAITS_H
#define BITCOIN_TEST_UTIL_TRAITS_H

#include <type_traits>

namespace traits {

/**
 * Determine if a class is a derived class of a template.  This is a specific
 * trait where the base template class has one type parameter: an unsigned int.
 * (This fits the case of `base_blob<>`, used for `uint256`);
 *
 * This metaprogramming magic adapted from excellent answer here:
 * https://stackoverflow.com/a/34672753/751579
 * Adapted by making it applicable to a base template class with one non-type
 * parameter.
 */
template <template <unsigned int> typename base, typename derived>
struct is_base_of_template_of_uint {
    template <unsigned int UI>
    static constexpr std::true_type test(const base<UI>*);
    static constexpr std::false_type test(...);
    using type = decltype(test(std::declval<derived*>()));
};

template <template <unsigned int> typename base, typename derived>
using is_base_of_template_of_uint_t = typename is_base_of_template_of_uint<base, derived>::type;

template <template <unsigned int> typename base, typename derived>
inline constexpr bool is_base_of_template_of_uint_v = is_base_of_template_of_uint<base, derived>::type::value;

/**
 * Trait `is_detected` from `std::experimental` for "library fundamentals TS v2"
 * See https://en.cppreference.com/w/cpp/experimental/is_detected for specification
 * and sample implementation, also https://stackoverflow.com/a/41936999/751579
 *
 * Namespace `detail_detected` consists of helpers for `is_detected`.
 */

namespace detail_detected {

template <class Default, class AlwaysVoid,
          template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

} // namespace detail_detected

template <template <class...> class Op, class... Args>
using is_detected = typename detail_detected::detector<detail_detected::nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
constexpr bool is_detected_v = is_detected<Op, Args...>::value;

// And here we have trait predicates that look for `begin()` and `end()`
template <class T>
using requires_has_begin = decltype(std::declval<T&>().begin());
template <class T>
using requires_has_end = decltype(std::declval<T&>().end());

/**
 * Trait (approximate) for a type that supports `begin()` and `end()
 *
 * (Approximate because it doesn't check return types are valid among other
 * flaws, e.g., not checking that theres a `begin(C)` or `end(C)` that can be
 * found via ADL.  But it's sufficient for our purposes here.  Anyway, will be
 * obsolete with C++20 and ranges.)
 */
template <class C>
using ok_for_range_based_for =
    std::enable_if_t<std::conjunction_v<is_detected<requires_has_begin, C>,
                                        is_detected<requires_has_end, C>>>;

} // namespace traits

#endif // BITCOIN_TEST_UTIL_TRAITS_H
