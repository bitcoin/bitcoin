//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_DETAIL_TYPE_TRAITS_HPP
#define BOOST_BEAST_HTTP_DETAIL_TYPE_TRAITS_HPP

#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/optional.hpp>
#include <cstdint>

namespace boost {
namespace beast {
namespace http {

template<bool isRequest, class Fields>
class header;

template<bool, class, class>
class message;

template<bool isRequest, class Body, class Fields>
class parser;

namespace detail {

template<class T>
class is_header_impl
{
    template<bool b, class F>
    static std::true_type check(
        header<b, F> const*);
    static std::false_type check(...);
public:
    using type = decltype(check((T*)0));
};

template<class T>
using is_header = typename is_header_impl<T>::type;

template<class T>
struct is_parser : std::false_type {};

template<bool isRequest, class Body, class Fields>
struct is_parser<parser<isRequest, Body, Fields>> : std::true_type {};

struct fields_model
{
    struct writer;
    
    string_view method() const;
    string_view reason() const;
    string_view target() const;

protected:
    string_view get_method_impl() const;
    string_view get_target_impl() const;
    string_view get_reason_impl() const;
    bool get_chunked_impl() const;
    bool get_keep_alive_impl(unsigned) const;
    bool has_content_length_impl() const;
    void set_method_impl(string_view);
    void set_target_impl(string_view);
    void set_reason_impl(string_view);
    void set_chunked_impl(bool);
    void set_content_length_impl(boost::optional<std::uint64_t>);
    void set_keep_alive_impl(unsigned, bool);
};

template<class T, class = beast::detail::void_t<>>
struct has_value_type : std::false_type {};

template<class T>
struct has_value_type<T, beast::detail::void_t<
    typename T::value_type
        > > : std::true_type {};

/** Determine if a <em>Body</em> type has a size

    This metafunction is equivalent to `std::true_type` if
    Body contains a static member function called `size`.
*/
template<class T, class = void>
struct is_body_sized : std::false_type {};

template<class T>
struct is_body_sized<T, beast::detail::void_t<
    typename T::value_type,
        decltype(
    std::declval<std::uint64_t&>() =
        T::size(std::declval<typename T::value_type const&>())
    )>> : std::true_type {};

template<class T>
struct is_fields_helper : T
{
    template<class U = is_fields_helper>
    static auto f1(int) -> decltype(
        std::declval<string_view&>() = std::declval<U const&>().get_method_impl(),
        std::true_type());
    static auto f1(...) -> std::false_type;
    using t1 = decltype(f1(0));

    template<class U = is_fields_helper>
    static auto f2(int) -> decltype(
        std::declval<string_view&>() = std::declval<U const&>().get_target_impl(),
        std::true_type());
    static auto f2(...) -> std::false_type;
    using t2 = decltype(f2(0));

    template<class U = is_fields_helper>
    static auto f3(int) -> decltype(
        std::declval<string_view&>() = std::declval<U const&>().get_reason_impl(),
        std::true_type());
    static auto f3(...) -> std::false_type;
    using t3 = decltype(f3(0));

    template<class U = is_fields_helper>
    static auto f4(int) -> decltype(
        std::declval<bool&>() = std::declval<U const&>().get_chunked_impl(),
        std::true_type());
    static auto f4(...) -> std::false_type;
    using t4 = decltype(f4(0));

    template<class U = is_fields_helper>
    static auto f5(int) -> decltype(
        std::declval<bool&>() = std::declval<U const&>().get_keep_alive_impl(
            std::declval<unsigned>()),
        std::true_type());
    static auto f5(...) -> std::false_type;
    using t5 = decltype(f5(0));

    template<class U = is_fields_helper>
    static auto f6(int) -> decltype(
        std::declval<bool&>() = std::declval<U const&>().has_content_length_impl(),
        std::true_type());
    static auto f6(...) -> std::false_type;
    using t6 = decltype(f6(0));

    template<class U = is_fields_helper>
    static auto f7(int) -> decltype(
        void(std::declval<U&>().set_method_impl(std::declval<string_view>())),
        std::true_type());
    static auto f7(...) -> std::false_type;
    using t7 = decltype(f7(0));

    template<class U = is_fields_helper>
    static auto f8(int) -> decltype(
        void(std::declval<U&>().set_target_impl(std::declval<string_view>())),
        std::true_type());
    static auto f8(...) -> std::false_type;
    using t8 = decltype(f8(0));

    template<class U = is_fields_helper>
    static auto f9(int) -> decltype(
        void(std::declval<U&>().set_reason_impl(std::declval<string_view>())),
        std::true_type());
    static auto f9(...) -> std::false_type;
    using t9 = decltype(f9(0));

    template<class U = is_fields_helper>
    static auto f10(int) -> decltype(
        void(std::declval<U&>().set_chunked_impl(std::declval<bool>())),
        std::true_type());
    static auto f10(...) -> std::false_type;
    using t10 = decltype(f10(0));

    template<class U = is_fields_helper>
    static auto f11(int) -> decltype(
        void(std::declval<U&>().set_content_length_impl(
            std::declval<boost::optional<std::uint64_t>>())),
        std::true_type());
    static auto f11(...) -> std::false_type;
    using t11 = decltype(f11(0));

    template<class U = is_fields_helper>
    static auto f12(int) -> decltype(
        void(std::declval<U&>().set_keep_alive_impl(
            std::declval<unsigned>(),
            std::declval<bool>())),
        std::true_type());
    static auto f12(...) -> std::false_type;
    using t12 = decltype(f12(0));

    using type = std::integral_constant<bool,
         t1::value &&  t2::value && t3::value &&
         t4::value &&  t5::value && t6::value &&
         t7::value &&  t8::value && t9::value &&
        t10::value && t11::value && t12::value>;
};

} // detail
} // http
} // beast
} // boost

#endif
