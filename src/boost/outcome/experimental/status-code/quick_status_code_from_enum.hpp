/* Proposed SG14 status_code
(C) 2018 - 2020 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: May 2020


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
(See accompanying file Licence.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_OUTCOME_SYSTEM_ERROR2_QUICK_STATUS_CODE_FROM_ENUM_HPP
#define BOOST_OUTCOME_SYSTEM_ERROR2_QUICK_STATUS_CODE_FROM_ENUM_HPP

#include "generic_code.hpp"

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN

template <class Enum> class _quick_status_code_from_enum_domain;
//! A status code wrapping `Enum` generated from `quick_status_code_from_enum`.
template <class Enum> using quick_status_code_from_enum_code = status_code<_quick_status_code_from_enum_domain<Enum>>;

//! Defaults for an implementation of `quick_status_code_from_enum<Enum>`
template <class Enum> struct quick_status_code_from_enum_defaults
{
  //! The type of the resulting code
  using code_type = quick_status_code_from_enum_code<Enum>;
  //! Used within `quick_status_code_from_enum` to define a mapping of enumeration value with its status code
  struct mapping
  {
    //! The enumeration type
    using enumeration_type = Enum;

    //! The value being mapped
    const Enum value;
    //! A string representation for this enumeration value
    const char *message;
    //! A list of `errc` equivalents for this enumeration value
    const std::initializer_list<errc> code_mappings;
  };
  //! Used within `quick_status_code_from_enum` to define mixins for the status code wrapping `Enum`
  template <class Base> struct mixin : Base
  {
    using Base::Base;
  };
};

/*! The implementation of the domain for status codes wrapping `Enum` generated from `quick_status_code_from_enum`.
 */
template <class Enum> class _quick_status_code_from_enum_domain : public status_code_domain
{
  template <class DomainType> friend class status_code;
  template <class StatusCode> friend class detail::indirecting_domain;
  using _base = status_code_domain;
  using _src = quick_status_code_from_enum<Enum>;

public:
  //! The value type of the quick status code from enum
  using value_type = Enum;
  using _base::string_ref;

  constexpr _quick_status_code_from_enum_domain()
      : status_code_domain(_src::domain_uuid, _uuid_size<detail::cstrlen(_src::domain_uuid)>())
  {
  }
  _quick_status_code_from_enum_domain(const _quick_status_code_from_enum_domain &) = default;
  _quick_status_code_from_enum_domain(_quick_status_code_from_enum_domain &&) = default;
  _quick_status_code_from_enum_domain &operator=(const _quick_status_code_from_enum_domain &) = default;
  _quick_status_code_from_enum_domain &operator=(_quick_status_code_from_enum_domain &&) = default;
  ~_quick_status_code_from_enum_domain() = default;

#if __cplusplus < 201402L && !defined(_MSC_VER)
  static inline const _quick_status_code_from_enum_domain &get()
  {
    static _quick_status_code_from_enum_domain v;
    return v;
  }
#else
  static inline constexpr const _quick_status_code_from_enum_domain &get();
#endif

  virtual string_ref name() const noexcept override { return string_ref(_src::domain_name); }

protected:
  // Not sure if a hash table is worth it here, most enumerations won't be long enough to be worth it
  // Also, until C++ 20's consteval, the hash table would get emitted into the binary, bloating it
  static BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 const typename _src::mapping *_find_mapping(value_type v) noexcept
  {
    for(const auto &i : _src::value_mappings())
    {
      if(i.value == v)
      {
        return &i;
      }
    }
    return nullptr;
  }

  virtual bool _do_failure(const status_code<void> &code) const noexcept override
  {
    assert(code.domain() == *this);  // NOLINT
    // If `errc::success` is in the generic code mapping, it is not a failure
    const auto *mapping = _find_mapping(static_cast<const quick_status_code_from_enum_code<value_type> &>(code).value());
    assert(mapping != nullptr);
    if(mapping != nullptr)
    {
      for(errc ec : mapping->code_mappings)
      {
        if(ec == errc::success)
        {
          return false;
        }
      }
    }
    return true;
  }
  virtual bool _do_equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept override
  {
    assert(code1.domain() == *this);                                                                   // NOLINT
    const auto &c1 = static_cast<const quick_status_code_from_enum_code<value_type> &>(code1);  // NOLINT
    if(code2.domain() == *this)
    {
      const auto &c2 = static_cast<const quick_status_code_from_enum_code<value_type> &>(code2);  // NOLINT
      return c1.value() == c2.value();
    }
    if(code2.domain() == generic_code_domain)
    {
      const auto &c2 = static_cast<const generic_code &>(code2);  // NOLINT
      const auto *mapping = _find_mapping(c1.value());
      assert(mapping != nullptr);
      if(mapping != nullptr)
      {
        for(errc ec : mapping->code_mappings)
        {
          if(ec == c2.value())
          {
            return true;
          }
        }
      }
    }
    return false;
  }
  virtual generic_code _generic_code(const status_code<void> &code) const noexcept override
  {
    assert(code.domain() == *this);  // NOLINT
    const auto *mapping = _find_mapping(static_cast<const quick_status_code_from_enum_code<value_type> &>(code).value());
    assert(mapping != nullptr);
    if(mapping != nullptr)
    {
      if(mapping->code_mappings.size() > 0)
      {
        return *mapping->code_mappings.begin();
      }
    }
    return errc::unknown;
  }
  virtual string_ref _do_message(const status_code<void> &code) const noexcept override
  {
    assert(code.domain() == *this);  // NOLINT
    const auto *mapping = _find_mapping(static_cast<const quick_status_code_from_enum_code<value_type> &>(code).value());
    assert(mapping != nullptr);
    if(mapping != nullptr)
    {
      return string_ref(mapping->message);
    }
    return string_ref("unknown");
  }
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
  BOOST_OUTCOME_SYSTEM_ERROR2_NORETURN virtual void _do_throw_exception(const status_code<void> &code) const override
  {
    assert(code.domain() == *this);                                                                  // NOLINT
    const auto &c = static_cast<const quick_status_code_from_enum_code<value_type> &>(code);  // NOLINT
    throw status_error<_quick_status_code_from_enum_domain>(c);
  }
#endif
};

#if __cplusplus >= 201402L || defined(_MSC_VER)
template <class Enum> constexpr _quick_status_code_from_enum_domain<Enum> quick_status_code_from_enum_domain = {};
template <class Enum> inline constexpr const _quick_status_code_from_enum_domain<Enum> &_quick_status_code_from_enum_domain<Enum>::get()
{
  return quick_status_code_from_enum_domain<Enum>;
}
#endif

namespace mixins
{
  template <class Base, class Enum> struct mixin<Base, _quick_status_code_from_enum_domain<Enum>> : public quick_status_code_from_enum<Enum>::template mixin<Base>
  {
    using quick_status_code_from_enum<Enum>::template mixin<Base>::mixin;
  };
}  // namespace mixins

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END

#endif
