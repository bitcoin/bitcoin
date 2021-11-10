//
// (C) Copyright Jeremy Siek 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Revision History:
//
//   17 July  2001: Added const to some member functions. (Jeremy Siek) 
//   05 May   2001: Removed static dummy_cons object. (Jeremy Siek)

// See http://www.boost.org/libs/concept_check for documentation.

#ifndef BOOST_CONCEPT_ARCHETYPES_HPP
#define BOOST_CONCEPT_ARCHETYPES_HPP

#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <functional>
#include <iterator>  // iterator tags
#include <cstddef>   // std::ptrdiff_t

namespace boost {

  //===========================================================================
  // Basic Archetype Classes

  namespace detail {
    class dummy_constructor { };
  }

  // A type that models no concept. The template parameter 
  // is only there so that null_archetype types can be created
  // that have different type.
  template <class T = int>
  class null_archetype {
  private:
    null_archetype() { }
    null_archetype(const null_archetype&) { }
    null_archetype& operator=(const null_archetype&) { return *this; }
  public:
    null_archetype(detail::dummy_constructor) { }
#ifndef __MWERKS__
    template <class TT>
    friend void dummy_friend(); // just to avoid warnings
#endif
  };

  // This is a helper class that provides a way to get a reference to
  // an object. The get() function will never be called at run-time
  // (nothing in this file will) so this seemingly very bad function
  // is really quite innocent. The name of this class needs to be
  // changed.
  template <class T>
  class static_object
  {
  public:
      static T& get()
      {
#if BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x564))
          return *reinterpret_cast<T*>(0);
#else 
          static char d[sizeof(T)];
          return *reinterpret_cast<T*>(d);
#endif 
      }
  };

  template <class Base = null_archetype<> >
  class default_constructible_archetype : public Base {
  public:
    default_constructible_archetype() 
      : Base(static_object<detail::dummy_constructor>::get()) { }
    default_constructible_archetype(detail::dummy_constructor x) : Base(x) { }
  };

  template <class Base = null_archetype<> >
  class assignable_archetype : public Base {
    assignable_archetype() { }
    assignable_archetype(const assignable_archetype&) { }
  public:
    assignable_archetype& operator=(const assignable_archetype&) {
      return *this;
    }
    assignable_archetype(detail::dummy_constructor x) : Base(x) { }
  };

  template <class Base = null_archetype<> >
  class copy_constructible_archetype : public Base {
  public:
    copy_constructible_archetype() 
      : Base(static_object<detail::dummy_constructor>::get()) { }
    copy_constructible_archetype(const copy_constructible_archetype&)
      : Base(static_object<detail::dummy_constructor>::get()) { }
    copy_constructible_archetype(detail::dummy_constructor x) : Base(x) { }
  };

  template <class Base = null_archetype<> >
  class sgi_assignable_archetype : public Base {
  public:
    sgi_assignable_archetype(const sgi_assignable_archetype&)
      : Base(static_object<detail::dummy_constructor>::get()) { }
    sgi_assignable_archetype& operator=(const sgi_assignable_archetype&) {
      return *this;
    }
    sgi_assignable_archetype(const detail::dummy_constructor& x) : Base(x) { }
  };

  struct default_archetype_base {
    default_archetype_base(detail::dummy_constructor) { }
  };

  // Careful, don't use same type for T and Base. That results in the
  // conversion operator being invalid.  Since T is often
  // null_archetype, can't use null_archetype for Base.
  template <class T, class Base = default_archetype_base>
  class convertible_to_archetype : public Base {
  private:
    convertible_to_archetype() { }
    convertible_to_archetype(const convertible_to_archetype& ) { }
    convertible_to_archetype& operator=(const convertible_to_archetype&)
      { return *this; }
  public:
    convertible_to_archetype(detail::dummy_constructor x) : Base(x) { }
    operator const T&() const { return static_object<T>::get(); }
  };

  template <class T, class Base = default_archetype_base>
  class convertible_from_archetype : public Base {
  private:
    convertible_from_archetype() { }
    convertible_from_archetype(const convertible_from_archetype& ) { }
    convertible_from_archetype& operator=(const convertible_from_archetype&)
      { return *this; }
  public:
    convertible_from_archetype(detail::dummy_constructor x) : Base(x) { }
    convertible_from_archetype(const T&) { }
    convertible_from_archetype& operator=(const T&)
      { return *this; }
  };

  class boolean_archetype {
  public:
    boolean_archetype(const boolean_archetype&) { }
    operator bool() const { return true; }
    boolean_archetype(detail::dummy_constructor) { }
  private:
    boolean_archetype() { }
    boolean_archetype& operator=(const boolean_archetype&) { return *this; }
  };
  
  template <class Base = null_archetype<> >
  class equality_comparable_archetype : public Base {
  public:
    equality_comparable_archetype(detail::dummy_constructor x) : Base(x) { }
  };
  template <class Base>
  boolean_archetype
  operator==(const equality_comparable_archetype<Base>&,
             const equality_comparable_archetype<Base>&) 
  { 
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }
  template <class Base>
  boolean_archetype
  operator!=(const equality_comparable_archetype<Base>&,
             const equality_comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }


  template <class Base = null_archetype<> >
  class equality_comparable2_first_archetype : public Base {
  public:
    equality_comparable2_first_archetype(detail::dummy_constructor x) 
      : Base(x) { }
  };
  template <class Base = null_archetype<> >
  class equality_comparable2_second_archetype : public Base {
  public:
    equality_comparable2_second_archetype(detail::dummy_constructor x) 
      : Base(x) { }
  };
  template <class Base1, class Base2>
  boolean_archetype
  operator==(const equality_comparable2_first_archetype<Base1>&,
             const equality_comparable2_second_archetype<Base2>&) 
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }
  template <class Base1, class Base2>
  boolean_archetype
  operator!=(const equality_comparable2_first_archetype<Base1>&,
             const equality_comparable2_second_archetype<Base2>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }


  template <class Base = null_archetype<> >
  class less_than_comparable_archetype : public Base {
  public:
    less_than_comparable_archetype(detail::dummy_constructor x) : Base(x) { }
  };
  template <class Base>
  boolean_archetype
  operator<(const less_than_comparable_archetype<Base>&,
            const less_than_comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }



  template <class Base = null_archetype<> >
  class comparable_archetype : public Base {
  public:
    comparable_archetype(detail::dummy_constructor x) : Base(x) { }
  };
  template <class Base>
  boolean_archetype
  operator<(const comparable_archetype<Base>&,
            const comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }
  template <class Base>
  boolean_archetype
  operator<=(const comparable_archetype<Base>&,
             const comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }
  template <class Base>
  boolean_archetype
  operator>(const comparable_archetype<Base>&,
            const comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }
  template <class Base>
  boolean_archetype
  operator>=(const comparable_archetype<Base>&,
             const comparable_archetype<Base>&)
  {
    return boolean_archetype(static_object<detail::dummy_constructor>::get());
  }


  // The purpose of the optags is so that one can specify
  // exactly which types the operator< is defined between.
  // This is useful for allowing the operations:
  //
  // A a; B b;
  // a < b
  // b < a
  //
  // without also allowing the combinations:
  //
  // a < a
  // b < b
  //
  struct optag1 { };
  struct optag2 { };
  struct optag3 { };

#define BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(OP, NAME)                       \
  template <class Base = null_archetype<>, class Tag = optag1 >                 \
  class NAME##_first_archetype : public Base {                                  \
  public:                                                                       \
    NAME##_first_archetype(detail::dummy_constructor x) : Base(x) { }           \
  };                                                                            \
                                                                                \
  template <class Base = null_archetype<>, class Tag = optag1 >                 \
  class NAME##_second_archetype : public Base {                                 \
  public:                                                                       \
    NAME##_second_archetype(detail::dummy_constructor x) : Base(x) { }          \
  };                                                                            \
                                                                                \
  template <class BaseFirst, class BaseSecond, class Tag>                       \
  boolean_archetype                                                             \
  operator OP (const NAME##_first_archetype<BaseFirst, Tag>&,                   \
               const NAME##_second_archetype<BaseSecond, Tag>&)                 \
  {                                                                             \
   return boolean_archetype(static_object<detail::dummy_constructor>::get());   \
  }

  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(==, equal_op)
  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(!=, not_equal_op)
  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(<, less_than_op)
  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(<=, less_equal_op)
  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(>, greater_than_op)
  BOOST_DEFINE_BINARY_PREDICATE_ARCHETYPE(>=, greater_equal_op)

#define BOOST_DEFINE_OPERATOR_ARCHETYPE(OP, NAME) \
  template <class Base = null_archetype<> > \
  class NAME##_archetype : public Base { \
  public: \
    NAME##_archetype(detail::dummy_constructor x) : Base(x) { } \
    NAME##_archetype(const NAME##_archetype&)  \
      : Base(static_object<detail::dummy_constructor>::get()) { } \
    NAME##_archetype& operator=(const NAME##_archetype&) { return *this; } \
  }; \
  template <class Base> \
  NAME##_archetype<Base> \
  operator OP (const NAME##_archetype<Base>&,\
               const NAME##_archetype<Base>&)  \
  { \
    return \
     NAME##_archetype<Base>(static_object<detail::dummy_constructor>::get()); \
  }

  BOOST_DEFINE_OPERATOR_ARCHETYPE(+, addable)
  BOOST_DEFINE_OPERATOR_ARCHETYPE(-, subtractable)
  BOOST_DEFINE_OPERATOR_ARCHETYPE(*, multipliable)
  BOOST_DEFINE_OPERATOR_ARCHETYPE(/, dividable)
  BOOST_DEFINE_OPERATOR_ARCHETYPE(%, modable)

  // As is, these are useless because of the return type.
  // Need to invent a better way...
#define BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(OP, NAME) \
  template <class Return, class Base = null_archetype<> > \
  class NAME##_first_archetype : public Base { \
  public: \
    NAME##_first_archetype(detail::dummy_constructor x) : Base(x) { } \
  }; \
  \
  template <class Return, class Base = null_archetype<> > \
  class NAME##_second_archetype : public Base { \
  public: \
    NAME##_second_archetype(detail::dummy_constructor x) : Base(x) { } \
  }; \
  \
  template <class Return, class BaseFirst, class BaseSecond> \
  Return \
  operator OP (const NAME##_first_archetype<Return, BaseFirst>&, \
               const NAME##_second_archetype<Return, BaseSecond>&) \
  { \
    return Return(static_object<detail::dummy_constructor>::get()); \
  }

  BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(+, plus_op)
  BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(*, time_op)
  BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(/, divide_op)
  BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(-, subtract_op)
  BOOST_DEFINE_BINARY_OPERATOR_ARCHETYPE(%, mod_op)

  //===========================================================================
  // Function Object Archetype Classes

  template <class Return>
  class generator_archetype {
  public:
    const Return& operator()() {
      return static_object<Return>::get(); 
    }
  };

  class void_generator_archetype {
  public:
    void operator()() { }
  };

  template <class Arg, class Return>
  class unary_function_archetype {
  private:
    unary_function_archetype() { }
  public:
    unary_function_archetype(detail::dummy_constructor) { }
    const Return& operator()(const Arg&) const {
      return static_object<Return>::get(); 
    }
  };

  template <class Arg1, class Arg2, class Return>
  class binary_function_archetype {
  private:
    binary_function_archetype() { }
  public:
    binary_function_archetype(detail::dummy_constructor) { }
    const Return& operator()(const Arg1&, const Arg2&) const {
      return static_object<Return>::get(); 
    }
  };

  template <class Arg>
  class unary_predicate_archetype {
    typedef boolean_archetype Return;
    unary_predicate_archetype() { }
  public:
    unary_predicate_archetype(detail::dummy_constructor) { }
    const Return& operator()(const Arg&) const {
      return static_object<Return>::get(); 
    }
  };

  template <class Arg1, class Arg2, class Base = null_archetype<> >
  class binary_predicate_archetype {
    typedef boolean_archetype Return;
    binary_predicate_archetype() { }
  public:
    binary_predicate_archetype(detail::dummy_constructor) { }
    const Return& operator()(const Arg1&, const Arg2&) const {
      return static_object<Return>::get(); 
    }
  };

  //===========================================================================
  // Iterator Archetype Classes

  template <class T, int I = 0>
  class input_iterator_archetype
  {
  private:
    typedef input_iterator_archetype self;
  public:
    typedef std::input_iterator_tag iterator_category;
    typedef T value_type;
    struct reference {
      operator const value_type&() const { return static_object<T>::get(); }
    };
    typedef const T* pointer;
    typedef std::ptrdiff_t difference_type;
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return reference(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  };

  template <class T>
  class input_iterator_archetype_no_proxy
  {
  private:
    typedef input_iterator_archetype_no_proxy self;
  public:
    typedef std::input_iterator_tag iterator_category;
    typedef T value_type;
    typedef const T& reference;
    typedef const T* pointer;
    typedef std::ptrdiff_t difference_type;
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  };

  template <class T>
  struct output_proxy {
    output_proxy& operator=(const T&) { return *this; }
  };

  template <class T>
  class output_iterator_archetype
  {
  public:
    typedef output_iterator_archetype self;
  public:
    typedef std::output_iterator_tag iterator_category;
    typedef output_proxy<T> value_type;
    typedef output_proxy<T> reference;
    typedef void pointer;
    typedef void difference_type;
    output_iterator_archetype(detail::dummy_constructor) { }
    output_iterator_archetype(const self&) { }
    self& operator=(const self&) { return *this; }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return output_proxy<T>(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  private:
    output_iterator_archetype() { }
  };

  template <class T>
  class input_output_iterator_archetype
  {
  private:
    typedef input_output_iterator_archetype self;
    struct in_out_tag : public std::input_iterator_tag, public std::output_iterator_tag { };
  public:
    typedef in_out_tag iterator_category;
    typedef T value_type;
    struct reference {
      reference& operator=(const T&) { return *this; }
      operator value_type() { return static_object<T>::get(); }
    };
    typedef const T* pointer;
    typedef std::ptrdiff_t difference_type;
    input_output_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return reference(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  };

  template <class T>
  class forward_iterator_archetype
  {
  public:
    typedef forward_iterator_archetype self;
  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef T value_type;
    typedef const T& reference;
    typedef T const* pointer;
    typedef std::ptrdiff_t difference_type;
    forward_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  };

  template <class T>
  class mutable_forward_iterator_archetype
  {
  public:
    typedef mutable_forward_iterator_archetype self;
  public:
    typedef std::forward_iterator_tag iterator_category;
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::ptrdiff_t difference_type;
    mutable_forward_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
  };

  template <class T>
  class bidirectional_iterator_archetype
  {
  public:
    typedef bidirectional_iterator_archetype self;
  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef const T& reference;
    typedef T* pointer;
    typedef std::ptrdiff_t difference_type;
    bidirectional_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
    self& operator--() { return *this; }
    self operator--(int) { return *this; }
  };

  template <class T>
  class mutable_bidirectional_iterator_archetype
  {
  public:
    typedef mutable_bidirectional_iterator_archetype self;
  public:
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::ptrdiff_t difference_type;
    mutable_bidirectional_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
    self& operator--() { return *this; }
    self operator--(int) { return *this; }
  };

  template <class T>
  class random_access_iterator_archetype
  {
  public:
    typedef random_access_iterator_archetype self;
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef const T& reference;
    typedef T* pointer;
    typedef std::ptrdiff_t difference_type;
    random_access_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
    self& operator--() { return *this; }
    self operator--(int) { return *this; }
    reference operator[](difference_type) const
      { return static_object<T>::get(); }
    self& operator+=(difference_type) { return *this; }
    self& operator-=(difference_type) { return *this; }
    difference_type operator-(const self&) const
      { return difference_type(); }
    self operator+(difference_type) const { return *this; }
    self operator-(difference_type) const { return *this; }
    bool operator<(const self&) const { return true; }
    bool operator<=(const self&) const { return true; }
    bool operator>(const self&) const { return true; }
    bool operator>=(const self&) const { return true; }
  };
  template <class T>
  random_access_iterator_archetype<T> 
  operator+(typename random_access_iterator_archetype<T>::difference_type, 
            const random_access_iterator_archetype<T>& x) 
    { return x; }


  template <class T>
  class mutable_random_access_iterator_archetype
  {
  public:
    typedef mutable_random_access_iterator_archetype self;
  public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef std::ptrdiff_t difference_type;
    mutable_random_access_iterator_archetype() { }
    self& operator=(const self&) { return *this;  }
    bool operator==(const self&) const { return true; }
    bool operator!=(const self&) const { return true; }
    reference operator*() const { return static_object<T>::get(); }
    self& operator++() { return *this; }
    self operator++(int) { return *this; }
    self& operator--() { return *this; }
    self operator--(int) { return *this; }
    reference operator[](difference_type) const
      { return static_object<T>::get(); }
    self& operator+=(difference_type) { return *this; }
    self& operator-=(difference_type) { return *this; }
    difference_type operator-(const self&) const
      { return difference_type(); }
    self operator+(difference_type) const { return *this; }
    self operator-(difference_type) const { return *this; }
    bool operator<(const self&) const { return true; }
    bool operator<=(const self&) const { return true; }
    bool operator>(const self&) const { return true; }
    bool operator>=(const self&) const { return true; }
  };
  template <class T>
  mutable_random_access_iterator_archetype<T> 
  operator+
    (typename mutable_random_access_iterator_archetype<T>::difference_type, 
     const mutable_random_access_iterator_archetype<T>& x) 
    { return x; }

} // namespace boost

#endif // BOOST_CONCEPT_ARCHETYPES_H
