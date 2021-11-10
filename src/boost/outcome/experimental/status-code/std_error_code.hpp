/* Proposed SG14 status_code
(C) 2018 - 2021 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Aug 2018


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

#ifndef BOOST_OUTCOME_SYSTEM_ERROR2_STD_ERROR_CODE_HPP
#define BOOST_OUTCOME_SYSTEM_ERROR2_STD_ERROR_CODE_HPP

#ifndef BOOST_OUTCOME_SYSTEM_ERROR2_NOT_POSIX
#include "posix_code.hpp"
#endif

#if defined(_WIN32) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
#include "win32_code.hpp"
#endif

#include <system_error>

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN

class _std_error_code_domain;
//! A `status_code` representing exactly a `std::error_code`
using std_error_code = status_code<_std_error_code_domain>;

namespace mixins
{
  template <class Base> struct mixin<Base, _std_error_code_domain> : public Base
  {
    using Base::Base;

    //! Implicit constructor from a `std::error_code`
    inline mixin(std::error_code ec);

    //! Returns the error code category
    inline const std::error_category &category() const noexcept;
  };
}  // namespace mixins


/*! The implementation of the domain for `std::error_code` error codes.
 */
class _std_error_code_domain final : public status_code_domain
{
  template <class DomainType> friend class status_code;
  template <class StatusCode> friend class detail::indirecting_domain;
  using _base = status_code_domain;
  using _error_code_type = std::error_code;
  using _error_category_type = std::error_category;

  std::string _name;

  static _base::string_ref _make_string_ref(_error_code_type c) noexcept
  {
    try
    {
      std::string msg = c.message();
      auto *p = static_cast<char *>(malloc(msg.size() + 1));  // NOLINT
      if(p == nullptr)
      {
        return _base::string_ref("failed to allocate message");
      }
      memcpy(p, msg.c_str(), msg.size() + 1);
      return _base::atomic_refcounted_string_ref(p, msg.size());
    }
    catch(...)
    {
      return _base::string_ref("failed to allocate message");
    }
  }

public:
  //! The value type of the `std::error_code` code, which stores the `int` from the `std::error_code`
  using value_type = int;
  using _base::string_ref;

  //! Returns the error category singleton pointer this status code domain represents
  const _error_category_type &error_category() const noexcept
  {
    auto ptr = 0x223a160d20de97b4 ^ this->id();
    return *reinterpret_cast<const _error_category_type *>(ptr);
  }

  //! Default constructor
  explicit _std_error_code_domain(const _error_category_type &category) noexcept
      : _base(0x223a160d20de97b4 ^ reinterpret_cast<_base::unique_id_type>(&category))
      , _name("std_error_code_domain(")
  {
    _name.append(category.name());
    _name.push_back(')');
  }
  _std_error_code_domain(const _std_error_code_domain &) = default;
  _std_error_code_domain(_std_error_code_domain &&) = default;
  _std_error_code_domain &operator=(const _std_error_code_domain &) = default;
  _std_error_code_domain &operator=(_std_error_code_domain &&) = default;
  ~_std_error_code_domain() = default;

  static inline const _std_error_code_domain *get(_error_code_type ec);

  virtual string_ref name() const noexcept override { return string_ref(_name.c_str(), _name.size()); }  // NOLINT
protected:
  virtual bool _do_failure(const status_code<void> &code) const noexcept override;
  virtual bool _do_equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept override;
  virtual generic_code _generic_code(const status_code<void> &code) const noexcept override;
  virtual string_ref _do_message(const status_code<void> &code) const noexcept override;
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
  BOOST_OUTCOME_SYSTEM_ERROR2_NORETURN virtual void _do_throw_exception(const status_code<void> &code) const override;
#endif
};

namespace detail
{
  extern inline _std_error_code_domain *std_error_code_domain_from_category(const std::error_category &category)
  {
    static constexpr size_t max_items = 64;
    static struct storage_t
    {
      std::atomic<unsigned> _lock;
      union item_t
      {
        int _init;
        _std_error_code_domain domain;
        constexpr item_t()
            : _init(0)
        {
        }
        ~item_t() {}
      } items[max_items];
      size_t count{0};

      void lock()
      {
        while(_lock.exchange(1, std::memory_order_acquire) != 0)
          ;
      }
      void unlock() { _lock.store(0, std::memory_order_release); }

      storage_t() {}
      ~storage_t()
      {
        lock();
        for(size_t n = 0; n < count; n++)
        {
          items[n].domain.~_std_error_code_domain();
        }
        unlock();
      }
      _std_error_code_domain *add(const std::error_category &category)
      {
        _std_error_code_domain *ret = nullptr;
        lock();
        for(size_t n = 0; n < count; n++)
        {
          if(items[n].domain.error_category() == category)
          {
            ret = &items[n].domain;
            break;
          }
        }
        if(ret == nullptr && count < max_items)
        {
          ret = new(&items[count++].domain) _std_error_code_domain(category);
        }
        unlock();
        return ret;
      }
    } storage;
    return storage.add(category);
  }
}  // namespace detail

namespace mixins
{
  template <class Base>
  inline mixin<Base, _std_error_code_domain>::mixin(std::error_code ec)
      : Base(typename Base::_value_type_constructor{}, _std_error_code_domain::get(ec), ec.value())
  {
  }

  template <class Base> inline const std::error_category &mixin<Base, _std_error_code_domain>::category() const noexcept
  {
    const auto &domain = static_cast<const _std_error_code_domain &>(this->domain());
    return domain.error_category();
  };
}  // namespace mixins

inline const _std_error_code_domain *_std_error_code_domain::get(std::error_code ec)
{
  auto *p = detail::std_error_code_domain_from_category(ec.category());
  assert(p != nullptr);
  if(p == nullptr)
  {
    abort();
  }
  return p;
}


inline bool _std_error_code_domain::_do_failure(const status_code<void> &code) const noexcept
{
  assert(code.domain() == *this);
  return static_cast<const std_error_code &>(code).value() != 0;  // NOLINT
}

inline bool _std_error_code_domain::_do_equivalent(const status_code<void> &code1, const status_code<void> &code2) const noexcept
{
  assert(code1.domain() == *this);
  const auto &c1 = static_cast<const std_error_code &>(code1);  // NOLINT
  const auto &cat1 = c1.category();
  // Are we comparing to another wrapped error_code?
  if(code2.domain() == *this)
  {
    const auto &c2 = static_cast<const std_error_code &>(code2);  // NOLINT
    const auto &cat2 = c2.category();
    // If the error code categories are identical, do literal comparison
    if(cat1 == cat2)
    {
      return c1.value() == c2.value();
    }
    // Otherwise fall back onto the _generic_code comparison, which uses default_error_condition()
    return false;
  }
  // Am I an error code with generic category?
  if(cat1 == std::generic_category())
  {
    // Convert to generic code, and compare that
    generic_code _c1(static_cast<errc>(c1.value()));
    return _c1 == code2;
  }
  // Am I an error code with system category?
  if(cat1 == std::system_category())
  {
// Convert to POSIX or Win32 code, and compare that
#ifdef _WIN32
    win32_code _c1((win32::DWORD) c1.value());
    return _c1 == code2;
#elif !defined(BOOST_OUTCOME_SYSTEM_ERROR2_NOT_POSIX)
    posix_code _c1(c1.value());
    return _c1 == code2;
#endif
  }
  return false;
}

inline generic_code _std_error_code_domain::_generic_code(const status_code<void> &code) const noexcept
{
  assert(code.domain() == *this);
  const auto &c = static_cast<const std_error_code &>(code);  // NOLINT
  // Ask my embedded error code for its mapping to std::errc, which is a subset of our generic_code errc.
  std::error_condition cond(c.category().default_error_condition(c.value()));
  if(cond.category() == std::generic_category())
  {
    return generic_code(static_cast<errc>(cond.value()));
  }
#if !defined(BOOST_OUTCOME_SYSTEM_ERROR2_NOT_POSIX) && !defined(_WIN32)
  if(cond.category() == std::system_category())
  {
    return generic_code(static_cast<errc>(cond.value()));
  }
#endif
  return errc::unknown;
}

inline _std_error_code_domain::string_ref _std_error_code_domain::_do_message(const status_code<void> &code) const noexcept
{
  assert(code.domain() == *this);
  const auto &c = static_cast<const std_error_code &>(code);  // NOLINT
  return _make_string_ref(_error_code_type(c.value(), c.category()));
}

#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
BOOST_OUTCOME_SYSTEM_ERROR2_NORETURN inline void _std_error_code_domain::_do_throw_exception(const status_code<void> &code) const
{
  assert(code.domain() == *this);
  const auto &c = static_cast<const std_error_code &>(code);  // NOLINT
  throw std::system_error(std::error_code(c.value(), c.category()));
}
#endif

static_assert(sizeof(std_error_code) <= sizeof(void *) * 2, "std_error_code does not fit into a system_code!");

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END

// Enable implicit construction of `std_error_code` from `std::error_code`.
namespace std
{
  inline BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE::std_error_code make_status_code(error_code c) noexcept { return BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE::std_error_code(c); }
}  // namespace std

#endif
