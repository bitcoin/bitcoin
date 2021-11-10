// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef RETURN_FROM_PYTHON_DWA200265_HPP
# define RETURN_FROM_PYTHON_DWA200265_HPP

# include <boost/python/converter/from_python.hpp>
# include <boost/python/converter/rvalue_from_python_data.hpp>
# include <boost/python/converter/registered.hpp>
# include <boost/python/converter/registered_pointee.hpp>
# include <boost/python/converter/object_manager.hpp>
# include <boost/python/detail/void_ptr.hpp>
# include <boost/python/detail/void_return.hpp>
# include <boost/python/errors.hpp>
# include <boost/python/handle.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/mpl/and.hpp>
# include <boost/mpl/bool.hpp>

namespace boost { namespace python { namespace converter { 

template <class T> struct is_object_manager;

namespace detail
{
  template <class T>
  struct return_pointer_from_python
  {
      typedef T result_type;
      T operator()(PyObject*) const;
  };
  
  template <class T>
  struct return_reference_from_python
  {
      typedef T result_type;
      T operator()(PyObject*) const;
  };
  
  template <class T>
  struct return_rvalue_from_python
  {
      typedef T result_type;

      return_rvalue_from_python();
      result_type operator()(PyObject*);
   private:
      rvalue_from_python_data<T> m_data;
  };
  
  template <class T>
  struct return_object_manager_from_python
  {
      typedef T result_type;
      result_type operator()(PyObject*) const;
  };
  
  template <class T>
  struct select_return_from_python
  {
      BOOST_STATIC_CONSTANT(
          bool, obj_mgr = is_object_manager<T>::value);

      BOOST_STATIC_CONSTANT(
          bool, ptr = is_pointer<T>::value);
    
      BOOST_STATIC_CONSTANT(
          bool, ref = is_reference<T>::value);

      typedef typename mpl::if_c<
          obj_mgr
          , return_object_manager_from_python<T>
          , typename mpl::if_c<
              ptr
              , return_pointer_from_python<T>
              , typename mpl::if_c<
                  ref
                  , return_reference_from_python<T>
                  , return_rvalue_from_python<T>
                >::type
            >::type
         >::type type;
  };
}

template <class T>
struct return_from_python
    : detail::select_return_from_python<T>::type
{
};

// Specialization as a convenience for call and call_method
template <>
struct return_from_python<void>
{
    typedef python::detail::returnable<void>::type result_type;
    
    result_type operator()(PyObject* x) const
    {
        (void_result_from_python)(x);
# ifdef BOOST_NO_VOID_RETURNS
        return result_type();
# endif 
    }
};

//
// Implementations
//
namespace detail
{
  template <class T>
  inline return_rvalue_from_python<T>::return_rvalue_from_python()
      : m_data(
          const_cast<registration*>(&registered<T>::converters)
          )
  {
  }
  
  template <class T>
  inline typename return_rvalue_from_python<T>::result_type
  return_rvalue_from_python<T>::operator()(PyObject* obj)
  {
    // Take possession of the source object here.  If the result is in
    // fact going to be a copy of an lvalue embedded in the object,
    // and we take possession inside rvalue_result_from_python, it
    // will be destroyed too early.
    handle<> holder(obj);

      return *(T*)
          (rvalue_result_from_python)(obj, m_data.stage1);
  }

  template <class T>
  inline T return_reference_from_python<T>::operator()(PyObject* obj) const
  {
      return python::detail::void_ptr_to_reference(
          (reference_result_from_python)(obj, registered<T>::converters)
          , (T(*)())0);
  }

  template <class T>
  inline T return_pointer_from_python<T>::operator()(PyObject* obj) const
  {
      return T(
          (pointer_result_from_python)(obj, registered_pointee<T>::converters)
          );
  }

  template <class T>
  inline T return_object_manager_from_python<T>::operator()(PyObject* obj) const
  {
      return T(
          object_manager_traits<T>::adopt(expect_non_null(obj))
          );
  }
}
  
}}} // namespace boost::python::converter

#endif // RETURN_FROM_PYTHON_DWA200265_HPP
