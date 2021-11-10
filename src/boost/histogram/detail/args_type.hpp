// Copyright 2015-2018 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_ARGS_TYPE_HPP
#define BOOST_HISTOGRAM_DETAIL_ARGS_TYPE_HPP

#include <tuple>

namespace boost {
namespace histogram {
namespace detail {

template <class T>
struct args_type_impl {
  using T::ERROR_this_should_never_be_instantiated_please_write_an_issue;
};

template <class R, class T, class... Ts>
struct args_type_impl<R (T::*)(Ts...)> {
  using type = std::tuple<Ts...>;
};

template <class R, class T, class... Ts>
struct args_type_impl<R (T ::*)(Ts...) const> {
  using type = std::tuple<Ts...>;
};

template <class R, class... Ts>
struct args_type_impl<R (*)(Ts...)> {
  using type = std::tuple<Ts...>;
};

#if __cpp_noexcept_function_type >= 201510
template <class R, class T, class... Ts>
struct args_type_impl<R (T::*)(Ts...) noexcept> {
  using type = std::tuple<Ts...>;
};

template <class R, class T, class... Ts>
struct args_type_impl<R (T ::*)(Ts...) const noexcept> {
  using type = std::tuple<Ts...>;
};

template <class R, class... Ts>
struct args_type_impl<R (*)(Ts...) noexcept> {
  using type = std::tuple<Ts...>;
};
#endif

template <class FunctionPointer>
using args_type = typename args_type_impl<FunctionPointer>::type;

template <class T, std::size_t N = 0>
using arg_type = std::tuple_element_t<N, args_type<T>>;

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
