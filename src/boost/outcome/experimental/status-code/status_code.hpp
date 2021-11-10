/* Proposed SG14 status_code
(C) 2018 - 2020 Niall Douglas <http://www.nedproductions.biz/> (5 commits)
File Created: Feb 2018


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

#ifndef BOOST_OUTCOME_SYSTEM_ERROR2_STATUS_CODE_HPP
#define BOOST_OUTCOME_SYSTEM_ERROR2_STATUS_CODE_HPP

#include "status_code_domain.hpp"

#if(__cplusplus >= 201700 || _HAS_CXX17) && !defined(BOOST_OUTCOME_SYSTEM_ERROR2_DISABLE_STD_IN_PLACE)
// 0.26
#include <utility>  // for in_place

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN
using in_place_t = std::in_place_t;
using std::in_place;
BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END

#else

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN
//! Aliases `std::in_place_t` if on C++ 17 or later, else defined locally.
struct in_place_t
{
  explicit in_place_t() = default;
};
//! Aliases `std::in_place` if on C++ 17 or later, else defined locally.
constexpr in_place_t in_place{};
BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END
#endif

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN

//! Namespace for user injected mixins
namespace mixins
{
  template <class Base, class T> struct mixin : public Base
  {
    using Base::Base;
  };
}  // namespace mixins

/*! A tag for an erased value type for `status_code<D>`.
Available only if `ErasedType` satisfies `traits::is_move_bitcopying<ErasedType>::value`.
*/
template <class ErasedType,  //
          typename std::enable_if<traits::is_move_bitcopying<ErasedType>::value, bool>::type = true>
struct erased
{
  using value_type = ErasedType;
};

/*! Specialise this template to quickly wrap a third party enumeration into a
custom status code domain.

Use like this:

```c++
BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_BEGIN
template <> struct quick_status_code_from_enum<AnotherCode> : quick_status_code_from_enum_defaults<AnotherCode>
{
  // Text name of the enum
  static constexpr const auto domain_name = "Another Code";
  // Unique UUID for the enum. PLEASE use https://www.random.org/cgi-bin/randbyte?nbytes=16&format=h
  static constexpr const auto domain_uuid = "{be201f65-3962-dd0e-1266-a72e63776a42}";
  // Map of each enum value to its text string, and list of semantically equivalent errc's
  static const std::initializer_list<mapping> &value_mappings()
  {
    static const std::initializer_list<mapping<AnotherCode>> v = {
    // Format is: { enum value, "string representation", { list of errc mappings ... } }
    {AnotherCode::success1, "Success 1", {errc::success}},        //
    {AnotherCode::goaway, "Go away", {errc::permission_denied}},  //
    {AnotherCode::success2, "Success 2", {errc::success}},        //
    {AnotherCode::error2, "Error 2", {}},                         //
    };
    return v;
  }
  // Completely optional definition of mixin for the status code synthesised from `Enum`. It can be omitted.
  template <class Base> struct mixin : Base
  {
    using Base::Base;
    constexpr int custom_method() const { return 42; }
  };
};
BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END
```

Note that if the `errc` mapping contains `errc::success`, then
the enumeration value is considered to be a successful value.
Otherwise it is considered to be a failure value.

The first value in the `errc` mapping is the one chosen as the
`generic_code` conversion. Other values are used during equivalence
comparisons.
*/
template <class Enum> struct quick_status_code_from_enum;

namespace detail
{
  template <class T> struct is_status_code
  {
    static constexpr bool value = false;
  };
  template <class T> struct is_status_code<status_code<T>>
  {
    static constexpr bool value = true;
  };
  template <class T> struct is_erased_status_code
  {
    static constexpr bool value = false;
  };
  template <class T> struct is_erased_status_code<status_code<erased<T>>>
  {
    static constexpr bool value = true;
  };

  // From http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4436.pdf
  namespace impl
  {
    template <typename... Ts> struct make_void
    {
      using type = void;
    };
    template <typename... Ts> using void_t = typename make_void<Ts...>::type;
    template <class...> struct types
    {
      using type = types;
    };
    template <template <class...> class T, class types, class = void> struct test_apply
    {
      using type = void;
    };
    template <template <class...> class T, class... Ts> struct test_apply<T, types<Ts...>, void_t<T<Ts...>>>
    {
      using type = T<Ts...>;
    };
  }  // namespace impl
  template <template <class...> class T, class... Ts> using test_apply = impl::test_apply<T, impl::types<Ts...>>;

  template <class T, class... Args> using get_make_status_code_result = decltype(make_status_code(std::declval<T>(), std::declval<Args>()...));
  template <class... Args> using safe_get_make_status_code_result = test_apply<get_make_status_code_result, Args...>;
}  // namespace detail

//! Trait returning true if the type is a status code.
template <class T> struct is_status_code
{
  static constexpr bool value = detail::is_status_code<typename std::decay<T>::type>::value || detail::is_erased_status_code<typename std::decay<T>::type>::value;
};

/*! A type erased lightweight status code reflecting empty, success, or failure.
Differs from `status_code<erased<>>` by being always available irrespective of
the domain's value type, but cannot be copied, moved, nor destructed. Thus one
always passes this around by const lvalue reference.
*/
template <> class BOOST_OUTCOME_SYSTEM_ERROR2_TRIVIAL_ABI status_code<void>
{
  template <class T> friend class status_code;

public:
  //! The type of the domain.
  using domain_type = void;
  //! The type of the status code.
  using value_type = void;
  //! The type of a reference to a message string.
  using string_ref = typename status_code_domain::string_ref;

protected:
  const status_code_domain *_domain{nullptr};

protected:
  //! No default construction at type erased level
  status_code() = default;
  //! No public copying at type erased level
  status_code(const status_code &) = default;
  //! No public moving at type erased level
  status_code(status_code &&) = default;
  //! No public assignment at type erased level
  status_code &operator=(const status_code &) = default;
  //! No public assignment at type erased level
  status_code &operator=(status_code &&) = default;
  //! No public destruction at type erased level
  ~status_code() = default;

  //! Used to construct a non-empty type erased status code
  constexpr explicit status_code(const status_code_domain *v) noexcept
      : _domain(v)
  {
  }

  // Used to work around triggering a ubsan failure. Do NOT remove!
  constexpr const status_code_domain *_domain_ptr() const noexcept { return _domain; }

public:
  //! Return the status code domain.
  constexpr const status_code_domain &domain() const noexcept { return *_domain; }
  //! True if the status code is empty.
  BOOST_OUTCOME_SYSTEM_ERROR2_NODISCARD constexpr bool empty() const noexcept { return _domain == nullptr; }

  //! Return a reference to a string textually representing a code.
  string_ref message() const noexcept
  {
    // Avoid MSVC's buggy ternary operator for expensive to destruct things
    if(_domain != nullptr)
    {
      return _domain->_do_message(*this);
    }
    return string_ref("(empty)");
  }
  //! True if code means success.
  bool success() const noexcept { return (_domain != nullptr) ? !_domain->_do_failure(*this) : false; }
  //! True if code means failure.
  bool failure() const noexcept { return (_domain != nullptr) ? _domain->_do_failure(*this) : false; }
  /*! True if code is strictly (and potentially non-transitively) semantically equivalent to another code in another domain.
  Note that usually non-semantic i.e. pure value comparison is used when the other status code has the same domain.
  As `equivalent()` will try mapping to generic code, this usually captures when two codes have the same semantic
  meaning in `equivalent()`.
  */
  template <class T> bool strictly_equivalent(const status_code<T> &o) const noexcept
  {
    if(_domain && o._domain)
    {
      return _domain->_do_equivalent(*this, o);
    }
    // If we are both empty, we are equivalent
    if(!_domain && !o._domain)
    {
      return true;  // NOLINT
    }
    // Otherwise not equivalent
    return false;
  }
  /*! True if code is equivalent, by any means, to another code in another domain (guaranteed transitive).
  Firstly `strictly_equivalent()` is run in both directions. If neither succeeds, each domain is asked
  for the equivalent generic code and those are compared.
  */
  template <class T> inline bool equivalent(const status_code<T> &o) const noexcept;
#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(BOOST_OUTCOME_STANDARDESE_IS_IN_THE_HOUSE)
  //! Throw a code as a C++ exception.
  BOOST_OUTCOME_SYSTEM_ERROR2_NORETURN void throw_exception() const
  {
    _domain->_do_throw_exception(*this);
    abort();  // suppress buggy GCC warning
  }
#endif
};

namespace detail
{
  template <class DomainType> struct get_domain_value_type
  {
    using domain_type = DomainType;
    using value_type = typename domain_type::value_type;
  };
  template <class ErasedType> struct get_domain_value_type<erased<ErasedType>>
  {
    using domain_type = status_code_domain;
    using value_type = ErasedType;
  };
  template <class DomainType> class BOOST_OUTCOME_SYSTEM_ERROR2_TRIVIAL_ABI status_code_storage : public status_code<void>
  {
    using _base = status_code<void>;

  public:
    //! The type of the domain.
    using domain_type = typename get_domain_value_type<DomainType>::domain_type;
    //! The type of the status code.
    using value_type = typename get_domain_value_type<DomainType>::value_type;
    //! The type of a reference to a message string.
    using string_ref = typename domain_type::string_ref;

#ifndef NDEBUG
    static_assert(std::is_move_constructible<value_type>::value || std::is_copy_constructible<value_type>::value, "DomainType::value_type is neither move nor copy constructible!");
    static_assert(!std::is_default_constructible<value_type>::value || std::is_nothrow_default_constructible<value_type>::value, "DomainType::value_type is not nothrow default constructible!");
    static_assert(!std::is_move_constructible<value_type>::value || std::is_nothrow_move_constructible<value_type>::value, "DomainType::value_type is not nothrow move constructible!");
    static_assert(std::is_nothrow_destructible<value_type>::value, "DomainType::value_type is not nothrow destructible!");
#endif

    // Replace the type erased implementations with type aware implementations for better codegen
    //! Return the status code domain.
    constexpr const domain_type &domain() const noexcept { return *static_cast<const domain_type *>(this->_domain); }

    //! Reset the code to empty.
    BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 void clear() noexcept
    {
      this->_value.~value_type();
      this->_domain = nullptr;
      new(&this->_value) value_type();
    }

#if __cplusplus >= 201400 || _MSC_VER >= 1910 /* VS2017 */
    //! Return a reference to the `value_type`.
    constexpr value_type &value() &noexcept { return this->_value; }
    //! Return a reference to the `value_type`.
    constexpr value_type &&value() &&noexcept { return static_cast<value_type &&>(this->_value); }
#endif
    //! Return a reference to the `value_type`.
    constexpr const value_type &value() const &noexcept { return this->_value; }
    //! Return a reference to the `value_type`.
    constexpr const value_type &&value() const &&noexcept { return static_cast<const value_type &&>(this->_value); }

  protected:
    status_code_storage() = default;
    status_code_storage(const status_code_storage &) = default;
    BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 status_code_storage(status_code_storage &&o) noexcept
        : _base(static_cast<status_code_storage &&>(o))
        , _value(static_cast<status_code_storage &&>(o)._value)
    {
      o._domain = nullptr;
    }
    status_code_storage &operator=(const status_code_storage &) = default;
    BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 status_code_storage &operator=(status_code_storage &&o) noexcept
    {
      this->~status_code_storage();
      new(this) status_code_storage(static_cast<status_code_storage &&>(o));
      return *this;
    }
    ~status_code_storage() = default;

    value_type _value{};
    struct _value_type_constructor
    {
    };
    template <class... Args>
    constexpr status_code_storage(_value_type_constructor /*unused*/, const status_code_domain *v, Args &&... args)
        : _base(v)
        , _value(static_cast<Args &&>(args)...)
    {
    }
  };
}  // namespace detail

/*! A lightweight, typed, status code reflecting empty, success, or failure.
This is the main workhorse of the system_error2 library. Its characteristics reflect the value type
set by its domain type, so if that value type is move-only or trivial, so is this.

An ADL discovered helper function `make_status_code(T, Args...)` is looked up by one of the constructors.
If it is found, and it generates a status code compatible with this status code, implicit construction
is made available.

You may mix in custom member functions and member function overrides by injecting a specialisation of
`mixins::mixin<Base, YourDomainType>`. Your mixin must inherit from `Base`.
*/
template <class DomainType> class BOOST_OUTCOME_SYSTEM_ERROR2_TRIVIAL_ABI status_code : public mixins::mixin<detail::status_code_storage<DomainType>, DomainType>
{
  template <class T> friend class status_code;
  using _base = mixins::mixin<detail::status_code_storage<DomainType>, DomainType>;

public:
  //! The type of the domain.
  using domain_type = DomainType;
  //! The type of the status code.
  using value_type = typename domain_type::value_type;
  //! The type of a reference to a message string.
  using string_ref = typename domain_type::string_ref;

protected:
  using _base::_base;

public:
  //! Default construction to empty
  status_code() = default;
  //! Copy constructor
  status_code(const status_code &) = default;
  //! Move constructor
  status_code(status_code &&) = default;  // NOLINT
  //! Copy assignment
  status_code &operator=(const status_code &) = default;
  //! Move assignment
  status_code &operator=(status_code &&) = default;  // NOLINT
  ~status_code() = default;

  //! Return a copy of the code.
  BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 status_code clone() const { return *this; }

  /***** KEEP THESE IN SYNC WITH ERRORED_STATUS_CODE *****/
  //! Implicit construction from any type where an ADL discovered `make_status_code(T, Args ...)` returns a `status_code`.
  template <class T, class... Args,                                                                            //
            class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<T, Args...>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
            typename std::enable_if<!std::is_same<typename std::decay<T>::type, status_code>::value            // not copy/move of self
                                    && !std::is_same<typename std::decay<T>::type, in_place_t>::value          // not in_place_t
                                    && is_status_code<MakeStatusCodeResult>::value                             // ADL makes a status code
                                    && std::is_constructible<status_code, MakeStatusCodeResult>::value,        // ADLed status code is compatible

                                    bool>::type = true>
  constexpr status_code(T &&v, Args &&... args) noexcept(noexcept(make_status_code(std::declval<T>(), std::declval<Args>()...)))  // NOLINT
      : status_code(make_status_code(static_cast<T &&>(v), static_cast<Args &&>(args)...))
  {
  }
  //! Implicit construction from any `quick_status_code_from_enum<Enum>` enumerated type.
  template <class Enum,                                                                              //
            class QuickStatusCodeType = typename quick_status_code_from_enum<Enum>::code_type,       // Enumeration has been activated
            typename std::enable_if<std::is_constructible<status_code, QuickStatusCodeType>::value,  // Its status code is compatible

                                    bool>::type = true>
  constexpr status_code(Enum &&v) noexcept(std::is_nothrow_constructible<status_code, QuickStatusCodeType>::value)  // NOLINT
      : status_code(QuickStatusCodeType(static_cast<Enum &&>(v)))
  {
  }
  //! Explicit in-place construction. Disables if `domain_type::get()` is not a valid expression.
  template <class... Args>
  constexpr explicit status_code(in_place_t /*unused */, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, Args &&...>::value)
      : _base(typename _base::_value_type_constructor{}, &domain_type::get(), static_cast<Args &&>(args)...)
  {
  }
  //! Explicit in-place construction from initialiser list. Disables if `domain_type::get()` is not a valid expression.
  template <class T, class... Args>
  constexpr explicit status_code(in_place_t /*unused */, std::initializer_list<T> il, Args &&... args) noexcept(std::is_nothrow_constructible<value_type, std::initializer_list<T>, Args &&...>::value)
      : _base(typename _base::_value_type_constructor{}, &domain_type::get(), il, static_cast<Args &&>(args)...)
  {
  }
  //! Explicit copy construction from a `value_type`. Disables if `domain_type::get()` is not a valid expression.
  constexpr explicit status_code(const value_type &v) noexcept(std::is_nothrow_copy_constructible<value_type>::value)
      : _base(typename _base::_value_type_constructor{}, &domain_type::get(), v)
  {
  }
  //! Explicit move construction from a `value_type`. Disables if `domain_type::get()` is not a valid expression.
  constexpr explicit status_code(value_type &&v) noexcept(std::is_nothrow_move_constructible<value_type>::value)
      : _base(typename _base::_value_type_constructor{}, &domain_type::get(), static_cast<value_type &&>(v))
  {
  }
  /*! Explicit construction from an erased status code. Available only if
  `value_type` is trivially copyable or move bitcopying, and `sizeof(status_code) <= sizeof(status_code<erased<>>)`.
  Does not check if domains are equal.
  */
  template <class ErasedType,  //
            typename std::enable_if<detail::type_erasure_is_safe<ErasedType, value_type>::value, bool>::type = true>
  constexpr explicit status_code(const status_code<erased<ErasedType>> &v) noexcept(std::is_nothrow_copy_constructible<value_type>::value)
      : status_code(detail::erasure_cast<value_type>(v.value()))
  {
#if __cplusplus >= 201400
    assert(v.domain() == this->domain());
#endif
  }

  //! Return a reference to a string textually representing a code.
  string_ref message() const noexcept
  {
    // Avoid MSVC's buggy ternary operator for expensive to destruct things
    if(this->_domain != nullptr)
    {
      return string_ref(this->domain()._do_message(*this));
    }
    return string_ref("(empty)");
  }
};

namespace traits
{
  template <class DomainType> struct is_move_bitcopying<status_code<DomainType>>
  {
    static constexpr bool value = is_move_bitcopying<typename DomainType::value_type>::value;
  };
}  // namespace traits


/*! Type erased, move-only status_code, unlike `status_code<void>` which cannot be moved nor destroyed. Available
only if `erased<>` is available, which is when the domain's type is trivially
copyable or is move relocatable, and if the size of the domain's typed error code is less than or equal to
this erased error code. Copy construction is disabled, but if you want a copy call `.clone()`.

An ADL discovered helper function `make_status_code(T, Args...)` is looked up by one of the constructors.
If it is found, and it generates a status code compatible with this status code, implicit construction
is made available.
*/
template <class ErasedType> class BOOST_OUTCOME_SYSTEM_ERROR2_TRIVIAL_ABI status_code<erased<ErasedType>> : public mixins::mixin<detail::status_code_storage<erased<ErasedType>>, erased<ErasedType>>
{
  template <class T> friend class status_code;
  using _base = mixins::mixin<detail::status_code_storage<erased<ErasedType>>, erased<ErasedType>>;

public:
  //! The type of the domain (void, as it is erased).
  using domain_type = void;
  //! The type of the erased status code.
  using value_type = ErasedType;
  //! The type of a reference to a message string.
  using string_ref = typename _base::string_ref;

public:
  //! Default construction to empty
  status_code() = default;
  //! Copy constructor
  status_code(const status_code &) = delete;
  //! Move constructor
  status_code(status_code &&) = default;  // NOLINT
  //! Copy assignment
  status_code &operator=(const status_code &) = delete;
  //! Move assignment
  status_code &operator=(status_code &&) = default;  // NOLINT
  ~status_code()
  {
    if(nullptr != this->_domain)
    {
      this->_domain->_do_erased_destroy(*this, sizeof(*this));
    }
  }

  //! Return a copy of the erased code by asking the domain to perform the erased copy.
  status_code clone() const
  {
    if(nullptr == this->_domain)
    {
      return {};
    }
    status_code x;
    this->_domain->_do_erased_copy(x, *this, sizeof(*this));
    return x;
  }

  /***** KEEP THESE IN SYNC WITH ERRORED_STATUS_CODE *****/
  //! Implicit copy construction from any other status code if its value type is trivially copyable and it would fit into our storage
  template <class DomainType,                                                                           //
            typename std::enable_if<std::is_trivially_copyable<typename DomainType::value_type>::value  //
                                    && detail::type_erasure_is_safe<value_type, typename DomainType::value_type>::value,
                                    bool>::type = true>
  constexpr status_code(const status_code<DomainType> &v) noexcept  // NOLINT
      : _base(typename _base::_value_type_constructor{}, v._domain_ptr(), detail::erasure_cast<value_type>(v.value()))
  {
  }
  //! Implicit move construction from any other status code if its value type is trivially copyable or move bitcopying and it would fit into our storage
  template <class DomainType,  //
            typename std::enable_if<detail::type_erasure_is_safe<value_type, typename DomainType::value_type>::value, bool>::type = true>
  BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 status_code(status_code<DomainType> &&v) noexcept  // NOLINT
      : _base(typename _base::_value_type_constructor{}, v._domain_ptr(), detail::erasure_cast<value_type>(v.value()))
  {
    v._domain = nullptr;
  }
  //! Implicit construction from any type where an ADL discovered `make_status_code(T, Args ...)` returns a `status_code`.
  template <class T, class... Args,                                                                            //
            class MakeStatusCodeResult = typename detail::safe_get_make_status_code_result<T, Args...>::type,  // Safe ADL lookup of make_status_code(), returns void if not found
            typename std::enable_if<!std::is_same<typename std::decay<T>::type, status_code>::value            // not copy/move of self
                                    && !std::is_same<typename std::decay<T>::type, value_type>::value          // not copy/move of value type
                                    && is_status_code<MakeStatusCodeResult>::value                             // ADL makes a status code
                                    && std::is_constructible<status_code, MakeStatusCodeResult>::value,        // ADLed status code is compatible
                                    bool>::type = true>
  constexpr status_code(T &&v, Args &&... args) noexcept(noexcept(make_status_code(std::declval<T>(), std::declval<Args>()...)))  // NOLINT
      : status_code(make_status_code(static_cast<T &&>(v), static_cast<Args &&>(args)...))
  {
  }
  //! Implicit construction from any `quick_status_code_from_enum<Enum>` enumerated type.
  template <class Enum,                                                                              //
            class QuickStatusCodeType = typename quick_status_code_from_enum<Enum>::code_type,       // Enumeration has been activated
            typename std::enable_if<std::is_constructible<status_code, QuickStatusCodeType>::value,  // Its status code is compatible

                                    bool>::type = true>
  constexpr status_code(Enum &&v) noexcept(std::is_nothrow_constructible<status_code, QuickStatusCodeType>::value)  // NOLINT
      : status_code(QuickStatusCodeType(static_cast<Enum &&>(v)))
  {
  }

  /**** By rights ought to be removed in any formal standard ****/
  //! Reset the code to empty.
  BOOST_OUTCOME_SYSTEM_ERROR2_CONSTEXPR14 void clear() noexcept { *this = status_code(); }
  //! Return the erased `value_type` by value.
  constexpr value_type value() const noexcept { return this->_value; }
};

namespace traits
{
  template <class ErasedType> struct is_move_bitcopying<status_code<erased<ErasedType>>>
  {
    static constexpr bool value = true;
  };
}  // namespace traits

BOOST_OUTCOME_SYSTEM_ERROR2_NAMESPACE_END

#endif
