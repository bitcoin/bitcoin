// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef POINTER_TYPE_ID_DWA2002222_HPP
# define POINTER_TYPE_ID_DWA2002222_HPP

# include <boost/python/type_id.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python { namespace converter { 

namespace detail
{
  template <bool is_ref = false>
  struct pointer_typeid_select
  {
      template <class T>
      static inline type_info execute(T*(*)() = 0)
      {
          return type_id<T>();
      }
  };

  template <>
  struct pointer_typeid_select<true>
  {
      template <class T>
      static inline type_info execute(T* const volatile&(*)() = 0)
      {
          return type_id<T>();
      }
    
      template <class T>
      static inline type_info execute(T*volatile&(*)() = 0)
      {
          return type_id<T>();
      }
    
      template <class T>
      static inline type_info execute(T*const&(*)() = 0)
      {
          return type_id<T>();
      }

      template <class T>
      static inline type_info execute(T*&(*)() = 0)
      {
          return type_id<T>();
      }
  };
}

// Usage: pointer_type_id<T>()
//
// Returns a type_info associated with the type pointed
// to by T, which may be a pointer or a reference to a pointer.
template <class T>
type_info pointer_type_id(T(*)() = 0)
{
    return detail::pointer_typeid_select<
          boost::python::detail::is_lvalue_reference<T>::value
        >::execute((T(*)())0);
}

}}} // namespace boost::python::converter

#endif // POINTER_TYPE_ID_DWA2002222_HPP
