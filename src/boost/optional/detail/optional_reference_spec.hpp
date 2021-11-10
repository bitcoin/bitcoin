// Copyright (C) 2015-2018 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com

#ifndef BOOST_OPTIONAL_DETAIL_OPTIONAL_REFERENCE_SPEC_AJK_03OCT2015_HPP
#define BOOST_OPTIONAL_DETAIL_OPTIONAL_REFERENCE_SPEC_AJK_03OCT2015_HPP

#ifdef BOOST_OPTIONAL_CONFIG_NO_PROPER_ASSIGN_FROM_CONST_INT
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_const.hpp>
#endif

# if 1

namespace boost {

namespace detail {

#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES

template <class From>
void prevent_binding_rvalue()
{
#ifndef BOOST_OPTIONAL_CONFIG_ALLOW_BINDING_TO_RVALUES
    BOOST_STATIC_ASSERT_MSG(boost::is_lvalue_reference<From>::value, 
                            "binding rvalue references to optional lvalue references is disallowed");
#endif    
}

template <class T>
BOOST_DEDUCED_TYPENAME boost::remove_reference<T>::type& forward_reference(T&& r)
{
    BOOST_STATIC_ASSERT_MSG(boost::is_lvalue_reference<T>::value, 
                            "binding rvalue references to optional lvalue references is disallowed");
    return boost::forward<T>(r);
}

#endif // BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES


template <class T>
struct is_const_integral
{
  static const bool value = boost::is_const<T>::value && boost::is_integral<T>::value;
};

template <class T>
struct is_const_integral_bad_for_conversion
{
#if (!defined BOOST_OPTIONAL_CONFIG_ALLOW_BINDING_TO_RVALUES) && (defined BOOST_OPTIONAL_CONFIG_NO_PROPER_CONVERT_FROM_CONST_INT)
  static const bool value = boost::is_const<T>::value && boost::is_integral<T>::value;
#else
  static const bool value = false;
#endif
};

template <class From>
void prevent_assignment_from_false_const_integral()
{
#ifndef BOOST_OPTIONAL_CONFIG_ALLOW_BINDING_TO_RVALUES
#ifdef BOOST_OPTIONAL_CONFIG_NO_PROPER_ASSIGN_FROM_CONST_INT
    // MSVC compiler without rvalue refernces: we need to disable the asignment from
    // const integral lvalue reference, as it may be an invalid temporary
    BOOST_STATIC_ASSERT_MSG(!is_const_integral<From>::value, 
                            "binding const lvalue references to integral types is disabled in this compiler");
#endif
#endif   
}


template <class T>
struct is_optional_
{
  static const bool value = false;
};

template <class U>
struct is_optional_< ::boost::optional<U> >
{
  static const bool value = true;
};

template <class T>
struct is_no_optional
{
  static const bool value = !is_optional_<BOOST_DEDUCED_TYPENAME boost::decay<T>::type>::value;
};


template <class T, class U>
  struct is_same_decayed
  {
    static const bool value = ::boost::is_same<T, BOOST_DEDUCED_TYPENAME ::boost::remove_reference<U>::type>::value
                           || ::boost::is_same<T, const BOOST_DEDUCED_TYPENAME ::boost::remove_reference<U>::type>::value;
  };

template <class T, class U>
struct no_unboxing_cond
{
  static const bool value = is_no_optional<U>::value && !is_same_decayed<T, U>::value;
};


} // namespace detail

template <class T>
class optional<T&> : public optional_detail::optional_tag
{
    T* ptr_;
    
public:
    typedef T& value_type;
    typedef T& reference_type;
    typedef T& reference_const_type;
    typedef T& rval_reference_type;
    typedef T* pointer_type;
    typedef T* pointer_const_type;
    
    optional() BOOST_NOEXCEPT : ptr_() {}
    optional(none_t) BOOST_NOEXCEPT : ptr_() {}  

    template <class U>
        explicit optional(const optional<U&>& rhs) BOOST_NOEXCEPT : ptr_(rhs.get_ptr()) {}
    optional(const optional& rhs) BOOST_NOEXCEPT : ptr_(rhs.get_ptr()) {}
    
    // the following two implement a 'conditionally explicit' constructor: condition is a hack for buggy compilers with srewed conversion construction from const int
    template <class U>
      explicit optional(U& rhs, BOOST_DEDUCED_TYPENAME boost::enable_if_c<detail::is_same_decayed<T, U>::value && detail::is_const_integral_bad_for_conversion<U>::value, bool>::type = true) BOOST_NOEXCEPT
      : ptr_(boost::addressof(rhs)) {}
      
    template <class U>
      optional(U& rhs, BOOST_DEDUCED_TYPENAME boost::enable_if_c<detail::is_same_decayed<T, U>::value && !detail::is_const_integral_bad_for_conversion<U>::value, bool>::type = true) BOOST_NOEXCEPT
      : ptr_(boost::addressof(rhs)) {}

    optional& operator=(const optional& rhs) BOOST_NOEXCEPT { ptr_ = rhs.get_ptr(); return *this; }
    template <class U>
        optional& operator=(const optional<U&>& rhs) BOOST_NOEXCEPT { ptr_ = rhs.get_ptr(); return *this; }
    optional& operator=(none_t) BOOST_NOEXCEPT { ptr_ = 0; return *this; }
    
    
    void swap(optional& rhs) BOOST_NOEXCEPT { std::swap(ptr_, rhs.ptr_); }
    T& get() const { BOOST_ASSERT(ptr_); return   *ptr_; }

    T* get_ptr() const BOOST_NOEXCEPT { return ptr_; }
    T* operator->() const { BOOST_ASSERT(ptr_); return ptr_; }
    T& operator*() const { BOOST_ASSERT(ptr_); return *ptr_; }
    
    T& value() const
    {
      if (this->is_initialized())
        return this->get();
      else
        throw_exception(bad_optional_access());
    }
    
    bool operator!() const BOOST_NOEXCEPT { return ptr_ == 0; }  
    BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()
      
    void reset() BOOST_NOEXCEPT { ptr_ = 0; }

    bool is_initialized() const BOOST_NOEXCEPT { return ptr_ != 0; }
    bool has_value() const BOOST_NOEXCEPT { return ptr_ != 0; }
    
    template <typename F>
    optional<typename boost::result_of<F(T&)>::type> map(F f) const
    {
      if (this->has_value())
        return f(this->get());
      else
        return none;
    }

    template <typename F>
    optional<typename optional_detail::optional_value_type<typename boost::result_of<F(T&)>::type>::type> flat_map(F f) const
      {
        if (this->has_value())
          return f(get());
        else
          return none;
      }
    
#ifndef BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES   
 
    optional(T&& /* rhs */) BOOST_NOEXCEPT { detail::prevent_binding_rvalue<T&&>(); }
    
    template <class R>
        optional(R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::no_unboxing_cond<T, R>, bool>::type = true) BOOST_NOEXCEPT
        : ptr_(boost::addressof(r)) { detail::prevent_binding_rvalue<R>(); }
        
    template <class R>
        optional(bool cond, R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, bool>::type = true) BOOST_NOEXCEPT
        : ptr_(cond ? boost::addressof(r) : 0) { detail::prevent_binding_rvalue<R>(); }
        
    template <class R>
        BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, optional<T&>&>::type
        operator=(R&& r) BOOST_NOEXCEPT { detail::prevent_binding_rvalue<R>(); ptr_ = boost::addressof(r); return *this; }
        
    template <class R>
        void emplace(R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, bool>::type = true) BOOST_NOEXCEPT
        { detail::prevent_binding_rvalue<R>(); ptr_ = boost::addressof(r); }
        
    template <class R>
      T& get_value_or(R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, bool>::type = true) const BOOST_NOEXCEPT
      { detail::prevent_binding_rvalue<R>(); return ptr_ ? *ptr_ : r; }
      
    template <class R>
        T& value_or(R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, bool>::type = true) const BOOST_NOEXCEPT
        { detail::prevent_binding_rvalue<R>(); return ptr_ ? *ptr_ : r; }
        
    template <class R>
      void reset(R&& r, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<R>, bool>::type = true) BOOST_NOEXCEPT
      { detail::prevent_binding_rvalue<R>(); ptr_ = boost::addressof(r); }
      
    template <class F>
        T& value_or_eval(F f) const { return ptr_ ? *ptr_ : detail::forward_reference(f()); }
      
#else  // BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES

    
    // the following two implement a 'conditionally explicit' constructor
    template <class U>
      explicit optional(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if_c<detail::no_unboxing_cond<T, U>::value && detail::is_const_integral_bad_for_conversion<U>::value, bool>::type = true) BOOST_NOEXCEPT
      : ptr_(boost::addressof(v)) { }
      
    template <class U>
      optional(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if_c<detail::no_unboxing_cond<T, U>::value && !detail::is_const_integral_bad_for_conversion<U>::value, bool>::type = true) BOOST_NOEXCEPT
      : ptr_(boost::addressof(v)) { }
        
    template <class U>
      optional(bool cond, U& v, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, bool>::type = true) BOOST_NOEXCEPT : ptr_(cond ? boost::addressof(v) : 0) {}

    template <class U>
      BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, optional<T&>&>::type
      operator=(U& v) BOOST_NOEXCEPT
      {
        detail::prevent_assignment_from_false_const_integral<U>();
        ptr_ = boost::addressof(v); return *this;
      }

    template <class U>
        void emplace(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, bool>::type = true) BOOST_NOEXCEPT
        { ptr_ = boost::addressof(v); }
        
    template <class U>
      T& get_value_or(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, bool>::type = true) const BOOST_NOEXCEPT
      { return ptr_ ? *ptr_ : v; }
      
    template <class U>
        T& value_or(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, bool>::type = true) const BOOST_NOEXCEPT
        { return ptr_ ? *ptr_ : v; }
        
    template <class U>
      void reset(U& v, BOOST_DEDUCED_TYPENAME boost::enable_if<detail::is_no_optional<U>, bool>::type = true) BOOST_NOEXCEPT
      { ptr_ = boost::addressof(v); }
      
    template <class F>
      T& value_or_eval(F f) const { return ptr_ ? *ptr_ : f(); }
      
#endif // BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
};

template <class T> 
  void swap ( optional<T&>& x, optional<T&>& y) BOOST_NOEXCEPT
{
  x.swap(y);
}

} // namespace boost

#endif // 1/0

#endif // header guard
