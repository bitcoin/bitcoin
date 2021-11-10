// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2016 Daniel James
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_IMPLEMENTATION_HPP
#define BOOST_UNORDERED_DETAIL_IMPLEMENTATION_HPP

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/assert.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/core/pointer_traits.hpp>
#include <boost/detail/select_type.hpp>
#include <boost/limits.hpp>
#include <boost/move/move.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/swap.hpp>
#include <boost/throw_exception.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/type_traits/add_lvalue_reference.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_empty.hpp>
#include <boost/type_traits/is_nothrow_move_assignable.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/type_traits/is_nothrow_swappable.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/unordered/detail/fwd.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/utility/enable_if.hpp>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <utility>

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)
#include <type_traits>
#endif

////////////////////////////////////////////////////////////////////////////////
// Configuration
//
// Unless documented elsewhere these configuration macros should be considered
// an implementation detail, I'll try not to break them, but you never know.

// Use Sun C++ workarounds
// I'm not sure which versions of the compiler require these workarounds, so
// I'm just using them of everything older than the current test compilers
// (as of May 2017).

#if !defined(BOOST_UNORDERED_SUN_WORKAROUNDS1)
#if BOOST_COMP_SUNPRO && BOOST_COMP_SUNPRO < BOOST_VERSION_NUMBER(5, 20, 0)
#define BOOST_UNORDERED_SUN_WORKAROUNDS1 1
#else
#define BOOST_UNORDERED_SUN_WORKAROUNDS1 0
#endif
#endif

// BOOST_UNORDERED_EMPLACE_LIMIT = The maximum number of parameters in
// emplace (not including things like hints). Don't set it to a lower value, as
// that might break something.

#if !defined BOOST_UNORDERED_EMPLACE_LIMIT
#define BOOST_UNORDERED_EMPLACE_LIMIT 10
#endif

// BOOST_UNORDERED_USE_ALLOCATOR_TRAITS - Pick which version of
// allocator_traits to use.
//
// 0 = Own partial implementation
// 1 = std::allocator_traits
// 2 = boost::container::allocator_traits

#if !defined(BOOST_UNORDERED_USE_ALLOCATOR_TRAITS)
#if !defined(BOOST_NO_CXX11_ALLOCATOR)
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 1
#elif defined(BOOST_MSVC)
#if BOOST_MSVC < 1400
// Use container's allocator_traits for older versions of Visual
// C++ as I don't test with them.
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 2
#endif
#endif
#endif

#if !defined(BOOST_UNORDERED_USE_ALLOCATOR_TRAITS)
#define BOOST_UNORDERED_USE_ALLOCATOR_TRAITS 0
#endif

// BOOST_UNORDERED_TUPLE_ARGS
//
// Maximum number of std::tuple members to support, or 0 if std::tuple
// isn't avaiable. More are supported when full C++11 is used.

// Already defined, so do nothing
#if defined(BOOST_UNORDERED_TUPLE_ARGS)

// Assume if we have C++11 tuple it's properly variadic,
// and just use a max number of 10 arguments.
#elif !defined(BOOST_NO_CXX11_HDR_TUPLE)
#define BOOST_UNORDERED_TUPLE_ARGS 10

// Visual C++ has a decent enough tuple for piecewise construction,
// so use that if available, using _VARIADIC_MAX for the maximum
// number of parameters. Note that this comes after the check
// for a full C++11 tuple.
#elif defined(BOOST_MSVC)
#if !BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT
#define BOOST_UNORDERED_TUPLE_ARGS 0
#elif defined(_VARIADIC_MAX)
#define BOOST_UNORDERED_TUPLE_ARGS _VARIADIC_MAX
#else
#define BOOST_UNORDERED_TUPLE_ARGS 5
#endif

// Assume that we don't have std::tuple
#else
#define BOOST_UNORDERED_TUPLE_ARGS 0
#endif

#if BOOST_UNORDERED_TUPLE_ARGS
#include <tuple>
#endif

// BOOST_UNORDERED_CXX11_CONSTRUCTION
//
// Use C++11 construction, requires variadic arguments, good construct support
// in allocator_traits and piecewise construction of std::pair
// Otherwise allocators aren't used for construction/destruction

#if BOOST_UNORDERED_HAVE_PIECEWISE_CONSTRUCT &&                                \
  !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && BOOST_UNORDERED_TUPLE_ARGS
#if BOOST_COMP_SUNPRO && BOOST_LIB_STD_GNU
// Sun C++ std::pair piecewise construction doesn't seem to be exception safe.
// (At least for Sun C++ 12.5 using libstdc++).
#define BOOST_UNORDERED_CXX11_CONSTRUCTION 0
#elif BOOST_COMP_GNUC && BOOST_COMP_GNUC < BOOST_VERSION_NUMBER(4, 7, 0)
// Piecewise construction in GCC 4.6 doesn't work for uncopyable types.
#define BOOST_UNORDERED_CXX11_CONSTRUCTION 0
#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 0 &&                             \
  !defined(BOOST_NO_SFINAE_EXPR)
#define BOOST_UNORDERED_CXX11_CONSTRUCTION 1
#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1
#define BOOST_UNORDERED_CXX11_CONSTRUCTION 1
#endif
#endif

#if !defined(BOOST_UNORDERED_CXX11_CONSTRUCTION)
#define BOOST_UNORDERED_CXX11_CONSTRUCTION 0
#endif

// BOOST_UNORDERED_SUPPRESS_DEPRECATED
//
// Define to stop deprecation attributes

#if defined(BOOST_UNORDERED_SUPPRESS_DEPRECATED)
#define BOOST_UNORDERED_DEPRECATED(msg)
#endif

// BOOST_UNORDERED_DEPRECATED
//
// Wrapper around various depreaction attributes.

#if defined(__has_cpp_attribute) &&                                            \
  (!defined(__cplusplus) || __cplusplus >= 201402)
#if __has_cpp_attribute(deprecated) && !defined(BOOST_UNORDERED_DEPRECATED)
#define BOOST_UNORDERED_DEPRECATED(msg) [[deprecated(msg)]]
#endif
#endif

#if !defined(BOOST_UNORDERED_DEPRECATED)
#if defined(__GNUC__) && __GNUC__ >= 4
#define BOOST_UNORDERED_DEPRECATED(msg) __attribute__((deprecated))
#elif defined(_MSC_VER) && _MSC_VER >= 1400
#define BOOST_UNORDERED_DEPRECATED(msg) __declspec(deprecated(msg))
#elif defined(_MSC_VER) && _MSC_VER >= 1310
#define BOOST_UNORDERED_DEPRECATED(msg) __declspec(deprecated)
#else
#define BOOST_UNORDERED_DEPRECATED(msg)
#endif
#endif

// BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES

#if !defined(BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES)
#if BOOST_COMP_CLANG && __cplusplus >= 201703
#define BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES 1
#endif
#endif

#if !defined(BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES)
#define BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES 0
#endif

namespace boost {
  namespace unordered {
    namespace iterator_detail {
      template <typename Node> struct iterator;
      template <typename Node> struct c_iterator;
      template <typename Node> struct l_iterator;
      template <typename Node> struct cl_iterator;
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {

      template <typename Types> struct table;
      template <typename NodePointer> struct bucket;
      struct ptr_bucket;

      template <typename A, typename T> struct node;
      template <typename T> struct ptr_node;

      static const float minimum_max_load_factor = 1e-3f;
      static const std::size_t default_bucket_count = 11;

      struct move_tag
      {
      };

      struct empty_emplace
      {
      };

      struct no_key
      {
        no_key() {}
        template <class T> no_key(T const&) {}
      };

      namespace func {
        template <class T> inline void ignore_unused_variable_warning(T const&)
        {
        }
      }

      //////////////////////////////////////////////////////////////////////////
      // iterator SFINAE

      template <typename I>
      struct is_forward : boost::is_base_of<std::forward_iterator_tag,
                            typename std::iterator_traits<I>::iterator_category>
      {
      };

      template <typename I, typename ReturnType>
      struct enable_if_forward
        : boost::enable_if_c<boost::unordered::detail::is_forward<I>::value,
            ReturnType>
      {
      };

      template <typename I, typename ReturnType>
      struct disable_if_forward
        : boost::disable_if_c<boost::unordered::detail::is_forward<I>::value,
            ReturnType>
      {
      };
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// primes

// clang-format off
#define BOOST_UNORDERED_PRIMES \
    (17ul)(29ul)(37ul)(53ul)(67ul)(79ul) \
    (97ul)(131ul)(193ul)(257ul)(389ul)(521ul)(769ul) \
    (1031ul)(1543ul)(2053ul)(3079ul)(6151ul)(12289ul)(24593ul) \
    (49157ul)(98317ul)(196613ul)(393241ul)(786433ul) \
    (1572869ul)(3145739ul)(6291469ul)(12582917ul)(25165843ul) \
    (50331653ul)(100663319ul)(201326611ul)(402653189ul)(805306457ul) \
    (1610612741ul)(3221225473ul)(4294967291ul)
// clang-format on

namespace boost {
  namespace unordered {
    namespace detail {
      template <class T> struct prime_list_template
      {
        static std::size_t const value[];

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1
        static std::ptrdiff_t const length;
#else
        static std::ptrdiff_t const length =
          BOOST_PP_SEQ_SIZE(BOOST_UNORDERED_PRIMES);
#endif
      };

      template <class T>
      std::size_t const prime_list_template<T>::value[] = {
        BOOST_PP_SEQ_ENUM(BOOST_UNORDERED_PRIMES)};

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1
      template <class T>
      std::ptrdiff_t const prime_list_template<T>::length = BOOST_PP_SEQ_SIZE(
        BOOST_UNORDERED_PRIMES);
#endif

#undef BOOST_UNORDERED_PRIMES

      typedef prime_list_template<std::size_t> prime_list;

      // no throw
      inline std::size_t next_prime(std::size_t num)
      {
        std::size_t const* const prime_list_begin = prime_list::value;
        std::size_t const* const prime_list_end =
          prime_list_begin + prime_list::length;
        std::size_t const* bound =
          std::lower_bound(prime_list_begin, prime_list_end, num);
        if (bound == prime_list_end)
          bound--;
        return *bound;
      }

      // no throw
      inline std::size_t prev_prime(std::size_t num)
      {
        std::size_t const* const prime_list_begin = prime_list::value;
        std::size_t const* const prime_list_end =
          prime_list_begin + prime_list::length;
        std::size_t const* bound =
          std::upper_bound(prime_list_begin, prime_list_end, num);
        if (bound != prime_list_begin)
          bound--;
        return *bound;
      }

      //////////////////////////////////////////////////////////////////////////
      // insert_size/initial_size

      template <class I>
      inline std::size_t insert_size(I i, I j,
        typename boost::unordered::detail::enable_if_forward<I, void*>::type =
          0)
      {
        return static_cast<std::size_t>(std::distance(i, j));
      }

      template <class I>
      inline std::size_t insert_size(I, I,
        typename boost::unordered::detail::disable_if_forward<I, void*>::type =
          0)
      {
        return 1;
      }

      template <class I>
      inline std::size_t initial_size(I i, I j,
        std::size_t num_buckets =
          boost::unordered::detail::default_bucket_count)
      {
        return (std::max)(
          boost::unordered::detail::insert_size(i, j), num_buckets);
      }

      //////////////////////////////////////////////////////////////////////////
      // compressed

      template <typename T, int Index> struct compressed_base : private T
      {
        compressed_base(T const& x) : T(x) {}
        compressed_base(T& x, move_tag) : T(boost::move(x)) {}

        T& get() { return *this; }
        T const& get() const { return *this; }
      };

      template <typename T, int Index> struct uncompressed_base
      {
        uncompressed_base(T const& x) : value_(x) {}
        uncompressed_base(T& x, move_tag) : value_(boost::move(x)) {}

        T& get() { return value_; }
        T const& get() const { return value_; }

      private:
        T value_;
      };

      template <typename T, int Index>
      struct generate_base
        : boost::detail::if_true<
            boost::is_empty<T>::value>::BOOST_NESTED_TEMPLATE
            then<boost::unordered::detail::compressed_base<T, Index>,
              boost::unordered::detail::uncompressed_base<T, Index> >
      {
      };

      template <typename T1, typename T2>
      struct compressed
        : private boost::unordered::detail::generate_base<T1, 1>::type,
          private boost::unordered::detail::generate_base<T2, 2>::type
      {
        typedef typename generate_base<T1, 1>::type base1;
        typedef typename generate_base<T2, 2>::type base2;

        typedef T1 first_type;
        typedef T2 second_type;

        first_type& first() { return static_cast<base1*>(this)->get(); }

        first_type const& first() const
        {
          return static_cast<base1 const*>(this)->get();
        }

        second_type& second() { return static_cast<base2*>(this)->get(); }

        second_type const& second() const
        {
          return static_cast<base2 const*>(this)->get();
        }

        template <typename First, typename Second>
        compressed(First const& x1, Second const& x2) : base1(x1), base2(x2)
        {
        }

        compressed(compressed const& x) : base1(x.first()), base2(x.second()) {}

        compressed(compressed& x, move_tag m)
            : base1(x.first(), m), base2(x.second(), m)
        {
        }

        void assign(compressed const& x)
        {
          first() = x.first();
          second() = x.second();
        }

        void move_assign(compressed& x)
        {
          first() = boost::move(x.first());
          second() = boost::move(x.second());
        }

        void swap(compressed& x)
        {
          boost::swap(first(), x.first());
          boost::swap(second(), x.second());
        }

      private:
        // Prevent assignment just to make use of assign or
        // move_assign explicit.
        compressed& operator=(compressed const&);
      };

      //////////////////////////////////////////////////////////////////////////
      // pair_traits
      //
      // Used to get the types from a pair without instantiating it.

      template <typename Pair> struct pair_traits
      {
        typedef typename Pair::first_type first_type;
        typedef typename Pair::second_type second_type;
      };

      template <typename T1, typename T2> struct pair_traits<std::pair<T1, T2> >
      {
        typedef T1 first_type;
        typedef T2 second_type;
      };

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4512) // assignment operator could not be generated.
#pragma warning(disable : 4345) // behavior change: an object of POD type
// constructed with an initializer of the form ()
// will be default-initialized.
#endif

      //////////////////////////////////////////////////////////////////////////
      // Bits and pieces for implementing traits

      template <typename T>
      typename boost::add_lvalue_reference<T>::type make();
      struct choice9
      {
        typedef char (&type)[9];
      };
      struct choice8 : choice9
      {
        typedef char (&type)[8];
      };
      struct choice7 : choice8
      {
        typedef char (&type)[7];
      };
      struct choice6 : choice7
      {
        typedef char (&type)[6];
      };
      struct choice5 : choice6
      {
        typedef char (&type)[5];
      };
      struct choice4 : choice5
      {
        typedef char (&type)[4];
      };
      struct choice3 : choice4
      {
        typedef char (&type)[3];
      };
      struct choice2 : choice3
      {
        typedef char (&type)[2];
      };
      struct choice1 : choice2
      {
        typedef char (&type)[1];
      };
      choice1 choose();

      typedef choice1::type yes_type;
      typedef choice2::type no_type;

      struct private_type
      {
        private_type const& operator,(int) const;
      };

      template <typename T> no_type is_private_type(T const&);
      yes_type is_private_type(private_type const&);

      struct convert_from_anything
      {
        template <typename T> convert_from_anything(T const&);
      };
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// emplace_args
//
// Either forwarding variadic arguments, or storing the arguments in
// emplace_args##n

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_EMPLACE_TEMPLATE typename... Args
#define BOOST_UNORDERED_EMPLACE_ARGS BOOST_FWD_REF(Args)... args
#define BOOST_UNORDERED_EMPLACE_FORWARD boost::forward<Args>(args)...

#else

#define BOOST_UNORDERED_EMPLACE_TEMPLATE typename Args
#define BOOST_UNORDERED_EMPLACE_ARGS Args const& args
#define BOOST_UNORDERED_EMPLACE_FORWARD args

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

#define BOOST_UNORDERED_EARGS_MEMBER(z, n, _)                                  \
  typedef BOOST_FWD_REF(BOOST_PP_CAT(A, n)) BOOST_PP_CAT(Arg, n);              \
  BOOST_PP_CAT(Arg, n) BOOST_PP_CAT(a, n);

#else

#define BOOST_UNORDERED_EARGS_MEMBER(z, n, _)                                  \
  typedef typename boost::add_lvalue_reference<BOOST_PP_CAT(A, n)>::type       \
    BOOST_PP_CAT(Arg, n);                                                      \
  BOOST_PP_CAT(Arg, n) BOOST_PP_CAT(a, n);

#endif

#define BOOST_UNORDERED_FWD_PARAM(z, n, a)                                     \
  BOOST_FWD_REF(BOOST_PP_CAT(A, n)) BOOST_PP_CAT(a, n)

#define BOOST_UNORDERED_CALL_FORWARD(z, i, a)                                  \
  boost::forward<BOOST_PP_CAT(A, i)>(BOOST_PP_CAT(a, i))

#define BOOST_UNORDERED_EARGS_INIT(z, n, _)                                    \
  BOOST_PP_CAT(a, n)(BOOST_PP_CAT(b, n))

#define BOOST_UNORDERED_EARGS(z, n, _)                                         \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  struct BOOST_PP_CAT(emplace_args, n)                                         \
  {                                                                            \
    BOOST_PP_REPEAT_##z(n, BOOST_UNORDERED_EARGS_MEMBER, _) BOOST_PP_CAT(      \
      emplace_args, n)(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, n, Arg, b))            \
        : BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_EARGS_INIT, _)                  \
    {                                                                          \
    }                                                                          \
  };                                                                           \
                                                                               \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  inline BOOST_PP_CAT(emplace_args, n)<BOOST_PP_ENUM_PARAMS_Z(z, n, A)>        \
    create_emplace_args(BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, b))    \
  {                                                                            \
    BOOST_PP_CAT(emplace_args, n)<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> e(          \
      BOOST_PP_ENUM_PARAMS_Z(z, n, b));                                        \
    return e;                                                                  \
  }

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename A0> struct emplace_args1
      {
        BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)

        explicit emplace_args1(Arg0 b0) : a0(b0) {}
      };

      template <typename A0>
      inline emplace_args1<A0> create_emplace_args(BOOST_FWD_REF(A0) b0)
      {
        emplace_args1<A0> e(b0);
        return e;
      }

      template <typename A0, typename A1> struct emplace_args2
      {
        BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)
        BOOST_UNORDERED_EARGS_MEMBER(1, 1, _)

        emplace_args2(Arg0 b0, Arg1 b1) : a0(b0), a1(b1) {}
      };

      template <typename A0, typename A1>
      inline emplace_args2<A0, A1> create_emplace_args(
        BOOST_FWD_REF(A0) b0, BOOST_FWD_REF(A1) b1)
      {
        emplace_args2<A0, A1> e(b0, b1);
        return e;
      }

      template <typename A0, typename A1, typename A2> struct emplace_args3
      {
        BOOST_UNORDERED_EARGS_MEMBER(1, 0, _)
        BOOST_UNORDERED_EARGS_MEMBER(1, 1, _)
        BOOST_UNORDERED_EARGS_MEMBER(1, 2, _)

        emplace_args3(Arg0 b0, Arg1 b1, Arg2 b2) : a0(b0), a1(b1), a2(b2) {}
      };

      template <typename A0, typename A1, typename A2>
      inline emplace_args3<A0, A1, A2> create_emplace_args(
        BOOST_FWD_REF(A0) b0, BOOST_FWD_REF(A1) b1, BOOST_FWD_REF(A2) b2)
      {
        emplace_args3<A0, A1, A2> e(b0, b1, b2);
        return e;
      }

      BOOST_UNORDERED_EARGS(1, 4, _)
      BOOST_UNORDERED_EARGS(1, 5, _)
      BOOST_UNORDERED_EARGS(1, 6, _)
      BOOST_UNORDERED_EARGS(1, 7, _)
      BOOST_UNORDERED_EARGS(1, 8, _)
      BOOST_UNORDERED_EARGS(1, 9, _)
      BOOST_PP_REPEAT_FROM_TO(10, BOOST_PP_INC(BOOST_UNORDERED_EMPLACE_LIMIT),
        BOOST_UNORDERED_EARGS, _)
    }
  }
}

#undef BOOST_UNORDERED_DEFINE_EMPLACE_ARGS
#undef BOOST_UNORDERED_EARGS_MEMBER
#undef BOOST_UNORDERED_EARGS_INIT

#endif

////////////////////////////////////////////////////////////////////////////////
//
// Some utilities for implementing allocator_traits, but useful elsewhere so
// they're always defined.

namespace boost {
  namespace unordered {
    namespace detail {

////////////////////////////////////////////////////////////////////////////
// Integral_constrant, true_type, false_type
//
// Uses the standard versions if available.

#if !defined(BOOST_NO_CXX11_HDR_TYPE_TRAITS)

      using std::integral_constant;
      using std::true_type;
      using std::false_type;

#else

      template <typename T, T Value> struct integral_constant
      {
        enum
        {
          value = Value
        };
      };

      typedef boost::unordered::detail::integral_constant<bool, true> true_type;
      typedef boost::unordered::detail::integral_constant<bool, false>
        false_type;

#endif

////////////////////////////////////////////////////////////////////////////
// Explicitly call a destructor

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // unreferenced formal parameter
#endif

      namespace func {
        template <class T> inline void destroy(T* x) { x->~T(); }
      }

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

      //////////////////////////////////////////////////////////////////////////
      // value_base
      //
      // Space used to store values.

      template <typename ValueType> struct value_base
      {
        typedef ValueType value_type;

        typename boost::aligned_storage<sizeof(value_type),
          boost::alignment_of<value_type>::value>::type data_;

        value_base() : data_() {}

        void* address() { return this; }

        value_type& value() { return *(ValueType*)this; }

        value_type const& value() const { return *(ValueType const*)this; }

        value_type* value_ptr() { return (ValueType*)this; }

        value_type const* value_ptr() const { return (ValueType const*)this; }

      private:
        value_base& operator=(value_base const&);
      };

      //////////////////////////////////////////////////////////////////////////
      // optional
      // TODO: Use std::optional when available.

      template <typename T> class optional
      {
        BOOST_MOVABLE_BUT_NOT_COPYABLE(optional)

        boost::unordered::detail::value_base<T> value_;
        bool has_value_;

        void destroy()
        {
          if (has_value_) {
            boost::unordered::detail::func::destroy(value_.value_ptr());
            has_value_ = false;
          }
        }

        void move(optional<T>& x)
        {
          BOOST_ASSERT(!has_value_ && x.has_value_);
          new (value_.value_ptr()) T(boost::move(x.value_.value()));
          boost::unordered::detail::func::destroy(x.value_.value_ptr());
          has_value_ = true;
          x.has_value_ = false;
        }

      public:
        optional() BOOST_NOEXCEPT : has_value_(false) {}

        optional(BOOST_RV_REF(optional<T>) x) : has_value_(false)
        {
          if (x.has_value_) {
            move(x);
          }
        }

        explicit optional(T const& x) : has_value_(true)
        {
          new (value_.value_ptr()) T(x);
        }

        optional& operator=(BOOST_RV_REF(optional<T>) x)
        {
          destroy();
          if (x.has_value_) {
            move(x);
          }
          return *this;
        }

        ~optional() { destroy(); }

        bool has_value() const { return has_value_; }
        T& operator*() { return value_.value(); }
        T const& operator*() const { return value_.value(); }
        T* operator->() { return value_.value_ptr(); }
        T const* operator->() const { return value_.value_ptr(); }

        bool operator==(optional<T> const& x)
        {
          return has_value_ ? x.has_value_ && value_.value() == x.value_.value()
                            : !x.has_value_;
        }

        bool operator!=(optional<T> const& x) { return !((*this) == x); }

        void swap(optional<T>& x)
        {
          if (has_value_ != x.has_value_) {
            if (has_value_) {
              x.move(*this);
            } else {
              move(x);
            }
          } else if (has_value_) {
            boost::swap(value_.value(), x.value_.value());
          }
        }

        friend void swap(optional<T>& x, optional<T>& y) { x.swap(y); }
      };
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// Expression test mechanism
//
// When SFINAE expressions are available, define
// BOOST_UNORDERED_HAS_FUNCTION which can check if a function call is
// supported by a class, otherwise define BOOST_UNORDERED_HAS_MEMBER which
// can detect if a class has the specified member, but not that it has the
// correct type, this is good enough for a passable impression of
// allocator_traits.

#if !defined(BOOST_NO_SFINAE_EXPR)

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename T, long unsigned int> struct expr_test;
      template <typename T> struct expr_test<T, sizeof(char)> : T
      {
      };
    }
  }
}

#define BOOST_UNORDERED_CHECK_EXPRESSION(count, result, expression)            \
  template <typename U>                                                        \
  static                                                                       \
    typename boost::unordered::detail::expr_test<BOOST_PP_CAT(choice, result), \
      sizeof(for_expr_test(((expression), 0)))>::type                          \
      test(BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_DEFAULT_EXPRESSION(count, result)                      \
  template <typename U>                                                        \
  static BOOST_PP_CAT(choice, result)::type test(BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_HAS_FUNCTION(name, thing, args, _)                     \
  struct BOOST_PP_CAT(has_, name)                                              \
  {                                                                            \
    template <typename U> static char for_expr_test(U const&);                 \
    BOOST_UNORDERED_CHECK_EXPRESSION(                                          \
      1, 1, boost::unordered::detail::make<thing>().name args);                \
    BOOST_UNORDERED_DEFAULT_EXPRESSION(2, 2);                                  \
                                                                               \
    enum                                                                       \
    {                                                                          \
      value = sizeof(test<T>(choose())) == sizeof(choice1::type)               \
    };                                                                         \
  }

#else

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename T> struct identity
      {
        typedef T type;
      };
    }
  }
}

#define BOOST_UNORDERED_CHECK_MEMBER(count, result, name, member)              \
                                                                               \
  typedef                                                                      \
    typename boost::unordered::detail::identity<member>::type BOOST_PP_CAT(    \
      check, count);                                                           \
                                                                               \
  template <BOOST_PP_CAT(check, count) e> struct BOOST_PP_CAT(test, count)     \
  {                                                                            \
    typedef BOOST_PP_CAT(choice, result) type;                                 \
  };                                                                           \
                                                                               \
  template <class U>                                                           \
  static typename BOOST_PP_CAT(test, count)<&U::name>::type test(              \
    BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_DEFAULT_MEMBER(count, result)                          \
  template <class U>                                                           \
  static BOOST_PP_CAT(choice, result)::type test(BOOST_PP_CAT(choice, count))

#define BOOST_UNORDERED_HAS_MEMBER(name)                                       \
  struct BOOST_PP_CAT(has_, name)                                              \
  {                                                                            \
    struct impl                                                                \
    {                                                                          \
      struct base_mixin                                                        \
      {                                                                        \
        int name;                                                              \
      };                                                                       \
      struct base : public T, public base_mixin                                \
      {                                                                        \
      };                                                                       \
                                                                               \
      BOOST_UNORDERED_CHECK_MEMBER(1, 1, name, int base_mixin::*);             \
      BOOST_UNORDERED_DEFAULT_MEMBER(2, 2);                                    \
                                                                               \
      enum                                                                     \
      {                                                                        \
        value = sizeof(choice2::type) == sizeof(test<base>(choose()))          \
      };                                                                       \
    };                                                                         \
                                                                               \
    enum                                                                       \
    {                                                                          \
      value = impl::value                                                      \
    };                                                                         \
  }

#endif

////////////////////////////////////////////////////////////////////////////
// TRAITS TYPE DETECTION MECHANISM
//
// Used to implement traits that use a type if present, or a
// default otherwise.

#if defined(BOOST_MSVC) && BOOST_MSVC <= 1400

#define BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(tname)                              \
  template <typename Tp, typename Default> struct default_type_##tname         \
  {                                                                            \
                                                                               \
    template <typename X>                                                      \
    static choice1::type test(choice1, typename X::tname* = 0);                \
                                                                               \
    template <typename X> static choice2::type test(choice2, void* = 0);       \
                                                                               \
    struct DefaultWrap                                                         \
    {                                                                          \
      typedef Default tname;                                                   \
    };                                                                         \
                                                                               \
    enum                                                                       \
    {                                                                          \
      value = (1 == sizeof(test<Tp>(choose())))                                \
    };                                                                         \
                                                                               \
    typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE      \
      then<Tp, DefaultWrap>::type::tname type;                                 \
  }

#else

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename T, typename T2> struct sfinae : T2
      {
      };
    }
  }
}

#define BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(tname)                              \
  template <typename Tp, typename Default> struct default_type_##tname         \
  {                                                                            \
                                                                               \
    template <typename X>                                                      \
    static typename boost::unordered::detail::sfinae<typename X::tname,        \
      choice1>::type test(choice1);                                            \
                                                                               \
    template <typename X> static choice2::type test(choice2);                  \
                                                                               \
    struct DefaultWrap                                                         \
    {                                                                          \
      typedef Default tname;                                                   \
    };                                                                         \
                                                                               \
    enum                                                                       \
    {                                                                          \
      value = (1 == sizeof(test<Tp>(choose())))                                \
    };                                                                         \
                                                                               \
    typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE      \
      then<Tp, DefaultWrap>::type::tname type;                                 \
  }

#endif

#define BOOST_UNORDERED_DEFAULT_TYPE(T, tname, arg)                            \
  typename default_type_##tname<T, arg>::type

////////////////////////////////////////////////////////////////////////////////
//
// Allocator traits
//
// First our implementation, then later light wrappers around the alternatives

#if BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 0

#include <boost/limits.hpp>
#include <boost/pointer_to_other.hpp>
#include <boost/utility/enable_if.hpp>

namespace boost {
  namespace unordered {
    namespace detail {

      template <typename Alloc, typename T> struct rebind_alloc;

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <template <typename, typename...> class Alloc, typename U,
        typename T, typename... Args>
      struct rebind_alloc<Alloc<U, Args...>, T>
      {
        typedef Alloc<T, Args...> type;
      };

#else

      template <template <typename> class Alloc, typename U, typename T>
      struct rebind_alloc<Alloc<U>, T>
      {
        typedef Alloc<T> type;
      };

      template <template <typename, typename> class Alloc, typename U,
        typename T, typename A0>
      struct rebind_alloc<Alloc<U, A0>, T>
      {
        typedef Alloc<T, A0> type;
      };

      template <template <typename, typename, typename> class Alloc, typename U,
        typename T, typename A0, typename A1>
      struct rebind_alloc<Alloc<U, A0, A1>, T>
      {
        typedef Alloc<T, A0, A1> type;
      };

#endif

      template <typename Alloc, typename T> struct rebind_wrap
      {
        template <typename X>
        static choice1::type test(
          choice1, typename X::BOOST_NESTED_TEMPLATE rebind<T>::other* = 0);
        template <typename X> static choice2::type test(choice2, void* = 0);

        enum
        {
          value = (1 == sizeof(test<Alloc>(choose())))
        };

        struct fallback
        {
          template <typename U> struct rebind
          {
            typedef typename rebind_alloc<Alloc, T>::type other;
          };
        };

        typedef
          typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE then<
            Alloc, fallback>::type::BOOST_NESTED_TEMPLATE rebind<T>::other type;
      };
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(pointer);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(const_pointer);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(void_pointer);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(const_void_pointer);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(difference_type);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(size_type);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(
        propagate_on_container_copy_assignment);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(
        propagate_on_container_move_assignment);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(propagate_on_container_swap);
      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(is_always_equal);

#if !defined(BOOST_NO_SFINAE_EXPR)

      template <typename T>
      BOOST_UNORDERED_HAS_FUNCTION(
        select_on_container_copy_construction, U const, (), 0);

      template <typename T>
      BOOST_UNORDERED_HAS_FUNCTION(max_size, U const, (), 0);

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <typename T, typename ValueType, typename... Args>
      BOOST_UNORDERED_HAS_FUNCTION(construct, U,
        (boost::unordered::detail::make<ValueType*>(),
          boost::unordered::detail::make<Args const>()...),
        2);

#else

      template <typename T, typename ValueType>
      BOOST_UNORDERED_HAS_FUNCTION(construct, U,
        (boost::unordered::detail::make<ValueType*>(),
          boost::unordered::detail::make<ValueType const>()),
        2);

#endif

      template <typename T, typename ValueType>
      BOOST_UNORDERED_HAS_FUNCTION(
        destroy, U, (boost::unordered::detail::make<ValueType*>()), 1);

#else

      template <typename T>
      BOOST_UNORDERED_HAS_MEMBER(select_on_container_copy_construction);

      template <typename T> BOOST_UNORDERED_HAS_MEMBER(max_size);

      template <typename T, typename ValueType>
      BOOST_UNORDERED_HAS_MEMBER(construct);

      template <typename T, typename ValueType>
      BOOST_UNORDERED_HAS_MEMBER(destroy);

#endif
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {

        template <typename Alloc>
        inline Alloc call_select_on_container_copy_construction(
          const Alloc& rhs,
          typename boost::enable_if_c<
            boost::unordered::detail::has_select_on_container_copy_construction<
              Alloc>::value,
            void*>::type = 0)
        {
          return rhs.select_on_container_copy_construction();
        }

        template <typename Alloc>
        inline Alloc call_select_on_container_copy_construction(
          const Alloc& rhs,
          typename boost::disable_if_c<
            boost::unordered::detail::has_select_on_container_copy_construction<
              Alloc>::value,
            void*>::type = 0)
        {
          return rhs;
        }

        template <typename SizeType, typename Alloc>
        inline SizeType call_max_size(const Alloc& a,
          typename boost::enable_if_c<
            boost::unordered::detail::has_max_size<Alloc>::value, void*>::type =
            0)
        {
          return a.max_size();
        }

        template <typename SizeType, typename Alloc>
        inline SizeType call_max_size(const Alloc&,
          typename boost::disable_if_c<
            boost::unordered::detail::has_max_size<Alloc>::value, void*>::type =
            0)
        {
          return (std::numeric_limits<SizeType>::max)();
        }
      } // namespace func.
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {
      template <typename Alloc> struct allocator_traits
      {
        typedef Alloc allocator_type;
        typedef typename Alloc::value_type value_type;

        typedef BOOST_UNORDERED_DEFAULT_TYPE(
          Alloc, pointer, value_type*) pointer;

        template <typename T>
        struct pointer_to_other : boost::pointer_to_other<pointer, T>
        {
        };

        typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, const_pointer,
          typename pointer_to_other<const value_type>::type) const_pointer;

        // typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, void_pointer,
        //    typename pointer_to_other<void>::type)
        //    void_pointer;
        //
        // typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, const_void_pointer,
        //    typename pointer_to_other<const void>::type)
        //    const_void_pointer;

        typedef BOOST_UNORDERED_DEFAULT_TYPE(
          Alloc, difference_type, std::ptrdiff_t) difference_type;

        typedef BOOST_UNORDERED_DEFAULT_TYPE(
          Alloc, size_type, std::size_t) size_type;

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)
        template <typename T>
        using rebind_alloc = typename rebind_wrap<Alloc, T>::type;

        template <typename T>
        using rebind_traits =
          boost::unordered::detail::allocator_traits<rebind_alloc<T> >;
#endif

        static pointer allocate(Alloc& a, size_type n) { return a.allocate(n); }

        // I never use this, so I'll just comment it out for now.
        //
        // static pointer allocate(Alloc& a, size_type n,
        //        const_void_pointer hint)
        //    { return DEFAULT_FUNC(allocate, pointer)(a, n, hint); }

        static void deallocate(Alloc& a, pointer p, size_type n)
        {
          a.deallocate(p, n);
        }

      public:
#if BOOST_UNORDERED_CXX11_CONSTRUCTION

        template <typename T, typename... Args>
        static
          typename boost::enable_if_c<boost::unordered::detail::has_construct<
            Alloc, T, Args...>::value>::type
          construct(Alloc& a, T* p, BOOST_FWD_REF(Args)... x)
        {
          a.construct(p, boost::forward<Args>(x)...);
        }

        template <typename T, typename... Args>
        static
          typename boost::disable_if_c<boost::unordered::detail::has_construct<
            Alloc, T, Args...>::value>::type
          construct(Alloc&, T* p, BOOST_FWD_REF(Args)... x)
        {
          new (static_cast<void*>(p)) T(boost::forward<Args>(x)...);
        }

        template <typename T>
        static typename boost::enable_if_c<
          boost::unordered::detail::has_destroy<Alloc, T>::value>::type
        destroy(Alloc& a, T* p)
        {
          a.destroy(p);
        }

        template <typename T>
        static typename boost::disable_if_c<
          boost::unordered::detail::has_destroy<Alloc, T>::value>::type
        destroy(Alloc&, T* p)
        {
          boost::unordered::detail::func::destroy(p);
        }

#elif !defined(BOOST_NO_SFINAE_EXPR)

        template <typename T>
        static typename boost::enable_if_c<
          boost::unordered::detail::has_construct<Alloc, T>::value>::type
        construct(Alloc& a, T* p, T const& x)
        {
          a.construct(p, x);
        }

        template <typename T>
        static typename boost::disable_if_c<
          boost::unordered::detail::has_construct<Alloc, T>::value>::type
        construct(Alloc&, T* p, T const& x)
        {
          new (static_cast<void*>(p)) T(x);
        }

        template <typename T>
        static typename boost::enable_if_c<
          boost::unordered::detail::has_destroy<Alloc, T>::value>::type
        destroy(Alloc& a, T* p)
        {
          a.destroy(p);
        }

        template <typename T>
        static typename boost::disable_if_c<
          boost::unordered::detail::has_destroy<Alloc, T>::value>::type
        destroy(Alloc&, T* p)
        {
          boost::unordered::detail::func::destroy(p);
        }

#else

        // If we don't have SFINAE expressions, only call construct for the
        // copy constructor for the allocator's value_type - as that's
        // the only construct method that old fashioned allocators support.

        template <typename T>
        static void construct(Alloc& a, T* p, T const& x,
          typename boost::enable_if_c<
            boost::unordered::detail::has_construct<Alloc, T>::value &&
              boost::is_same<T, value_type>::value,
            void*>::type = 0)
        {
          a.construct(p, x);
        }

        template <typename T>
        static void construct(Alloc&, T* p, T const& x,
          typename boost::disable_if_c<
            boost::unordered::detail::has_construct<Alloc, T>::value &&
              boost::is_same<T, value_type>::value,
            void*>::type = 0)
        {
          new (static_cast<void*>(p)) T(x);
        }

        template <typename T>
        static void destroy(Alloc& a, T* p,
          typename boost::enable_if_c<
            boost::unordered::detail::has_destroy<Alloc, T>::value &&
              boost::is_same<T, value_type>::value,
            void*>::type = 0)
        {
          a.destroy(p);
        }

        template <typename T>
        static void destroy(Alloc&, T* p,
          typename boost::disable_if_c<
            boost::unordered::detail::has_destroy<Alloc, T>::value &&
              boost::is_same<T, value_type>::value,
            void*>::type = 0)
        {
          boost::unordered::detail::func::destroy(p);
        }

#endif

        static size_type max_size(const Alloc& a)
        {
          return boost::unordered::detail::func::call_max_size<size_type>(a);
        }

        // Allocator propagation on construction

        static Alloc select_on_container_copy_construction(Alloc const& rhs)
        {
          return boost::unordered::detail::func::
            call_select_on_container_copy_construction(rhs);
        }

        // Allocator propagation on assignment and swap.
        // Return true if lhs is modified.
        typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc,
          propagate_on_container_copy_assignment,
          false_type) propagate_on_container_copy_assignment;
        typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc,
          propagate_on_container_move_assignment,
          false_type) propagate_on_container_move_assignment;
        typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, propagate_on_container_swap,
          false_type) propagate_on_container_swap;

        typedef BOOST_UNORDERED_DEFAULT_TYPE(Alloc, is_always_equal,
          typename boost::is_empty<Alloc>::type) is_always_equal;
      };
    }
  }
}

#undef BOOST_UNORDERED_DEFAULT_TYPE_TMPLT
#undef BOOST_UNORDERED_DEFAULT_TYPE

////////////////////////////////////////////////////////////////////////////////
//
// std::allocator_traits

#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1

#include <memory>

namespace boost {
  namespace unordered {
    namespace detail {

      BOOST_UNORDERED_DEFAULT_TYPE_TMPLT(is_always_equal);

      template <typename Alloc>
      struct allocator_traits : std::allocator_traits<Alloc>
      {
        // As is_always_equal was introduced in C++17, std::allocator_traits
        // doesn't always have it. So use it when available, implement it
        // ourselves when not. Would be simpler not to bother with
        // std::allocator_traits, but I feel like I should try to use
        // it where possible.
        typedef BOOST_UNORDERED_DEFAULT_TYPE(std::allocator_traits<Alloc>,
          is_always_equal,
          BOOST_UNORDERED_DEFAULT_TYPE(Alloc, is_always_equal,
            typename boost::is_empty<Alloc>::type)) is_always_equal;
      };

      template <typename Alloc, typename T> struct rebind_wrap
      {
        typedef typename std::allocator_traits<Alloc>::template rebind_alloc<T>
          type;
      };
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// boost::container::allocator_traits

#elif BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 2

#include <boost/container/allocator_traits.hpp>

namespace boost {
  namespace unordered {
    namespace detail {

      template <typename Alloc>
      struct allocator_traits : boost::container::allocator_traits<Alloc>
      {
      };

      template <typename Alloc, typename T>
      struct rebind_wrap : boost::container::allocator_traits<
                             Alloc>::template portable_rebind_alloc<T>
      {
      };
    }
  }
}

#else

#error "Invalid BOOST_UNORDERED_USE_ALLOCATOR_TRAITS value."

#endif

////////////////////////////////////////////////////////////////////////////
// Functions used to construct nodes. Emulates variadic construction,
// piecewise construction etc.

////////////////////////////////////////////////////////////////////////////
// construct_value
//
// Only use allocator_traits::construct, allocator_traits::destroy when full
// C++11 support is available.

#if BOOST_UNORDERED_CXX11_CONSTRUCTION

#define BOOST_UNORDERED_CALL_CONSTRUCT1(Traits, alloc, address, a0)            \
  Traits::construct(alloc, address, a0)
#define BOOST_UNORDERED_CALL_DESTROY(Traits, alloc, x) Traits::destroy(alloc, x)

#elif !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        template <typename T, typename... Args>
        inline void construct_value(T* address, BOOST_FWD_REF(Args)... args)
        {
          new ((void*)address) T(boost::forward<Args>(args)...);
        }
      }
    }
  }
}

#define BOOST_UNORDERED_CALL_CONSTRUCT1(Traits, alloc, address, a0)            \
  boost::unordered::detail::func::construct_value(address, a0)
#define BOOST_UNORDERED_CALL_DESTROY(Traits, alloc, x)                         \
  boost::unordered::detail::func::destroy(x)

#else

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        template <typename T> inline void construct_value(T* address)
        {
          new ((void*)address) T();
        }

        template <typename T, typename A0>
        inline void construct_value(T* address, BOOST_FWD_REF(A0) a0)
        {
          new ((void*)address) T(boost::forward<A0>(a0));
        }
      }
    }
  }
}

#define BOOST_UNORDERED_CALL_CONSTRUCT1(Traits, alloc, address, a0)            \
  boost::unordered::detail::func::construct_value(address, a0)
#define BOOST_UNORDERED_CALL_DESTROY(Traits, alloc, x)                         \
  boost::unordered::detail::func::destroy(x)

#endif

////////////////////////////////////////////////////////////////////////////
// Construct from tuple
//
// Used to emulate piecewise construction.

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(z, n, namespace_)                 \
  template <typename Alloc, typename T,                                        \
    BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                                  \
  void construct_from_tuple(Alloc&, T* ptr,                                    \
    namespace_::tuple<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> const& x)               \
  {                                                                            \
    new ((void*)ptr)                                                           \
      T(BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_GET_TUPLE_ARG, namespace_));      \
  }

#define BOOST_UNORDERED_GET_TUPLE_ARG(z, n, namespace_) namespace_::get<n>(x)

// construct_from_tuple for boost::tuple
// The workaround for old Sun compilers comes later in the file.

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        template <typename Alloc, typename T>
        void construct_from_tuple(Alloc&, T* ptr, boost::tuple<>)
        {
          new ((void*)ptr) T();
        }

        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 1, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 2, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 3, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 4, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 5, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 6, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 7, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 8, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 9, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 10, boost)
      }
    }
  }
}

#endif

// construct_from_tuple for std::tuple

#if !BOOST_UNORDERED_CXX11_CONSTRUCTION && BOOST_UNORDERED_TUPLE_ARGS

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        template <typename Alloc, typename T>
        void construct_from_tuple(Alloc&, T* ptr, std::tuple<>)
        {
          new ((void*)ptr) T();
        }

        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 1, std)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 2, std)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 3, std)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 4, std)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 5, std)

#if BOOST_UNORDERED_TUPLE_ARGS >= 6
        BOOST_PP_REPEAT_FROM_TO(6, BOOST_PP_INC(BOOST_UNORDERED_TUPLE_ARGS),
          BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE, std)
#endif
      }
    }
  }
}

#endif

#undef BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE
#undef BOOST_UNORDERED_GET_TUPLE_ARG

// construct_from_tuple for boost::tuple on old versions of sunpro.
//
// Old versions of Sun C++ had problems with template overloads of
// boost::tuple, so to fix it I added a distinct type for each length to
// the overloads. That means there's no possible ambiguity between the
// different overloads, so that the compiler doesn't get confused

#if BOOST_UNORDERED_SUN_WORKAROUNDS1

#define BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(z, n, namespace_)                 \
  template <typename Alloc, typename T,                                        \
    BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                                  \
  void construct_from_tuple_impl(boost::unordered::detail::func::length<n>,    \
    Alloc&, T* ptr,                                                            \
    namespace_::tuple<BOOST_PP_ENUM_PARAMS_Z(z, n, A)> const& x)               \
  {                                                                            \
    new ((void*)ptr)                                                           \
      T(BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_GET_TUPLE_ARG, namespace_));      \
  }

#define BOOST_UNORDERED_GET_TUPLE_ARG(z, n, namespace_) namespace_::get<n>(x)

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        template <int N> struct length
        {
        };

        template <typename Alloc, typename T>
        void construct_from_tuple_impl(
          boost::unordered::detail::func::length<0>, Alloc&, T* ptr,
          boost::tuple<>)
        {
          new ((void*)ptr) T();
        }

        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 1, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 2, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 3, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 4, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 5, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 6, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 7, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 8, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 9, boost)
        BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE(1, 10, boost)

        template <typename Alloc, typename T, typename Tuple>
        void construct_from_tuple(Alloc& alloc, T* ptr, Tuple const& x)
        {
          construct_from_tuple_impl(boost::unordered::detail::func::length<
                                      boost::tuples::length<Tuple>::value>(),
            alloc, ptr, x);
        }
      }
    }
  }
}

#undef BOOST_UNORDERED_CONSTRUCT_FROM_TUPLE
#undef BOOST_UNORDERED_GET_TUPLE_ARG

#endif

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {
        ////////////////////////////////////////////////////////////////////////
        // Trait to check for piecewise construction.

        template <typename A0> struct use_piecewise
        {
          static choice1::type test(
            choice1, boost::unordered::piecewise_construct_t);

          static choice2::type test(choice2, ...);

          enum
          {
            value = sizeof(choice1::type) ==
                    sizeof(test(choose(), boost::unordered::detail::make<A0>()))
          };
        };

#if BOOST_UNORDERED_CXX11_CONSTRUCTION

        ////////////////////////////////////////////////////////////////////////
        // Construct from variadic parameters

        template <typename Alloc, typename T, typename... Args>
        inline void construct_from_args(
          Alloc& alloc, T* address, BOOST_FWD_REF(Args)... args)
        {
          boost::unordered::detail::allocator_traits<Alloc>::construct(
            alloc, address, boost::forward<Args>(args)...);
        }

        // For backwards compatibility, implement a special case for
        // piecewise_construct with boost::tuple

        template <typename A0> struct detect_boost_tuple
        {
          template <typename T0, typename T1, typename T2, typename T3,
            typename T4, typename T5, typename T6, typename T7, typename T8,
            typename T9>
          static choice1::type test(choice1,
            boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> const&);

          static choice2::type test(choice2, ...);

          enum
          {
            value = sizeof(choice1::type) ==
                    sizeof(test(choose(), boost::unordered::detail::make<A0>()))
          };
        };

        // Special case for piecewise_construct

        template <typename Alloc, typename A, typename B, typename A0,
          typename A1, typename A2>
        inline typename boost::enable_if_c<use_piecewise<A0>::value &&
                                             detect_boost_tuple<A1>::value &&
                                             detect_boost_tuple<A2>::value,
          void>::type
        construct_from_args(Alloc& alloc, std::pair<A, B>* address,
          BOOST_FWD_REF(A0), BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
        {
          boost::unordered::detail::func::construct_from_tuple(
            alloc, boost::addressof(address->first), boost::forward<A1>(a1));
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_from_tuple(
              alloc, boost::addressof(address->second), boost::forward<A2>(a2));
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(address->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
        }

#elif !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

        ////////////////////////////////////////////////////////////////////////
        // Construct from variadic parameters

        template <typename Alloc, typename T, typename... Args>
        inline void construct_from_args(
          Alloc&, T* address, BOOST_FWD_REF(Args)... args)
        {
          new ((void*)address) T(boost::forward<Args>(args)...);
        }

        // Special case for piecewise_construct

        template <typename Alloc, typename A, typename B, typename A0,
          typename A1, typename A2>
        inline typename enable_if<use_piecewise<A0>, void>::type
        construct_from_args(Alloc& alloc, std::pair<A, B>* address,
          BOOST_FWD_REF(A0), BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
        {
          boost::unordered::detail::func::construct_from_tuple(
            alloc, boost::addressof(address->first), boost::forward<A1>(a1));
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_from_tuple(
              alloc, boost::addressof(address->second), boost::forward<A2>(a2));
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(address->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
        }

#else // BOOST_NO_CXX11_VARIADIC_TEMPLATES

        ////////////////////////////////////////////////////////////////////////
        // Construct from emplace_args

        // Explicitly write out first three overloads for the sake of sane
        // error messages.

        template <typename Alloc, typename T, typename A0>
        inline void construct_from_args(
          Alloc&, T* address, emplace_args1<A0> const& args)
        {
          new ((void*)address) T(boost::forward<A0>(args.a0));
        }

        template <typename Alloc, typename T, typename A0, typename A1>
        inline void construct_from_args(
          Alloc&, T* address, emplace_args2<A0, A1> const& args)
        {
          new ((void*)address)
            T(boost::forward<A0>(args.a0), boost::forward<A1>(args.a1));
        }

        template <typename Alloc, typename T, typename A0, typename A1,
          typename A2>
        inline void construct_from_args(
          Alloc&, T* address, emplace_args3<A0, A1, A2> const& args)
        {
          new ((void*)address) T(boost::forward<A0>(args.a0),
            boost::forward<A1>(args.a1), boost::forward<A2>(args.a2));
        }

// Use a macro for the rest.

#define BOOST_UNORDERED_CONSTRUCT_IMPL(z, num_params, _)                       \
  template <typename Alloc, typename T,                                        \
    BOOST_PP_ENUM_PARAMS_Z(z, num_params, typename A)>                         \
  inline void construct_from_args(Alloc&, T* address,                          \
    boost::unordered::detail::BOOST_PP_CAT(emplace_args, num_params) <         \
      BOOST_PP_ENUM_PARAMS_Z(z, num_params, A) > const& args)                  \
  {                                                                            \
    new ((void*)address)                                                       \
      T(BOOST_PP_ENUM_##z(num_params, BOOST_UNORDERED_CALL_FORWARD, args.a));  \
  }

        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 4, _)
        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 5, _)
        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 6, _)
        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 7, _)
        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 8, _)
        BOOST_UNORDERED_CONSTRUCT_IMPL(1, 9, _)
        BOOST_PP_REPEAT_FROM_TO(10, BOOST_PP_INC(BOOST_UNORDERED_EMPLACE_LIMIT),
          BOOST_UNORDERED_CONSTRUCT_IMPL, _)

#undef BOOST_UNORDERED_CONSTRUCT_IMPL

        // Construct with piecewise_construct

        template <typename Alloc, typename A, typename B, typename A0,
          typename A1, typename A2>
        inline void construct_from_args(Alloc& alloc, std::pair<A, B>* address,
          boost::unordered::detail::emplace_args3<A0, A1, A2> const& args,
          typename enable_if<use_piecewise<A0>, void*>::type = 0)
        {
          boost::unordered::detail::func::construct_from_tuple(
            alloc, boost::addressof(address->first), args.a1);
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_from_tuple(
              alloc, boost::addressof(address->second), args.a2);
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(address->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
        }

#endif // BOOST_NO_CXX11_VARIADIC_TEMPLATES
      }
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {

      ///////////////////////////////////////////////////////////////////
      //
      // Node construction

      template <typename NodeAlloc> struct node_constructor
      {
        typedef NodeAlloc node_allocator;
        typedef boost::unordered::detail::allocator_traits<NodeAlloc>
          node_allocator_traits;
        typedef typename node_allocator_traits::value_type node;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef typename node::value_type value_type;

        node_allocator& alloc_;
        node_pointer node_;

        node_constructor(node_allocator& n) : alloc_(n), node_() {}

        ~node_constructor();

        void create_node();

        // no throw
        node_pointer release()
        {
          BOOST_ASSERT(node_);
          node_pointer p = node_;
          node_ = node_pointer();
          return p;
        }

        void reclaim(node_pointer p)
        {
          BOOST_ASSERT(!node_);
          node_ = p;
          BOOST_UNORDERED_CALL_DESTROY(
            node_allocator_traits, alloc_, node_->value_ptr());
        }

      private:
        node_constructor(node_constructor const&);
        node_constructor& operator=(node_constructor const&);
      };

      template <typename Alloc> node_constructor<Alloc>::~node_constructor()
      {
        if (node_) {
          boost::unordered::detail::func::destroy(boost::to_address(node_));
          node_allocator_traits::deallocate(alloc_, node_, 1);
        }
      }

      template <typename Alloc> void node_constructor<Alloc>::create_node()
      {
        BOOST_ASSERT(!node_);
        node_ = node_allocator_traits::allocate(alloc_, 1);
        new ((void*)boost::to_address(node_)) node();
      }

      template <typename NodeAlloc> struct node_tmp
      {
        typedef boost::unordered::detail::allocator_traits<NodeAlloc>
          node_allocator_traits;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef typename node_allocator_traits::value_type node;

        NodeAlloc& alloc_;
        node_pointer node_;

        explicit node_tmp(node_pointer n, NodeAlloc& a) : alloc_(a), node_(n) {}

        ~node_tmp();

        // no throw
        node_pointer release()
        {
          node_pointer p = node_;
          node_ = node_pointer();
          return p;
        }
      };

      template <typename Alloc> node_tmp<Alloc>::~node_tmp()
      {
        if (node_) {
          BOOST_UNORDERED_CALL_DESTROY(
            node_allocator_traits, alloc_, node_->value_ptr());
          boost::unordered::detail::func::destroy(boost::to_address(node_));
          node_allocator_traits::deallocate(alloc_, node_, 1);
        }
      }
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {
      namespace func {

        // Some nicer construct_node functions, might try to
        // improve implementation later.

        template <typename Alloc, BOOST_UNORDERED_EMPLACE_TEMPLATE>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_from_args(Alloc& alloc, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          construct_from_args(
            alloc, a.node_->value_ptr(), BOOST_UNORDERED_EMPLACE_FORWARD);
          return a.release();
        }

        template <typename Alloc, typename U>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node(Alloc& alloc, BOOST_FWD_REF(U) x)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          BOOST_UNORDERED_CALL_CONSTRUCT1(
            boost::unordered::detail::allocator_traits<Alloc>, alloc,
            a.node_->value_ptr(), boost::forward<U>(x));
          return a.release();
        }

#if BOOST_UNORDERED_CXX11_CONSTRUCTION

        template <typename Alloc, typename Key>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair(Alloc& alloc, BOOST_FWD_REF(Key) k)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          boost::unordered::detail::allocator_traits<Alloc>::construct(alloc,
            a.node_->value_ptr(), std::piecewise_construct,
            std::forward_as_tuple(boost::forward<Key>(k)),
            std::forward_as_tuple());
          return a.release();
        }

        template <typename Alloc, typename Key, typename Mapped>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair(
            Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_FWD_REF(Mapped) m)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          boost::unordered::detail::allocator_traits<Alloc>::construct(alloc,
            a.node_->value_ptr(), std::piecewise_construct,
            std::forward_as_tuple(boost::forward<Key>(k)),
            std::forward_as_tuple(boost::forward<Mapped>(m)));
          return a.release();
        }

        template <typename Alloc, typename Key, typename... Args>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair_from_args(
            Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_FWD_REF(Args)... args)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
#if !(BOOST_COMP_CLANG && BOOST_COMP_CLANG < BOOST_VERSION_NUMBER(3, 8, 0) &&  \
      defined(BOOST_LIBSTDCXX11))
          boost::unordered::detail::allocator_traits<Alloc>::construct(alloc,
            a.node_->value_ptr(), std::piecewise_construct,
            std::forward_as_tuple(boost::forward<Key>(k)),
            std::forward_as_tuple(boost::forward<Args>(args)...));
#else
          // It doesn't seem to be possible to construct a tuple with 3 variadic
          // rvalue reference members when using older versions of clang with
          // libstdc++, so just use std::make_tuple instead of
          // std::forward_as_tuple.
          boost::unordered::detail::allocator_traits<Alloc>::construct(alloc,
            a.node_->value_ptr(), std::piecewise_construct,
            std::forward_as_tuple(boost::forward<Key>(k)),
            std::make_tuple(boost::forward<Args>(args)...));
#endif
          return a.release();
        }

#else

        template <typename Alloc, typename Key>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair(Alloc& alloc, BOOST_FWD_REF(Key) k)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          boost::unordered::detail::func::construct_value(
            boost::addressof(a.node_->value_ptr()->first),
            boost::forward<Key>(k));
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_value(
              boost::addressof(a.node_->value_ptr()->second));
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(a.node_->value_ptr()->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          return a.release();
        }

        template <typename Alloc, typename Key, typename Mapped>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair(
            Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_FWD_REF(Mapped) m)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          boost::unordered::detail::func::construct_value(
            boost::addressof(a.node_->value_ptr()->first),
            boost::forward<Key>(k));
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_value(
              boost::addressof(a.node_->value_ptr()->second),
              boost::forward<Mapped>(m));
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(a.node_->value_ptr()->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          return a.release();
        }

        template <typename Alloc, typename Key,
          BOOST_UNORDERED_EMPLACE_TEMPLATE>
        inline
          typename boost::unordered::detail::allocator_traits<Alloc>::pointer
          construct_node_pair_from_args(
            Alloc& alloc, BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          node_constructor<Alloc> a(alloc);
          a.create_node();
          boost::unordered::detail::func::construct_value(
            boost::addressof(a.node_->value_ptr()->first),
            boost::forward<Key>(k));
          BOOST_TRY
          {
            boost::unordered::detail::func::construct_from_args(alloc,
              boost::addressof(a.node_->value_ptr()->second),
              BOOST_UNORDERED_EMPLACE_FORWARD);
          }
          BOOST_CATCH(...)
          {
            boost::unordered::detail::func::destroy(
              boost::addressof(a.node_->value_ptr()->first));
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          return a.release();
        }

#endif
      }
    }
  }
}

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

// The 'iterator_detail' namespace was a misguided attempt at avoiding ADL
// in the detail namespace. It didn't work because the template parameters
// were in detail. I'm not changing it at the moment to be safe. I might
// do in the future if I change the iterator types.
namespace boost {
  namespace unordered {
    namespace iterator_detail {

      //////////////////////////////////////////////////////////////////////////
      // Iterators
      //
      // all no throw

      template <typename Node> struct l_iterator
      {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename Node2>
        friend struct boost::unordered::iterator_detail::cl_iterator;

      private:
#endif
        typedef typename Node::node_pointer node_pointer;
        node_pointer ptr_;
        std::size_t bucket_;
        std::size_t bucket_count_;

      public:
        typedef typename Node::value_type element_type;
        typedef typename Node::value_type value_type;
        typedef value_type* pointer;
        typedef value_type& reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::forward_iterator_tag iterator_category;

        l_iterator() BOOST_NOEXCEPT : ptr_() {}

        l_iterator(node_pointer n, std::size_t b, std::size_t c) BOOST_NOEXCEPT
          : ptr_(n),
            bucket_(b),
            bucket_count_(c)
        {
        }

        value_type& operator*() const { return ptr_->value(); }

        value_type* operator->() const { return ptr_->value_ptr(); }

        l_iterator& operator++()
        {
          ptr_ = static_cast<node_pointer>(ptr_->next_);
          if (ptr_ && ptr_->get_bucket() != bucket_)
            ptr_ = node_pointer();
          return *this;
        }

        l_iterator operator++(int)
        {
          l_iterator tmp(*this);
          ++(*this);
          return tmp;
        }

        bool operator==(l_iterator x) const BOOST_NOEXCEPT
        {
          return ptr_ == x.ptr_;
        }

        bool operator!=(l_iterator x) const BOOST_NOEXCEPT
        {
          return ptr_ != x.ptr_;
        }
      };

      template <typename Node> struct cl_iterator
      {
        friend struct boost::unordered::iterator_detail::l_iterator<Node>;

      private:
        typedef typename Node::node_pointer node_pointer;
        node_pointer ptr_;
        std::size_t bucket_;
        std::size_t bucket_count_;

      public:
        typedef typename Node::value_type const element_type;
        typedef typename Node::value_type value_type;
        typedef value_type const* pointer;
        typedef value_type const& reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::forward_iterator_tag iterator_category;

        cl_iterator() BOOST_NOEXCEPT : ptr_() {}

        cl_iterator(node_pointer n, std::size_t b, std::size_t c) BOOST_NOEXCEPT
          : ptr_(n),
            bucket_(b),
            bucket_count_(c)
        {
        }

        cl_iterator(
          boost::unordered::iterator_detail::l_iterator<Node> const& x)
          BOOST_NOEXCEPT : ptr_(x.ptr_),
                           bucket_(x.bucket_),
                           bucket_count_(x.bucket_count_)
        {
        }

        value_type const& operator*() const { return ptr_->value(); }

        value_type const* operator->() const { return ptr_->value_ptr(); }

        cl_iterator& operator++()
        {
          ptr_ = static_cast<node_pointer>(ptr_->next_);
          if (ptr_ && ptr_->get_bucket() != bucket_)
            ptr_ = node_pointer();
          return *this;
        }

        cl_iterator operator++(int)
        {
          cl_iterator tmp(*this);
          ++(*this);
          return tmp;
        }

        friend bool operator==(
          cl_iterator const& x, cl_iterator const& y) BOOST_NOEXCEPT
        {
          return x.ptr_ == y.ptr_;
        }

        friend bool operator!=(
          cl_iterator const& x, cl_iterator const& y) BOOST_NOEXCEPT
        {
          return x.ptr_ != y.ptr_;
        }
      };

      template <typename Node> struct iterator
      {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename>
        friend struct boost::unordered::iterator_detail::c_iterator;
        template <typename> friend struct boost::unordered::detail::table;

      private:
#endif
        typedef typename Node::node_pointer node_pointer;
        node_pointer node_;

      public:
        typedef typename Node::value_type element_type;
        typedef typename Node::value_type value_type;
        typedef value_type* pointer;
        typedef value_type& reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::forward_iterator_tag iterator_category;

        iterator() BOOST_NOEXCEPT : node_() {}

        explicit iterator(typename Node::link_pointer x) BOOST_NOEXCEPT
          : node_(static_cast<node_pointer>(x))
        {
        }

        value_type& operator*() const { return node_->value(); }

        value_type* operator->() const { return node_->value_ptr(); }

        iterator& operator++()
        {
          node_ = static_cast<node_pointer>(node_->next_);
          return *this;
        }

        iterator operator++(int)
        {
          iterator tmp(node_);
          node_ = static_cast<node_pointer>(node_->next_);
          return tmp;
        }

        bool operator==(iterator const& x) const BOOST_NOEXCEPT
        {
          return node_ == x.node_;
        }

        bool operator!=(iterator const& x) const BOOST_NOEXCEPT
        {
          return node_ != x.node_;
        }
      };

      template <typename Node> struct c_iterator
      {
        friend struct boost::unordered::iterator_detail::iterator<Node>;

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename> friend struct boost::unordered::detail::table;

      private:
#endif
        typedef typename Node::node_pointer node_pointer;
        typedef boost::unordered::iterator_detail::iterator<Node> n_iterator;
        node_pointer node_;

      public:
        typedef typename Node::value_type const element_type;
        typedef typename Node::value_type value_type;
        typedef value_type const* pointer;
        typedef value_type const& reference;
        typedef std::ptrdiff_t difference_type;
        typedef std::forward_iterator_tag iterator_category;

        c_iterator() BOOST_NOEXCEPT : node_() {}

        explicit c_iterator(typename Node::link_pointer x) BOOST_NOEXCEPT
          : node_(static_cast<node_pointer>(x))
        {
        }

        c_iterator(n_iterator const& x) BOOST_NOEXCEPT : node_(x.node_) {}

        value_type const& operator*() const { return node_->value(); }

        value_type const* operator->() const { return node_->value_ptr(); }

        c_iterator& operator++()
        {
          node_ = static_cast<node_pointer>(node_->next_);
          return *this;
        }

        c_iterator operator++(int)
        {
          c_iterator tmp(node_);
          node_ = static_cast<node_pointer>(node_->next_);
          return tmp;
        }

        friend bool operator==(
          c_iterator const& x, c_iterator const& y) BOOST_NOEXCEPT
        {
          return x.node_ == y.node_;
        }

        friend bool operator!=(
          c_iterator const& x, c_iterator const& y) BOOST_NOEXCEPT
        {
          return x.node_ != y.node_;
        }
      };
    }
  }
}

namespace boost {
  namespace unordered {
    namespace detail {

      ///////////////////////////////////////////////////////////////////
      //
      // Node Holder
      //
      // Temporary store for nodes. Deletes any that aren't used.

      template <typename NodeAlloc> struct node_holder
      {
      private:
        typedef NodeAlloc node_allocator;
        typedef boost::unordered::detail::allocator_traits<NodeAlloc>
          node_allocator_traits;
        typedef typename node_allocator_traits::value_type node;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef typename node::value_type value_type;
        typedef typename node::link_pointer link_pointer;
        typedef boost::unordered::iterator_detail::iterator<node> iterator;

        node_constructor<NodeAlloc> constructor_;
        node_pointer nodes_;

      public:
        template <typename Table>
        explicit node_holder(Table& b) : constructor_(b.node_alloc()), nodes_()
        {
          if (b.size_) {
            typename Table::link_pointer prev = b.get_previous_start();
            nodes_ = static_cast<node_pointer>(prev->next_);
            prev->next_ = link_pointer();
            b.size_ = 0;
          }
        }

        ~node_holder();

        node_pointer pop_node()
        {
          node_pointer n = nodes_;
          nodes_ = static_cast<node_pointer>(nodes_->next_);
          n->next_ = link_pointer();
          return n;
        }

        template <typename T> inline node_pointer copy_of(T const& v)
        {
          if (nodes_) {
            constructor_.reclaim(pop_node());
          } else {
            constructor_.create_node();
          }
          BOOST_UNORDERED_CALL_CONSTRUCT1(node_allocator_traits,
            constructor_.alloc_, constructor_.node_->value_ptr(), v);
          return constructor_.release();
        }

        template <typename T> inline node_pointer move_copy_of(T& v)
        {
          if (nodes_) {
            constructor_.reclaim(pop_node());
          } else {
            constructor_.create_node();
          }
          BOOST_UNORDERED_CALL_CONSTRUCT1(node_allocator_traits,
            constructor_.alloc_, constructor_.node_->value_ptr(),
            boost::move(v));
          return constructor_.release();
        }

        iterator begin() const { return iterator(nodes_); }
      };

      template <typename Alloc> node_holder<Alloc>::~node_holder()
      {
        while (nodes_) {
          node_pointer p = nodes_;
          nodes_ = static_cast<node_pointer>(p->next_);

          BOOST_UNORDERED_CALL_DESTROY(
            node_allocator_traits, constructor_.alloc_, p->value_ptr());
          boost::unordered::detail::func::destroy(boost::to_address(p));
          node_allocator_traits::deallocate(constructor_.alloc_, p, 1);
        }
      }

      ///////////////////////////////////////////////////////////////////
      //
      // Bucket

      template <typename NodePointer> struct bucket
      {
        typedef NodePointer link_pointer;
        link_pointer next_;

        bucket() : next_() {}
        bucket(link_pointer n) : next_(n) {}

        link_pointer first_from_start() { return next_; }

        enum
        {
          extra_node = true
        };
      };

      struct ptr_bucket
      {
        typedef ptr_bucket* link_pointer;
        link_pointer next_;

        ptr_bucket() : next_(0) {}
        ptr_bucket(link_pointer n) : next_(n) {}

        link_pointer first_from_start() { return this; }

        enum
        {
          extra_node = false
        };
      };

      ///////////////////////////////////////////////////////////////////
      //
      // Hash Policy

      template <typename SizeT> struct prime_policy
      {
        template <typename Hash, typename T>
        static inline SizeT apply_hash(Hash const& hf, T const& x)
        {
          return hf(x);
        }

        static inline SizeT to_bucket(SizeT bucket_count, SizeT hash)
        {
          return hash % bucket_count;
        }

        static inline SizeT new_bucket_count(SizeT min)
        {
          return boost::unordered::detail::next_prime(min);
        }

        static inline SizeT prev_bucket_count(SizeT max)
        {
          return boost::unordered::detail::prev_prime(max);
        }
      };

      template <typename SizeT> struct mix64_policy
      {
        template <typename Hash, typename T>
        static inline SizeT apply_hash(Hash const& hf, T const& x)
        {
          SizeT key = hf(x);
          key = (~key) + (key << 21); // key = (key << 21) - key - 1;
          key = key ^ (key >> 24);
          key = (key + (key << 3)) + (key << 8); // key * 265
          key = key ^ (key >> 14);
          key = (key + (key << 2)) + (key << 4); // key * 21
          key = key ^ (key >> 28);
          key = key + (key << 31);
          return key;
        }

        static inline SizeT to_bucket(SizeT bucket_count, SizeT hash)
        {
          return hash & (bucket_count - 1);
        }

        static inline SizeT new_bucket_count(SizeT min)
        {
          if (min <= 4)
            return 4;
          --min;
          min |= min >> 1;
          min |= min >> 2;
          min |= min >> 4;
          min |= min >> 8;
          min |= min >> 16;
          min |= min >> 32;
          return min + 1;
        }

        static inline SizeT prev_bucket_count(SizeT max)
        {
          max |= max >> 1;
          max |= max >> 2;
          max |= max >> 4;
          max |= max >> 8;
          max |= max >> 16;
          max |= max >> 32;
          return (max >> 1) + 1;
        }
      };

      template <int digits, int radix> struct pick_policy_impl
      {
        typedef prime_policy<std::size_t> type;
      };

      template <> struct pick_policy_impl<64, 2>
      {
        typedef mix64_policy<std::size_t> type;
      };

      template <typename T>
      struct pick_policy2
        : pick_policy_impl<std::numeric_limits<std::size_t>::digits,
            std::numeric_limits<std::size_t>::radix>
      {
      };

      // While the mix policy is generally faster, the prime policy is a lot
      // faster when a large number consecutive integers are used, because
      // there are no collisions. Since that is probably quite common, use
      // prime policy for integeral types. But not the smaller ones, as they
      // don't have enough unique values for this to be an issue.

      template <> struct pick_policy2<int>
      {
        typedef prime_policy<std::size_t> type;
      };

      template <> struct pick_policy2<unsigned int>
      {
        typedef prime_policy<std::size_t> type;
      };

      template <> struct pick_policy2<long>
      {
        typedef prime_policy<std::size_t> type;
      };

      template <> struct pick_policy2<unsigned long>
      {
        typedef prime_policy<std::size_t> type;
      };

#if !defined(BOOST_NO_LONG_LONG)
      template <> struct pick_policy2<boost::long_long_type>
      {
        typedef prime_policy<std::size_t> type;
      };

      template <> struct pick_policy2<boost::ulong_long_type>
      {
        typedef prime_policy<std::size_t> type;
      };
#endif

      template <typename T>
      struct pick_policy : pick_policy2<typename boost::remove_cv<T>::type>
      {
      };

      //////////////////////////////////////////////////////////////////////////
      // Functions
      //
      // This double buffers the storage for the hash function and key equality
      // predicate in order to have exception safe copy/swap. To do so,
      // use 'construct_spare' to construct in the spare space, and then when
      // ready to use 'switch_functions' to switch to the new functions.
      // If an exception is thrown between these two calls, use
      // 'cleanup_spare_functions' to destroy the unused constructed functions.

      template <class H, class P> class functions
      {
      public:
        static const bool nothrow_move_assignable =
          boost::is_nothrow_move_assignable<H>::value &&
          boost::is_nothrow_move_assignable<P>::value;
        static const bool nothrow_move_constructible =
          boost::is_nothrow_move_constructible<H>::value &&
          boost::is_nothrow_move_constructible<P>::value;
        static const bool nothrow_swappable =
          boost::is_nothrow_swappable<H>::value &&
          boost::is_nothrow_swappable<P>::value;

      private:
        functions& operator=(functions const&);

        typedef compressed<H, P> function_pair;

        typedef typename boost::aligned_storage<sizeof(function_pair),
          boost::alignment_of<function_pair>::value>::type aligned_function;

        unsigned char current_; // 0/1 - Currently active functions
                                // +2 - Both constructed
        aligned_function funcs_[2];

      public:
        functions(H const& hf, P const& eq) : current_(0)
        {
          construct_functions(current_, hf, eq);
        }

        functions(functions const& bf) : current_(0)
        {
          construct_functions(current_, bf.current_functions());
        }

        functions(functions& bf, boost::unordered::detail::move_tag)
            : current_(0)
        {
          construct_functions(current_, bf.current_functions(),
            boost::unordered::detail::integral_constant<bool,
              nothrow_move_constructible>());
        }

        ~functions()
        {
          BOOST_ASSERT(!(current_ & 2));
          destroy_functions(current_);
        }

        H const& hash_function() const { return current_functions().first(); }

        P const& key_eq() const { return current_functions().second(); }

        function_pair const& current_functions() const
        {
          return *static_cast<function_pair const*>(
            static_cast<void const*>(funcs_[current_ & 1].address()));
        }

        function_pair& current_functions()
        {
          return *static_cast<function_pair*>(
            static_cast<void*>(funcs_[current_ & 1].address()));
        }

        void construct_spare_functions(function_pair const& f)
        {
          BOOST_ASSERT(!(current_ & 2));
          construct_functions(current_ ^ 1, f);
          current_ |= 2;
        }

        void cleanup_spare_functions()
        {
          if (current_ & 2) {
            current_ = static_cast<unsigned char>(current_ & 1);
            destroy_functions(current_ ^ 1);
          }
        }

        void switch_functions()
        {
          BOOST_ASSERT(current_ & 2);
          destroy_functions(static_cast<unsigned char>(current_ & 1));
          current_ ^= 3;
        }

      private:
        void construct_functions(unsigned char which, H const& hf, P const& eq)
        {
          BOOST_ASSERT(!(which & 2));
          new ((void*)&funcs_[which]) function_pair(hf, eq);
        }

        void construct_functions(unsigned char which, function_pair const& f,
          boost::unordered::detail::false_type =
            boost::unordered::detail::false_type())
        {
          BOOST_ASSERT(!(which & 2));
          new ((void*)&funcs_[which]) function_pair(f);
        }

        void construct_functions(unsigned char which, function_pair& f,
          boost::unordered::detail::true_type)
        {
          BOOST_ASSERT(!(which & 2));
          new ((void*)&funcs_[which])
            function_pair(f, boost::unordered::detail::move_tag());
        }

        void destroy_functions(unsigned char which)
        {
          BOOST_ASSERT(!(which & 2));
          boost::unordered::detail::func::destroy(
            (function_pair*)(&funcs_[which]));
        }
      };

////////////////////////////////////////////////////////////////////////////
// rvalue parameters when type can't be a BOOST_RV_REF(T) parameter
// e.g. for int

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#define BOOST_UNORDERED_RV_REF(T) BOOST_RV_REF(T)
#else
      struct please_ignore_this_overload
      {
        typedef please_ignore_this_overload type;
      };

      template <typename T> struct rv_ref_impl
      {
        typedef BOOST_RV_REF(T) type;
      };

      template <typename T>
      struct rv_ref
        : boost::detail::if_true<boost::is_class<T>::value>::
            BOOST_NESTED_TEMPLATE then<boost::unordered::detail::rv_ref_impl<T>,
              please_ignore_this_overload>::type
      {
      };

#define BOOST_UNORDERED_RV_REF(T)                                              \
  typename boost::unordered::detail::rv_ref<T>::type
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

      //////////////////////////////////////////////////////////////////////////
      // convert double to std::size_t

      inline std::size_t double_to_size(double f)
      {
        return f >= static_cast<double>(
                      (std::numeric_limits<std::size_t>::max)())
                 ? (std::numeric_limits<std::size_t>::max)()
                 : static_cast<std::size_t>(f);
      }

      template <typename Types>
      struct table : boost::unordered::detail::functions<typename Types::hasher,
                       typename Types::key_equal>
      {
      private:
        table(table const&);
        table& operator=(table const&);

      public:
        typedef typename Types::node node;
        typedef typename Types::bucket bucket;
        typedef typename Types::hasher hasher;
        typedef typename Types::key_equal key_equal;
        typedef typename Types::const_key_type const_key_type;
        typedef typename Types::extractor extractor;
        typedef typename Types::value_type value_type;
        typedef typename Types::table table_impl;
        typedef typename Types::link_pointer link_pointer;
        typedef typename Types::policy policy;
        typedef typename Types::iterator iterator;
        typedef typename Types::c_iterator c_iterator;
        typedef typename Types::l_iterator l_iterator;
        typedef typename Types::cl_iterator cl_iterator;

        typedef boost::unordered::detail::functions<typename Types::hasher,
          typename Types::key_equal>
          functions;

        typedef typename Types::value_allocator value_allocator;
        typedef typename boost::unordered::detail::rebind_wrap<value_allocator,
          node>::type node_allocator;
        typedef typename boost::unordered::detail::rebind_wrap<value_allocator,
          bucket>::type bucket_allocator;
        typedef boost::unordered::detail::allocator_traits<node_allocator>
          node_allocator_traits;
        typedef boost::unordered::detail::allocator_traits<bucket_allocator>
          bucket_allocator_traits;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef
          typename node_allocator_traits::const_pointer const_node_pointer;
        typedef typename bucket_allocator_traits::pointer bucket_pointer;
        typedef boost::unordered::detail::node_constructor<node_allocator>
          node_constructor;
        typedef boost::unordered::detail::node_tmp<node_allocator> node_tmp;

        typedef std::pair<iterator, bool> emplace_return;

        ////////////////////////////////////////////////////////////////////////
        // Members

        boost::unordered::detail::compressed<bucket_allocator, node_allocator>
          allocators_;
        std::size_t bucket_count_;
        std::size_t size_;
        float mlf_;
        std::size_t max_load_;
        bucket_pointer buckets_;

        ////////////////////////////////////////////////////////////////////////
        // Data access

        static node_pointer get_node(c_iterator it) { return it.node_; }

        static node_pointer next_node(link_pointer n)
        {
          return static_cast<node_pointer>(n->next_);
        }

        static node_pointer next_for_find(link_pointer n)
        {
          node_pointer n2 = static_cast<node_pointer>(n);
          do {
            n2 = next_node(n2);
          } while (n2 && !n2->is_first_in_group());
          return n2;
        }

        node_pointer next_group(node_pointer n) const
        {
          node_pointer n1 = n;
          do {
            n1 = next_node(n1);
          } while (n1 && !n1->is_first_in_group());
          return n1;
        }

        std::size_t group_count(node_pointer n) const
        {
          std::size_t x = 0;
          node_pointer it = n;
          do {
            ++x;
            it = next_node(it);
          } while (it && !it->is_first_in_group());

          return x;
        }

        std::size_t node_bucket(node_pointer n) const
        {
          return n->get_bucket();
        }

        bucket_allocator const& bucket_alloc() const
        {
          return allocators_.first();
        }

        node_allocator const& node_alloc() const
        {
          return allocators_.second();
        }

        bucket_allocator& bucket_alloc() { return allocators_.first(); }

        node_allocator& node_alloc() { return allocators_.second(); }

        std::size_t max_bucket_count() const
        {
          // -1 to account for the start bucket.
          return policy::prev_bucket_count(
            bucket_allocator_traits::max_size(bucket_alloc()) - 1);
        }

        bucket_pointer get_bucket_pointer(std::size_t bucket_index) const
        {
          BOOST_ASSERT(buckets_);
          return buckets_ + static_cast<std::ptrdiff_t>(bucket_index);
        }

        link_pointer get_previous_start() const
        {
          return get_bucket_pointer(bucket_count_)->first_from_start();
        }

        link_pointer get_previous_start(std::size_t bucket_index) const
        {
          return get_bucket_pointer(bucket_index)->next_;
        }

        node_pointer begin() const
        {
          return size_ ? next_node(get_previous_start()) : node_pointer();
        }

        node_pointer begin(std::size_t bucket_index) const
        {
          if (!size_)
            return node_pointer();
          link_pointer prev = get_previous_start(bucket_index);
          return prev ? next_node(prev) : node_pointer();
        }

        std::size_t hash_to_bucket(std::size_t hash_value) const
        {
          return policy::to_bucket(bucket_count_, hash_value);
        }

        std::size_t bucket_size(std::size_t index) const
        {
          node_pointer n = begin(index);
          if (!n)
            return 0;

          std::size_t count = 0;
          while (n && node_bucket(n) == index) {
            ++count;
            n = next_node(n);
          }

          return count;
        }

        ////////////////////////////////////////////////////////////////////////
        // Load methods

        void recalculate_max_load()
        {
          using namespace std;

          // From 6.3.1/13:
          // Only resize when size >= mlf_ * count
          max_load_ = buckets_ ? boost::unordered::detail::double_to_size(
                                   ceil(static_cast<double>(mlf_) *
                                        static_cast<double>(bucket_count_)))
                               : 0;
        }

        void max_load_factor(float z)
        {
          BOOST_ASSERT(z > 0);
          mlf_ = (std::max)(z, minimum_max_load_factor);
          recalculate_max_load();
        }

        std::size_t min_buckets_for_size(std::size_t size) const
        {
          BOOST_ASSERT(mlf_ >= minimum_max_load_factor);

          using namespace std;

          // From insert/emplace requirements:
          //
          // size <= mlf_ * count
          // => count >= size / mlf_
          //
          // Or from rehash post-condition:
          //
          // count >= size / mlf_

          return policy::new_bucket_count(
            boost::unordered::detail::double_to_size(
              floor(static_cast<double>(size) / static_cast<double>(mlf_)) +
              1));
        }

        ////////////////////////////////////////////////////////////////////////
        // Constructors

        table(std::size_t num_buckets, hasher const& hf, key_equal const& eq,
          node_allocator const& a)
            : functions(hf, eq), allocators_(a, a),
              bucket_count_(policy::new_bucket_count(num_buckets)), size_(0),
              mlf_(1.0f), max_load_(0), buckets_()
        {
        }

        table(table const& x, node_allocator const& a)
            : functions(x), allocators_(a, a),
              bucket_count_(x.min_buckets_for_size(x.size_)), size_(0),
              mlf_(x.mlf_), max_load_(0), buckets_()
        {
        }

        table(table& x, boost::unordered::detail::move_tag m)
            : functions(x, m), allocators_(x.allocators_, m),
              bucket_count_(x.bucket_count_), size_(x.size_), mlf_(x.mlf_),
              max_load_(x.max_load_), buckets_(x.buckets_)
        {
          x.buckets_ = bucket_pointer();
          x.size_ = 0;
          x.max_load_ = 0;
        }

        table(table& x, node_allocator const& a,
          boost::unordered::detail::move_tag m)
            : functions(x, m), allocators_(a, a),
              bucket_count_(x.bucket_count_), size_(0), mlf_(x.mlf_),
              max_load_(0), buckets_()
        {
        }

        ////////////////////////////////////////////////////////////////////////
        // Clear buckets and Create buckets
        //
        // IMPORTANT: If the container already contains any elements, the
        //            buckets will not contain any links to them. This will
        //            need to be dealt with, for example by:
        //            - deleting them
        //            - putting them in a 'node_holder' for future use
        //              (as in assignment)
        //            - placing them in buckets (see rehash_impl)

        // Clear the bucket pointers.
        void clear_buckets()
        {
          bucket_pointer end = get_bucket_pointer(bucket_count_);
          for (bucket_pointer it = buckets_; it != end; ++it) {
            it->next_ = node_pointer();
          }
        }

        // Create container buckets. If the container already contains any
        // buckets
        // the linked list will be transferred to the new buckets, but none
        // of the bucket pointers will be set. See above note.
        //
        // Strong exception safety.
        void create_buckets(std::size_t new_count)
        {
          link_pointer dummy_node;

          // Construct the new buckets and dummy node, and destroy the old
          // buckets
          if (buckets_) {
            dummy_node =
              (buckets_ + static_cast<std::ptrdiff_t>(bucket_count_))->next_;
            bucket_pointer new_buckets =
              bucket_allocator_traits::allocate(bucket_alloc(), new_count + 1);
            destroy_buckets();
            buckets_ = new_buckets;
          } else if (bucket::extra_node) {
            node_constructor a(node_alloc());
            a.create_node();
            buckets_ =
              bucket_allocator_traits::allocate(bucket_alloc(), new_count + 1);
            dummy_node = a.release();
          } else {
            dummy_node = link_pointer();
            buckets_ =
              bucket_allocator_traits::allocate(bucket_alloc(), new_count + 1);
          }

          // nothrow from here...
          bucket_count_ = new_count;
          recalculate_max_load();

          bucket_pointer end =
            buckets_ + static_cast<std::ptrdiff_t>(new_count);
          for (bucket_pointer i = buckets_; i != end; ++i) {
            new ((void*)boost::to_address(i)) bucket();
          }
          new ((void*)boost::to_address(end)) bucket(dummy_node);
        }

        ////////////////////////////////////////////////////////////////////////
        // Swap and Move

        void swap_allocators(table& other, false_type)
        {
          boost::unordered::detail::func::ignore_unused_variable_warning(other);

          // According to 23.2.1.8, if propagate_on_container_swap is
          // false the behaviour is undefined unless the allocators
          // are equal.
          BOOST_ASSERT(node_alloc() == other.node_alloc());
        }

        void swap_allocators(table& other, true_type)
        {
          allocators_.swap(other.allocators_);
        }

        // Not nothrow swappable
        void swap(table& x, false_type)
        {
          if (this == &x) {
            return;
          }

          this->construct_spare_functions(x.current_functions());
          BOOST_TRY { x.construct_spare_functions(this->current_functions()); }
          BOOST_CATCH(...)
          {
            this->cleanup_spare_functions();
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          this->switch_functions();
          x.switch_functions();

          swap_allocators(
            x, boost::unordered::detail::integral_constant<bool,
                 allocator_traits<
                   node_allocator>::propagate_on_container_swap::value>());

          boost::swap(buckets_, x.buckets_);
          boost::swap(bucket_count_, x.bucket_count_);
          boost::swap(size_, x.size_);
          std::swap(mlf_, x.mlf_);
          std::swap(max_load_, x.max_load_);
        }

        // Nothrow swappable
        void swap(table& x, true_type)
        {
          swap_allocators(
            x, boost::unordered::detail::integral_constant<bool,
                 allocator_traits<
                   node_allocator>::propagate_on_container_swap::value>());

          boost::swap(buckets_, x.buckets_);
          boost::swap(bucket_count_, x.bucket_count_);
          boost::swap(size_, x.size_);
          std::swap(mlf_, x.mlf_);
          std::swap(max_load_, x.max_load_);
          this->current_functions().swap(x.current_functions());
        }

        // Only swaps the allocators if propagate_on_container_swap.
        // If not propagate_on_container_swap and allocators aren't
        // equal, behaviour is undefined.
        void swap(table& x)
        {
          BOOST_ASSERT(allocator_traits<
                         node_allocator>::propagate_on_container_swap::value ||
                       node_alloc() == x.node_alloc());
          swap(x, boost::unordered::detail::integral_constant<bool,
                    functions::nothrow_swappable>());
        }

        // Only call with nodes allocated with the currect allocator, or
        // one that is equal to it. (Can't assert because other's
        // allocators might have already been moved).
        void move_buckets_from(table& other)
        {
          BOOST_ASSERT(!buckets_);
          buckets_ = other.buckets_;
          bucket_count_ = other.bucket_count_;
          size_ = other.size_;
          max_load_ = other.max_load_;
          other.buckets_ = bucket_pointer();
          other.size_ = 0;
          other.max_load_ = 0;
        }

        // For use in the constructor when allocators might be different.
        void move_construct_buckets(table& src)
        {
          if (this->node_alloc() == src.node_alloc()) {
            move_buckets_from(src);
          } else {
            this->create_buckets(this->bucket_count_);
            link_pointer prev = this->get_previous_start();
            std::size_t last_bucket = this->bucket_count_;
            for (node_pointer n = src.begin(); n; n = next_node(n)) {
              std::size_t n_bucket = n->get_bucket();
              if (n_bucket != last_bucket) {
                this->get_bucket_pointer(n_bucket)->next_ = prev;
              }
              node_pointer n2 = boost::unordered::detail::func::construct_node(
                this->node_alloc(), boost::move(n->value()));
              n2->bucket_info_ = n->bucket_info_;
              prev->next_ = n2;
              ++size_;
              prev = n2;
              last_bucket = n_bucket;
            }
          }
        }

        ////////////////////////////////////////////////////////////////////////
        // Delete/destruct

        ~table() { delete_buckets(); }

        void destroy_node(node_pointer n)
        {
          BOOST_UNORDERED_CALL_DESTROY(
            node_allocator_traits, node_alloc(), n->value_ptr());
          boost::unordered::detail::func::destroy(boost::to_address(n));
          node_allocator_traits::deallocate(node_alloc(), n, 1);
        }

        void delete_buckets()
        {
          if (buckets_) {
            node_pointer n = static_cast<node_pointer>(
              get_bucket_pointer(bucket_count_)->next_);

            if (bucket::extra_node) {
              node_pointer next = next_node(n);
              boost::unordered::detail::func::destroy(boost::to_address(n));
              node_allocator_traits::deallocate(node_alloc(), n, 1);
              n = next;
            }

            while (n) {
              node_pointer next = next_node(n);
              destroy_node(n);
              n = next;
            }

            destroy_buckets();
            buckets_ = bucket_pointer();
            max_load_ = 0;
            size_ = 0;
          }
        }

        void destroy_buckets()
        {
          bucket_pointer end = get_bucket_pointer(bucket_count_ + 1);
          for (bucket_pointer it = buckets_; it != end; ++it) {
            boost::unordered::detail::func::destroy(boost::to_address(it));
          }

          bucket_allocator_traits::deallocate(
            bucket_alloc(), buckets_, bucket_count_ + 1);
        }

        ////////////////////////////////////////////////////////////////////////
        // Fix buckets after delete/extract
        //
        // (prev,next) should mark an open range of nodes in a single bucket
        // which
        // have either been unlinked, or are about to be.

        std::size_t fix_bucket(
          std::size_t bucket_index, link_pointer prev, node_pointer next)
        {
          std::size_t bucket_index2 = bucket_index;

          if (next) {
            bucket_index2 = node_bucket(next);

            // If next is in the same bucket, then there's nothing to do.
            if (bucket_index == bucket_index2) {
              return bucket_index2;
            }

            // Update the bucket containing next.
            get_bucket_pointer(bucket_index2)->next_ = prev;
          }

          // Check if this bucket is now empty.
          bucket_pointer this_bucket = get_bucket_pointer(bucket_index);
          if (this_bucket->next_ == prev) {
            this_bucket->next_ = link_pointer();
          }

          return bucket_index2;
        }

        ////////////////////////////////////////////////////////////////////////
        // Clear

        void clear_impl();

        ////////////////////////////////////////////////////////////////////////
        // Assignment

        template <typename UniqueType>
        void assign(table const& x, UniqueType is_unique)
        {
          if (this != &x) {
            assign(x, is_unique,
              boost::unordered::detail::integral_constant<bool,
                allocator_traits<node_allocator>::
                  propagate_on_container_copy_assignment::value>());
          }
        }

        template <typename UniqueType>
        void assign(table const& x, UniqueType is_unique, false_type)
        {
          // Strong exception safety.
          this->construct_spare_functions(x.current_functions());
          BOOST_TRY
          {
            mlf_ = x.mlf_;
            recalculate_max_load();

            if (x.size_ > max_load_) {
              create_buckets(min_buckets_for_size(x.size_));
            } else if (size_) {
              clear_buckets();
            }
          }
          BOOST_CATCH(...)
          {
            this->cleanup_spare_functions();
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          this->switch_functions();
          assign_buckets(x, is_unique);
        }

        template <typename UniqueType>
        void assign(table const& x, UniqueType is_unique, true_type)
        {
          if (node_alloc() == x.node_alloc()) {
            allocators_.assign(x.allocators_);
            assign(x, is_unique, false_type());
          } else {
            this->construct_spare_functions(x.current_functions());
            this->switch_functions();

            // Delete everything with current allocators before assigning
            // the new ones.
            delete_buckets();
            allocators_.assign(x.allocators_);

            // Copy over other data, all no throw.
            mlf_ = x.mlf_;
            bucket_count_ = min_buckets_for_size(x.size_);

            // Finally copy the elements.
            if (x.size_) {
              copy_buckets(x, is_unique);
            }
          }
        }

        template <typename UniqueType>
        void move_assign(table& x, UniqueType is_unique)
        {
          if (this != &x) {
            move_assign(x, is_unique,
              boost::unordered::detail::integral_constant<bool,
                allocator_traits<node_allocator>::
                  propagate_on_container_move_assignment::value>());
          }
        }

        // Propagate allocator
        template <typename UniqueType>
        void move_assign(table& x, UniqueType, true_type)
        {
          if (!functions::nothrow_move_assignable) {
            this->construct_spare_functions(x.current_functions());
            this->switch_functions();
          } else {
            this->current_functions().move_assign(x.current_functions());
          }
          delete_buckets();
          allocators_.move_assign(x.allocators_);
          mlf_ = x.mlf_;
          move_buckets_from(x);
        }

        // Don't propagate allocator
        template <typename UniqueType>
        void move_assign(table& x, UniqueType is_unique, false_type)
        {
          if (node_alloc() == x.node_alloc()) {
            move_assign_equal_alloc(x);
          } else {
            move_assign_realloc(x, is_unique);
          }
        }

        void move_assign_equal_alloc(table& x)
        {
          if (!functions::nothrow_move_assignable) {
            this->construct_spare_functions(x.current_functions());
            this->switch_functions();
          } else {
            this->current_functions().move_assign(x.current_functions());
          }
          delete_buckets();
          mlf_ = x.mlf_;
          move_buckets_from(x);
        }

        template <typename UniqueType>
        void move_assign_realloc(table& x, UniqueType is_unique)
        {
          this->construct_spare_functions(x.current_functions());
          BOOST_TRY
          {
            mlf_ = x.mlf_;
            recalculate_max_load();

            if (x.size_ > max_load_) {
              create_buckets(min_buckets_for_size(x.size_));
            } else if (size_) {
              clear_buckets();
            }
          }
          BOOST_CATCH(...)
          {
            this->cleanup_spare_functions();
            BOOST_RETHROW
          }
          BOOST_CATCH_END
          this->switch_functions();
          move_assign_buckets(x, is_unique);
        }

        // Accessors

        const_key_type& get_key(node_pointer n) const
        {
          return extractor::extract(n->value());
        }

        std::size_t hash(const_key_type& k) const
        {
          return policy::apply_hash(this->hash_function(), k);
        }

        // Find Node

        node_pointer find_node(std::size_t key_hash, const_key_type& k) const
        {
          return this->find_node_impl(key_hash, k, this->key_eq());
        }

        node_pointer find_node(const_key_type& k) const
        {
          return this->find_node_impl(hash(k), k, this->key_eq());
        }

        template <class Key, class Pred>
        node_pointer find_node_impl(
          std::size_t key_hash, Key const& k, Pred const& eq) const
        {
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          node_pointer n = this->begin(bucket_index);

          for (;;) {
            if (!n)
              return n;

            if (eq(k, this->get_key(n))) {
              return n;
            } else if (this->node_bucket(n) != bucket_index) {
              return node_pointer();
            }

            n = next_for_find(n);
          }
        }

        // Find the node before the key, so that it can be erased.
        link_pointer find_previous_node(
          const_key_type& k, std::size_t bucket_index)
        {
          link_pointer prev = this->get_previous_start(bucket_index);
          if (!prev) {
            return prev;
          }

          for (;;) {
            node_pointer n = next_node(prev);
            if (!n) {
              return link_pointer();
            } else if (n->is_first_in_group()) {
              if (node_bucket(n) != bucket_index) {
                return link_pointer();
              } else if (this->key_eq()(k, this->get_key(n))) {
                return prev;
              }
            }
            prev = n;
          }
        }

        // Extract and erase

        inline node_pointer extract_by_key(const_key_type& k)
        {
          if (!this->size_) {
            return node_pointer();
          }
          std::size_t key_hash = this->hash(k);
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          link_pointer prev = this->find_previous_node(k, bucket_index);
          if (!prev) {
            return node_pointer();
          }
          node_pointer n = next_node(prev);
          node_pointer n2 = next_node(n);
          if (n2) {
            n2->set_first_in_group();
          }
          prev->next_ = n2;
          --this->size_;
          this->fix_bucket(bucket_index, prev, n2);
          n->next_ = link_pointer();

          return n;
        }

        // Reserve and rehash

        void reserve_for_insert(std::size_t);
        void rehash(std::size_t);
        void reserve(std::size_t);
        void rehash_impl(std::size_t);

        ////////////////////////////////////////////////////////////////////////
        // Unique keys

        // equals

        bool equals_unique(table const& other) const
        {
          if (this->size_ != other.size_)
            return false;

          for (node_pointer n1 = this->begin(); n1; n1 = next_node(n1)) {
            node_pointer n2 = other.find_node(other.get_key(n1));

            if (!n2 || n1->value() != n2->value())
              return false;
          }

          return true;
        }

        // Emplace/Insert

        inline node_pointer add_node_unique(
          node_pointer n, std::size_t key_hash)
        {
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          bucket_pointer b = this->get_bucket_pointer(bucket_index);

          n->bucket_info_ = bucket_index;
          n->set_first_in_group();

          if (!b->next_) {
            link_pointer start_node = this->get_previous_start();

            if (start_node->next_) {
              this->get_bucket_pointer(node_bucket(next_node(start_node)))
                ->next_ = n;
            }

            b->next_ = start_node;
            n->next_ = start_node->next_;
            start_node->next_ = n;
          } else {
            n->next_ = b->next_->next_;
            b->next_->next_ = n;
          }

          ++this->size_;
          return n;
        }

        inline node_pointer resize_and_add_node_unique(
          node_pointer n, std::size_t key_hash)
        {
          node_tmp b(n, this->node_alloc());
          this->reserve_for_insert(this->size_ + 1);
          return this->add_node_unique(b.release(), key_hash);
        }

        template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
        iterator emplace_hint_unique(
          c_iterator hint, const_key_type& k, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          if (hint.node_ && this->key_eq()(k, this->get_key(hint.node_))) {
            return iterator(hint.node_);
          } else {
            return emplace_unique(k, BOOST_UNORDERED_EMPLACE_FORWARD).first;
          }
        }

        template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
        emplace_return emplace_unique(
          const_key_type& k, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (pos) {
            return emplace_return(iterator(pos), false);
          } else {
            return emplace_return(
              iterator(this->resize_and_add_node_unique(
                boost::unordered::detail::func::construct_node_from_args(
                  this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
                key_hash)),
              true);
          }
        }

        template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
        iterator emplace_hint_unique(
          c_iterator hint, no_key, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          node_tmp b(boost::unordered::detail::func::construct_node_from_args(
                       this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
            this->node_alloc());
          const_key_type& k = this->get_key(b.node_);
          if (hint.node_ && this->key_eq()(k, this->get_key(hint.node_))) {
            return iterator(hint.node_);
          }
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (pos) {
            return iterator(pos);
          } else {
            return iterator(
              this->resize_and_add_node_unique(b.release(), key_hash));
          }
        }

        template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
        emplace_return emplace_unique(no_key, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          node_tmp b(boost::unordered::detail::func::construct_node_from_args(
                       this->node_alloc(), BOOST_UNORDERED_EMPLACE_FORWARD),
            this->node_alloc());
          const_key_type& k = this->get_key(b.node_);
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (pos) {
            return emplace_return(iterator(pos), false);
          } else {
            return emplace_return(
              iterator(this->resize_and_add_node_unique(b.release(), key_hash)),
              true);
          }
        }

        template <typename Key>
        emplace_return try_emplace_unique(BOOST_FWD_REF(Key) k)
        {
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (pos) {
            return emplace_return(iterator(pos), false);
          } else {
            return emplace_return(
              iterator(this->resize_and_add_node_unique(
                boost::unordered::detail::func::construct_node_pair(
                  this->node_alloc(), boost::forward<Key>(k)),
                key_hash)),
              true);
          }
        }

        template <typename Key>
        iterator try_emplace_hint_unique(c_iterator hint, BOOST_FWD_REF(Key) k)
        {
          if (hint.node_ && this->key_eq()(hint->first, k)) {
            return iterator(hint.node_);
          } else {
            return try_emplace_unique(k).first;
          }
        }

        template <typename Key, BOOST_UNORDERED_EMPLACE_TEMPLATE>
        emplace_return try_emplace_unique(
          BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (pos) {
            return emplace_return(iterator(pos), false);
          } else {
            return emplace_return(
              iterator(this->resize_and_add_node_unique(
                boost::unordered::detail::func::construct_node_pair_from_args(
                  this->node_alloc(), boost::forward<Key>(k),
                  BOOST_UNORDERED_EMPLACE_FORWARD),
                key_hash)),
              true);
          }
        }

        template <typename Key, BOOST_UNORDERED_EMPLACE_TEMPLATE>
        iterator try_emplace_hint_unique(
          c_iterator hint, BOOST_FWD_REF(Key) k, BOOST_UNORDERED_EMPLACE_ARGS)
        {
          if (hint.node_ && this->key_eq()(hint->first, k)) {
            return iterator(hint.node_);
          } else {
            return try_emplace_unique(k, BOOST_UNORDERED_EMPLACE_FORWARD).first;
          }
        }

        template <typename Key, typename M>
        emplace_return insert_or_assign_unique(
          BOOST_FWD_REF(Key) k, BOOST_FWD_REF(M) obj)
        {
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);

          if (pos) {
            pos->value().second = boost::forward<M>(obj);
            return emplace_return(iterator(pos), false);
          } else {
            return emplace_return(
              iterator(this->resize_and_add_node_unique(
                boost::unordered::detail::func::construct_node_pair(
                  this->node_alloc(), boost::forward<Key>(k),
                  boost::forward<M>(obj)),
                key_hash)),
              true);
          }
        }

        template <typename NodeType, typename InsertReturnType>
        void move_insert_node_type_unique(
          NodeType& np, InsertReturnType& result)
        {
          if (np) {
            const_key_type& k = this->get_key(np.ptr_);
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);

            if (pos) {
              result.node = boost::move(np);
              result.position = iterator(pos);
            } else {
              this->reserve_for_insert(this->size_ + 1);
              result.position =
                iterator(this->add_node_unique(np.ptr_, key_hash));
              result.inserted = true;
              np.ptr_ = node_pointer();
            }
          }
        }

        template <typename NodeType>
        iterator move_insert_node_type_with_hint_unique(
          c_iterator hint, NodeType& np)
        {
          if (!np) {
            return iterator();
          }
          const_key_type& k = this->get_key(np.ptr_);
          if (hint.node_ && this->key_eq()(k, this->get_key(hint.node_))) {
            return iterator(hint.node_);
          }
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);
          if (!pos) {
            this->reserve_for_insert(this->size_ + 1);
            pos = this->add_node_unique(np.ptr_, key_hash);
            np.ptr_ = node_pointer();
          }
          return iterator(pos);
        }

        template <typename Types2>
        void merge_unique(boost::unordered::detail::table<Types2>& other)
        {
          typedef boost::unordered::detail::table<Types2> other_table;
          BOOST_STATIC_ASSERT(
            (boost::is_same<node, typename other_table::node>::value));
          BOOST_ASSERT(this->node_alloc() == other.node_alloc());

          if (other.size_) {
            link_pointer prev = other.get_previous_start();

            while (prev->next_) {
              node_pointer n = other_table::next_node(prev);
              const_key_type& k = this->get_key(n);
              std::size_t key_hash = this->hash(k);
              node_pointer pos = this->find_node(key_hash, k);

              if (pos) {
                prev = n;
              } else {
                this->reserve_for_insert(this->size_ + 1);
                node_pointer n2 = next_node(n);
                prev->next_ = n2;
                if (n2 && n->is_first_in_group()) {
                  n2->set_first_in_group();
                }
                --other.size_;
                other.fix_bucket(other.node_bucket(n), prev, n2);
                this->add_node_unique(n, key_hash);
              }
            }
          }
        }

        ////////////////////////////////////////////////////////////////////////
        // Insert range methods
        //
        // if hash function throws, or inserting > 1 element, basic exception
        // safety strong otherwise

        template <class InputIt>
        void insert_range_unique(const_key_type& k, InputIt i, InputIt j)
        {
          insert_range_unique2(k, i, j);

          while (++i != j) {
            // Note: can't use get_key as '*i' might not be value_type - it
            // could be a pair with first_types as key_type without const or
            // a different second_type.
            insert_range_unique2(extractor::extract(*i), i, j);
          }
        }

        template <class InputIt>
        void insert_range_unique2(const_key_type& k, InputIt i, InputIt j)
        {
          // No side effects in this initial code
          std::size_t key_hash = this->hash(k);
          node_pointer pos = this->find_node(key_hash, k);

          if (!pos) {
            node_tmp b(boost::unordered::detail::func::construct_node(
                         this->node_alloc(), *i),
              this->node_alloc());
            if (this->size_ + 1 > this->max_load_)
              this->reserve_for_insert(
                this->size_ + boost::unordered::detail::insert_size(i, j));
            this->add_node_unique(b.release(), key_hash);
          }
        }

        template <class InputIt>
        void insert_range_unique(no_key, InputIt i, InputIt j)
        {
          node_constructor a(this->node_alloc());

          do {
            if (!a.node_) {
              a.create_node();
            }
            BOOST_UNORDERED_CALL_CONSTRUCT1(
              node_allocator_traits, a.alloc_, a.node_->value_ptr(), *i);
            node_tmp b(a.release(), a.alloc_);

            const_key_type& k = this->get_key(b.node_);
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);

            if (pos) {
              a.reclaim(b.release());
            } else {
              // reserve has basic exception safety if the hash function
              // throws, strong otherwise.
              this->reserve_for_insert(this->size_ + 1);
              this->add_node_unique(b.release(), key_hash);
            }
          } while (++i != j);
        }

        ////////////////////////////////////////////////////////////////////////
        // Extract

        inline node_pointer extract_by_iterator_unique(c_iterator i)
        {
          node_pointer n = i.node_;
          BOOST_ASSERT(n);
          std::size_t bucket_index = this->node_bucket(n);
          link_pointer prev = this->get_previous_start(bucket_index);
          while (prev->next_ != n) {
            prev = prev->next_;
          }
          node_pointer n2 = next_node(n);
          prev->next_ = n2;
          --this->size_;
          this->fix_bucket(bucket_index, prev, n2);
          n->next_ = link_pointer();
          return n;
        }

        ////////////////////////////////////////////////////////////////////////
        // Erase
        //
        // no throw

        std::size_t erase_key_unique(const_key_type& k)
        {
          if (!this->size_)
            return 0;
          std::size_t key_hash = this->hash(k);
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          link_pointer prev = this->find_previous_node(k, bucket_index);
          if (!prev)
            return 0;
          node_pointer n = next_node(prev);
          node_pointer n2 = next_node(n);
          prev->next_ = n2;
          --size_;
          this->fix_bucket(bucket_index, prev, n2);
          this->destroy_node(n);
          return 1;
        }

        void erase_nodes_unique(node_pointer i, node_pointer j)
        {
          std::size_t bucket_index = this->node_bucket(i);

          // Find the node before i.
          link_pointer prev = this->get_previous_start(bucket_index);
          while (prev->next_ != i)
            prev = prev->next_;

          // Delete the nodes.
          prev->next_ = j;
          do {
            node_pointer next = next_node(i);
            destroy_node(i);
            --size_;
            bucket_index = this->fix_bucket(bucket_index, prev, next);
            i = next;
          } while (i != j);
        }

        ////////////////////////////////////////////////////////////////////////
        // fill_buckets_unique

        void copy_buckets(table const& src, true_type)
        {
          this->create_buckets(this->bucket_count_);

          for (node_pointer n = src.begin(); n; n = next_node(n)) {
            std::size_t key_hash = this->hash(this->get_key(n));
            this->add_node_unique(
              boost::unordered::detail::func::construct_node(
                this->node_alloc(), n->value()),
              key_hash);
          }
        }

        void assign_buckets(table const& src, true_type)
        {
          node_holder<node_allocator> holder(*this);
          for (node_pointer n = src.begin(); n; n = next_node(n)) {
            std::size_t key_hash = this->hash(this->get_key(n));
            this->add_node_unique(holder.copy_of(n->value()), key_hash);
          }
        }

        void move_assign_buckets(table& src, true_type)
        {
          node_holder<node_allocator> holder(*this);
          for (node_pointer n = src.begin(); n; n = next_node(n)) {
            std::size_t key_hash = this->hash(this->get_key(n));
            this->add_node_unique(holder.move_copy_of(n->value()), key_hash);
          }
        }

        ////////////////////////////////////////////////////////////////////////
        // Equivalent keys

        // Equality

        bool equals_equiv(table const& other) const
        {
          if (this->size_ != other.size_)
            return false;

          for (node_pointer n1 = this->begin(); n1;) {
            node_pointer n2 = other.find_node(other.get_key(n1));
            if (!n2)
              return false;
            node_pointer end1 = next_group(n1);
            node_pointer end2 = next_group(n2);
            if (!group_equals_equiv(n1, end1, n2, end2))
              return false;
            n1 = end1;
          }

          return true;
        }

        static bool group_equals_equiv(node_pointer n1, node_pointer end1,
          node_pointer n2, node_pointer end2)
        {
          for (;;) {
            if (n1->value() != n2->value())
              break;

            n1 = next_node(n1);
            n2 = next_node(n2);

            if (n1 == end1)
              return n2 == end2;
            if (n2 == end2)
              return false;
          }

          for (node_pointer n1a = n1, n2a = n2;;) {
            n1a = next_node(n1a);
            n2a = next_node(n2a);

            if (n1a == end1) {
              if (n2a == end2)
                break;
              else
                return false;
            }

            if (n2a == end2)
              return false;
          }

          node_pointer start = n1;
          for (; n1 != end1; n1 = next_node(n1)) {
            value_type const& v = n1->value();
            if (!find_equiv(start, n1, v)) {
              std::size_t matches = count_equal_equiv(n2, end2, v);
              if (!matches)
                return false;
              if (matches != 1 + count_equal_equiv(next_node(n1), end1, v))
                return false;
            }
          }

          return true;
        }

        static bool find_equiv(
          node_pointer n, node_pointer end, value_type const& v)
        {
          for (; n != end; n = next_node(n))
            if (n->value() == v)
              return true;
          return false;
        }

        static std::size_t count_equal_equiv(
          node_pointer n, node_pointer end, value_type const& v)
        {
          std::size_t count = 0;
          for (; n != end; n = next_node(n))
            if (n->value() == v)
              ++count;
          return count;
        }

        // Emplace/Insert

        inline node_pointer add_node_equiv(
          node_pointer n, std::size_t key_hash, node_pointer pos)
        {
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          n->bucket_info_ = bucket_index;

          if (pos) {
            n->reset_first_in_group();
            n->next_ = pos->next_;
            pos->next_ = n;
            if (n->next_) {
              std::size_t next_bucket = this->node_bucket(next_node(n));
              if (next_bucket != bucket_index) {
                this->get_bucket_pointer(next_bucket)->next_ = n;
              }
            }
          } else {
            n->set_first_in_group();
            bucket_pointer b = this->get_bucket_pointer(bucket_index);

            if (!b->next_) {
              link_pointer start_node = this->get_previous_start();

              if (start_node->next_) {
                this
                  ->get_bucket_pointer(this->node_bucket(next_node(start_node)))
                  ->next_ = n;
              }

              b->next_ = start_node;
              n->next_ = start_node->next_;
              start_node->next_ = n;
            } else {
              n->next_ = b->next_->next_;
              b->next_->next_ = n;
            }
          }
          ++this->size_;
          return n;
        }

        inline node_pointer add_using_hint_equiv(
          node_pointer n, node_pointer hint)
        {
          n->bucket_info_ = hint->bucket_info_;
          n->reset_first_in_group();
          n->next_ = hint->next_;
          hint->next_ = n;
          if (n->next_) {
            std::size_t next_bucket = this->node_bucket(next_node(n));
            if (next_bucket != this->node_bucket(n)) {
              this->get_bucket_pointer(next_bucket)->next_ = n;
            }
          }
          ++this->size_;
          return n;
        }

        iterator emplace_equiv(node_pointer n)
        {
          node_tmp a(n, this->node_alloc());
          const_key_type& k = this->get_key(a.node_);
          std::size_t key_hash = this->hash(k);
          node_pointer position = this->find_node(key_hash, k);
          this->reserve_for_insert(this->size_ + 1);
          return iterator(
            this->add_node_equiv(a.release(), key_hash, position));
        }

        iterator emplace_hint_equiv(c_iterator hint, node_pointer n)
        {
          node_tmp a(n, this->node_alloc());
          const_key_type& k = this->get_key(a.node_);
          if (hint.node_ && this->key_eq()(k, this->get_key(hint.node_))) {
            this->reserve_for_insert(this->size_ + 1);
            return iterator(
              this->add_using_hint_equiv(a.release(), hint.node_));
          } else {
            std::size_t key_hash = this->hash(k);
            node_pointer position = this->find_node(key_hash, k);
            this->reserve_for_insert(this->size_ + 1);
            return iterator(
              this->add_node_equiv(a.release(), key_hash, position));
          }
        }

        void emplace_no_rehash_equiv(node_pointer n)
        {
          node_tmp a(n, this->node_alloc());
          const_key_type& k = this->get_key(a.node_);
          std::size_t key_hash = this->hash(k);
          node_pointer position = this->find_node(key_hash, k);
          this->add_node_equiv(a.release(), key_hash, position);
        }

        template <typename NodeType>
        iterator move_insert_node_type_equiv(NodeType& np)
        {
          iterator result;

          if (np) {
            const_key_type& k = this->get_key(np.ptr_);
            std::size_t key_hash = this->hash(k);
            node_pointer pos = this->find_node(key_hash, k);
            this->reserve_for_insert(this->size_ + 1);
            result = iterator(this->add_node_equiv(np.ptr_, key_hash, pos));
            np.ptr_ = node_pointer();
          }

          return result;
        }

        template <typename NodeType>
        iterator move_insert_node_type_with_hint_equiv(
          c_iterator hint, NodeType& np)
        {
          iterator result;

          if (np) {
            const_key_type& k = this->get_key(np.ptr_);

            if (hint.node_ && this->key_eq()(k, this->get_key(hint.node_))) {
              this->reserve_for_insert(this->size_ + 1);
              result =
                iterator(this->add_using_hint_equiv(np.ptr_, hint.node_));
            } else {
              std::size_t key_hash = this->hash(k);
              node_pointer pos = this->find_node(key_hash, k);
              this->reserve_for_insert(this->size_ + 1);
              result = iterator(this->add_node_equiv(np.ptr_, key_hash, pos));
            }
            np.ptr_ = node_pointer();
          }

          return result;
        }

        ////////////////////////////////////////////////////////////////////////
        // Insert range methods

        // if hash function throws, or inserting > 1 element, basic exception
        // safety. Strong otherwise
        template <class I>
        void insert_range_equiv(I i, I j,
          typename boost::unordered::detail::enable_if_forward<I, void*>::type =
            0)
        {
          if (i == j)
            return;

          std::size_t distance = static_cast<std::size_t>(std::distance(i, j));
          if (distance == 1) {
            emplace_equiv(boost::unordered::detail::func::construct_node(
              this->node_alloc(), *i));
          } else {
            // Only require basic exception safety here
            this->reserve_for_insert(this->size_ + distance);

            for (; i != j; ++i) {
              emplace_no_rehash_equiv(
                boost::unordered::detail::func::construct_node(
                  this->node_alloc(), *i));
            }
          }
        }

        template <class I>
        void insert_range_equiv(I i, I j,
          typename boost::unordered::detail::disable_if_forward<I,
            void*>::type = 0)
        {
          for (; i != j; ++i) {
            emplace_equiv(boost::unordered::detail::func::construct_node(
              this->node_alloc(), *i));
          }
        }

        ////////////////////////////////////////////////////////////////////////
        // Extract

        inline node_pointer extract_by_iterator_equiv(c_iterator n)
        {
          node_pointer i = n.node_;
          BOOST_ASSERT(i);
          node_pointer j(next_node(i));
          std::size_t bucket_index = this->node_bucket(i);

          link_pointer prev = this->get_previous_start(bucket_index);
          while (prev->next_ != i) {
            prev = next_node(prev);
          }

          prev->next_ = j;
          if (j && i->is_first_in_group()) {
            j->set_first_in_group();
          }
          --this->size_;
          this->fix_bucket(bucket_index, prev, j);
          i->next_ = link_pointer();

          return i;
        }

        ////////////////////////////////////////////////////////////////////////
        // Erase
        //
        // no throw

        std::size_t erase_key_equiv(const_key_type& k)
        {
          if (!this->size_)
            return 0;

          std::size_t key_hash = this->hash(k);
          std::size_t bucket_index = this->hash_to_bucket(key_hash);
          link_pointer prev = this->find_previous_node(k, bucket_index);
          if (!prev)
            return 0;

          std::size_t deleted_count = 0;
          node_pointer n = next_node(prev);
          do {
            node_pointer n2 = next_node(n);
            destroy_node(n);
            ++deleted_count;
            n = n2;
          } while (n && !n->is_first_in_group());
          size_ -= deleted_count;
          prev->next_ = n;
          this->fix_bucket(bucket_index, prev, n);
          return deleted_count;
        }

        link_pointer erase_nodes_equiv(node_pointer i, node_pointer j)
        {
          std::size_t bucket_index = this->node_bucket(i);

          link_pointer prev = this->get_previous_start(bucket_index);
          while (prev->next_ != i) {
            prev = next_node(prev);
          }

          // Delete the nodes.
          // Is it inefficient to call fix_bucket for every node?
          bool includes_first = false;
          prev->next_ = j;
          do {
            includes_first = includes_first || i->is_first_in_group();
            node_pointer next = next_node(i);
            destroy_node(i);
            --size_;
            bucket_index = this->fix_bucket(bucket_index, prev, next);
            i = next;
          } while (i != j);
          if (j && includes_first) {
            j->set_first_in_group();
          }

          return prev;
        }

        ////////////////////////////////////////////////////////////////////////
        // fill_buckets

        void copy_buckets(table const& src, false_type)
        {
          this->create_buckets(this->bucket_count_);

          for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = this->hash(this->get_key(n));
            node_pointer group_end(next_group(n));
            node_pointer pos = this->add_node_equiv(
              boost::unordered::detail::func::construct_node(
                this->node_alloc(), n->value()),
              key_hash, node_pointer());
            for (n = next_node(n); n != group_end; n = next_node(n)) {
              this->add_node_equiv(
                boost::unordered::detail::func::construct_node(
                  this->node_alloc(), n->value()),
                key_hash, pos);
            }
          }
        }

        void assign_buckets(table const& src, false_type)
        {
          node_holder<node_allocator> holder(*this);
          for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = this->hash(this->get_key(n));
            node_pointer group_end(next_group(n));
            node_pointer pos = this->add_node_equiv(
              holder.copy_of(n->value()), key_hash, node_pointer());
            for (n = next_node(n); n != group_end; n = next_node(n)) {
              this->add_node_equiv(holder.copy_of(n->value()), key_hash, pos);
            }
          }
        }

        void move_assign_buckets(table& src, false_type)
        {
          node_holder<node_allocator> holder(*this);
          for (node_pointer n = src.begin(); n;) {
            std::size_t key_hash = this->hash(this->get_key(n));
            node_pointer group_end(next_group(n));
            node_pointer pos = this->add_node_equiv(
              holder.move_copy_of(n->value()), key_hash, node_pointer());
            for (n = next_node(n); n != group_end; n = next_node(n)) {
              this->add_node_equiv(
                holder.move_copy_of(n->value()), key_hash, pos);
            }
          }
        }
      };

      //////////////////////////////////////////////////////////////////////////
      // Clear

      template <typename Types> inline void table<Types>::clear_impl()
      {
        if (size_) {
          bucket_pointer end = get_bucket_pointer(bucket_count_);
          for (bucket_pointer it = buckets_; it != end; ++it) {
            it->next_ = node_pointer();
          }

          link_pointer prev = end->first_from_start();
          node_pointer n = next_node(prev);
          prev->next_ = node_pointer();
          size_ = 0;

          while (n) {
            node_pointer next = next_node(n);
            destroy_node(n);
            n = next;
          }
        }
      }

      //////////////////////////////////////////////////////////////////////////
      // Reserve & Rehash

      // basic exception safety
      template <typename Types>
      inline void table<Types>::reserve_for_insert(std::size_t size)
      {
        if (!buckets_) {
          create_buckets((std::max)(bucket_count_, min_buckets_for_size(size)));
        } else if (size > max_load_) {
          std::size_t num_buckets =
            min_buckets_for_size((std::max)(size, size_ + (size_ >> 1)));

          if (num_buckets != bucket_count_)
            this->rehash_impl(num_buckets);
        }
      }

      // if hash function throws, basic exception safety
      // strong otherwise.

      template <typename Types>
      inline void table<Types>::rehash(std::size_t min_buckets)
      {
        using namespace std;

        if (!size_) {
          delete_buckets();
          bucket_count_ = policy::new_bucket_count(min_buckets);
        } else {
          min_buckets = policy::new_bucket_count((std::max)(min_buckets,
            boost::unordered::detail::double_to_size(
              floor(static_cast<double>(size_) / static_cast<double>(mlf_))) +
              1));

          if (min_buckets != bucket_count_)
            this->rehash_impl(min_buckets);
        }
      }

      template <typename Types>
      inline void table<Types>::rehash_impl(std::size_t num_buckets)
      {
        BOOST_ASSERT(this->buckets_);

        this->create_buckets(num_buckets);
        link_pointer prev = this->get_previous_start();
        BOOST_TRY
        {
          while (prev->next_) {
            node_pointer n = next_node(prev);
            std::size_t key_hash = this->hash(this->get_key(n));
            std::size_t bucket_index = this->hash_to_bucket(key_hash);

            n->bucket_info_ = bucket_index;
            n->set_first_in_group();

            // Iterator through the rest of the group of equal nodes,
            // setting the bucket.
            for (;;) {
              node_pointer next = next_node(n);
              if (!next || next->is_first_in_group()) {
                break;
              }
              n = next;
              n->bucket_info_ = bucket_index;
              n->reset_first_in_group();
            }

            // n is now the last node in the group
            bucket_pointer b = this->get_bucket_pointer(bucket_index);
            if (!b->next_) {
              b->next_ = prev;
              prev = n;
            } else {
              link_pointer next = n->next_;
              n->next_ = b->next_->next_;
              b->next_->next_ = prev->next_;
              prev->next_ = next;
            }
          }
        }
        BOOST_CATCH(...)
        {
          node_pointer n = next_node(prev);
          prev->next_ = node_pointer();
          while (n) {
            node_pointer next = next_node(n);
            destroy_node(n);
            --size_;
            n = next;
          }
          BOOST_RETHROW
        }
        BOOST_CATCH_END
      }

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

      ////////////////////////////////////////////////////////////////////////
      // key extractors
      //
      // no throw
      //
      // 'extract_key' is called with the emplace parameters to return a
      // key if available or 'no_key' is one isn't and will need to be
      // constructed. This could be done by overloading the emplace
      // implementation
      // for the different cases, but that's a bit tricky on compilers without
      // variadic templates.

      template <typename Key, typename T> struct is_key
      {
        template <typename T2> static choice1::type test(T2 const&);
        static choice2::type test(Key const&);

        enum
        {
          value = sizeof(test(boost::unordered::detail::make<T>())) ==
                  sizeof(choice2::type)
        };

        typedef typename boost::detail::if_true<value>::BOOST_NESTED_TEMPLATE
          then<Key const&, no_key>::type type;
      };

      template <class ValueType> struct set_extractor
      {
        typedef ValueType value_type;
        typedef ValueType key_type;

        static key_type const& extract(value_type const& v) { return v; }

        static key_type const& extract(BOOST_UNORDERED_RV_REF(value_type) v)
        {
          return v;
        }

        static no_key extract() { return no_key(); }

        template <class Arg> static no_key extract(Arg const&)
        {
          return no_key();
        }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template <class Arg1, class Arg2, class... Args>
        static no_key extract(Arg1 const&, Arg2 const&, Args const&...)
        {
          return no_key();
        }
#else
        template <class Arg1, class Arg2>
        static no_key extract(Arg1 const&, Arg2 const&)
        {
          return no_key();
        }
#endif
      };

      template <class ValueType> struct map_extractor
      {
        typedef ValueType value_type;
        typedef typename boost::remove_const<typename boost::unordered::detail::
            pair_traits<ValueType>::first_type>::type key_type;

        static key_type const& extract(value_type const& v) { return v.first; }

        template <class Second>
        static key_type const& extract(std::pair<key_type, Second> const& v)
        {
          return v.first;
        }

        template <class Second>
        static key_type const& extract(
          std::pair<key_type const, Second> const& v)
        {
          return v.first;
        }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
        template <class Second>
        static key_type const& extract(
          boost::rv<std::pair<key_type, Second> > const& v)
        {
          return v.first;
        }

        template <class Second>
        static key_type const& extract(
          boost::rv<std::pair<key_type const, Second> > const& v)
        {
          return v.first;
        }
#endif

        template <class Arg1>
        static key_type const& extract(key_type const& k, Arg1 const&)
        {
          return k;
        }

        static no_key extract() { return no_key(); }

        template <class Arg> static no_key extract(Arg const&)
        {
          return no_key();
        }

        template <class Arg1, class Arg2>
        static no_key extract(Arg1 const&, Arg2 const&)
        {
          return no_key();
        }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template <class Arg1, class Arg2, class Arg3, class... Args>
        static no_key extract(
          Arg1 const&, Arg2 const&, Arg3 const&, Args const&...)
        {
          return no_key();
        }
#endif

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_KEY_FROM_TUPLE(namespace_)                             \
  template <typename T2>                                                       \
  static no_key extract(boost::unordered::piecewise_construct_t,               \
    namespace_ tuple<> const&, T2 const&)                                      \
  {                                                                            \
    return no_key();                                                           \
  }                                                                            \
                                                                               \
  template <typename T, typename T2>                                           \
  static typename is_key<key_type, T>::type extract(                           \
    boost::unordered::piecewise_construct_t, namespace_ tuple<T> const& k,     \
    T2 const&)                                                                 \
  {                                                                            \
    return typename is_key<key_type, T>::type(namespace_ get<0>(k));           \
  }

#else

#define BOOST_UNORDERED_KEY_FROM_TUPLE(namespace_)                             \
  static no_key extract(                                                       \
    boost::unordered::piecewise_construct_t, namespace_ tuple<> const&)        \
  {                                                                            \
    return no_key();                                                           \
  }                                                                            \
                                                                               \
  template <typename T>                                                        \
  static typename is_key<key_type, T>::type extract(                           \
    boost::unordered::piecewise_construct_t, namespace_ tuple<T> const& k)     \
  {                                                                            \
    return typename is_key<key_type, T>::type(namespace_ get<0>(k));           \
  }

#endif

        BOOST_UNORDERED_KEY_FROM_TUPLE(boost::)

#if BOOST_UNORDERED_TUPLE_ARGS
        BOOST_UNORDERED_KEY_FROM_TUPLE(std::)
#endif

#undef BOOST_UNORDERED_KEY_FROM_TUPLE
      };

      ////////////////////////////////////////////////////////////////////////
      // Unique nodes

      template <typename A, typename T>
      struct node : boost::unordered::detail::value_base<T>
      {
        typedef
          typename ::boost::unordered::detail::rebind_wrap<A, node<A, T> >::type
            allocator;
        typedef typename ::boost::unordered::detail::allocator_traits<
          allocator>::pointer node_pointer;
        typedef node_pointer link_pointer;
        typedef typename ::boost::unordered::detail::rebind_wrap<A,
          bucket<node_pointer> >::type bucket_allocator;
        typedef typename ::boost::unordered::detail::allocator_traits<
          bucket_allocator>::pointer bucket_pointer;

        link_pointer next_;
        std::size_t bucket_info_;

        node() : next_(), bucket_info_(0) {}

        std::size_t get_bucket() const
        {
          return bucket_info_ & ((std::size_t)-1 >> 1);
        }

        std::size_t is_first_in_group() const
        {
          return !(bucket_info_ & ~((std::size_t)-1 >> 1));
        }

        void set_first_in_group()
        {
          bucket_info_ = bucket_info_ & ((std::size_t)-1 >> 1);
        }

        void reset_first_in_group()
        {
          bucket_info_ = bucket_info_ | ~((std::size_t)-1 >> 1);
        }

      private:
        node& operator=(node const&);
      };

      template <typename T>
      struct ptr_node : boost::unordered::detail::ptr_bucket
      {
        typedef T value_type;
        typedef boost::unordered::detail::ptr_bucket bucket_base;
        typedef ptr_node<T>* node_pointer;
        typedef ptr_bucket* link_pointer;
        typedef ptr_bucket* bucket_pointer;

        std::size_t bucket_info_;
        boost::unordered::detail::value_base<T> value_base_;

        ptr_node() : bucket_base(), bucket_info_(0) {}

        void* address() { return value_base_.address(); }
        value_type& value() { return value_base_.value(); }
        value_type* value_ptr() { return value_base_.value_ptr(); }

        std::size_t get_bucket() const
        {
          return bucket_info_ & ((std::size_t)-1 >> 1);
        }

        std::size_t is_first_in_group() const
        {
          return !(bucket_info_ & ~((std::size_t)-1 >> 1));
        }

        void set_first_in_group()
        {
          bucket_info_ = bucket_info_ & ((std::size_t)-1 >> 1);
        }

        void reset_first_in_group()
        {
          bucket_info_ = bucket_info_ | ~((std::size_t)-1 >> 1);
        }

      private:
        ptr_node& operator=(ptr_node const&);
      };

      // If the allocator uses raw pointers use ptr_node
      // Otherwise use node.

      template <typename A, typename T, typename NodePtr, typename BucketPtr>
      struct pick_node2
      {
        typedef boost::unordered::detail::node<A, T> node;

        typedef typename boost::unordered::detail::allocator_traits<
          typename boost::unordered::detail::rebind_wrap<A,
            node>::type>::pointer node_pointer;

        typedef boost::unordered::detail::bucket<node_pointer> bucket;
        typedef node_pointer link_pointer;
      };

      template <typename A, typename T>
      struct pick_node2<A, T, boost::unordered::detail::ptr_node<T>*,
        boost::unordered::detail::ptr_bucket*>
      {
        typedef boost::unordered::detail::ptr_node<T> node;
        typedef boost::unordered::detail::ptr_bucket bucket;
        typedef bucket* link_pointer;
      };

      template <typename A, typename T> struct pick_node
      {
        typedef typename boost::remove_const<T>::type nonconst;

        typedef boost::unordered::detail::allocator_traits<
          typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::ptr_node<nonconst> >::type>
          tentative_node_traits;

        typedef boost::unordered::detail::allocator_traits<
          typename boost::unordered::detail::rebind_wrap<A,
            boost::unordered::detail::ptr_bucket>::type>
          tentative_bucket_traits;

        typedef pick_node2<A, nonconst, typename tentative_node_traits::pointer,
          typename tentative_bucket_traits::pointer>
          pick;

        typedef typename pick::node node;
        typedef typename pick::bucket bucket;
        typedef typename pick::link_pointer link_pointer;
      };
    }
  }
}

#undef BOOST_UNORDERED_EMPLACE_TEMPLATE
#undef BOOST_UNORDERED_EMPLACE_ARGS
#undef BOOST_UNORDERED_EMPLACE_FORWARD
#undef BOOST_UNORDERED_CALL_CONSTRUCT1
#undef BOOST_UNORDERED_CALL_DESTROY

#endif
