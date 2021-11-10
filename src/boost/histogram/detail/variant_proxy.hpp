// Copyright 2019 Hans Dembinski
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_HISTOGRAM_DETAIL_VARIANT_PROXY_HPP
#define BOOST_HISTOGRAM_DETAIL_VARIANT_PROXY_HPP

#include <boost/core/nvp.hpp>
#include <boost/histogram/axis/traits.hpp> // variant_access
#include <boost/histogram/detail/static_if.hpp>
#include <boost/mp11/algorithm.hpp> // mp_with_index, mp_find, mp_at
#include <boost/mp11/list.hpp>      // mp_size
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace boost {
namespace histogram {
namespace detail {

// This is a workaround to remain backward compatible in the serialization format. The
// proxy uses only the public interface of axis::variant for serialization and works
// independently of the underlying variant implementation.
template <class Variant>
struct variant_proxy {
  Variant& variant;

  template <class Archive>
  void serialize(Archive& ar, unsigned /* version */) {
    detail::static_if_c<Archive::is_loading::value>(
        [this](auto& ar) { // loading
          int which = 0;
          ar >> make_nvp("which", which);
          constexpr unsigned N = mp11::mp_size<Variant>::value;
          if (which < 0 || static_cast<unsigned>(which) >= N)
            // throw if which >= N, can happen if type was removed from variant
            BOOST_THROW_EXCEPTION(
                std::runtime_error("variant has fewer types than stored version"));
          mp11::mp_with_index<N>(static_cast<unsigned>(which), [&ar, this](auto i) {
            using T = mp11::mp_at_c<Variant, i>;
            T value;
            ar >> make_nvp("value", value);
            this->variant = std::move(value);
            T* new_address = variant_access::template get_if<T>(&this->variant);
            ar.reset_object_address(new_address, &value);
          });
        },
        [this](auto& ar) { // saving
          visit(
              [&ar](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                const int which = static_cast<int>(mp11::mp_find<Variant, T>::value);
                ar << make_nvp("which", which);
                ar << make_nvp("value", value);
              },
              this->variant);
        },
        ar);
  }
};

} // namespace detail
} // namespace histogram
} // namespace boost

#endif
