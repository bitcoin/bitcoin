// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef WRAPPER_BASE_DWA2004722_HPP
# define WRAPPER_BASE_DWA2004722_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python {

class override;

namespace detail
{
  class wrapper_base;
  
  namespace wrapper_base_ // ADL disabler
  {
    inline PyObject* get_owner(wrapper_base const volatile& w);

    inline PyObject*
    owner_impl(void const volatile* /*x*/, detail::false_)
    {
        return 0;
    }
    
    template <class T>
    inline PyObject*
    owner_impl(T const volatile* x, detail::true_);
    
    template <class T>
    inline PyObject*
    owner(T const volatile* x)
    {
        return wrapper_base_::owner_impl(x,is_polymorphic<T>());
    }
  }
  
  class BOOST_PYTHON_DECL wrapper_base
  {
      friend void initialize_wrapper(PyObject* self, wrapper_base* w);
      friend PyObject* wrapper_base_::get_owner(wrapper_base const volatile& w);
   protected:
      wrapper_base() : m_self(0) {}
          
      override get_override(
          char const* name, PyTypeObject* class_object) const;

   private:
      void detach();
      
   private:
      PyObject* m_self;
  };

  namespace wrapper_base_ // ADL disabler
  {
    template <class T>
    inline PyObject*
    owner_impl(T const volatile* x, detail::true_)
    {
        if (wrapper_base const volatile* w = dynamic_cast<wrapper_base const volatile*>(x))
        {
            return wrapper_base_::get_owner(*w);
        }
        return 0;
    }
    
    inline PyObject* get_owner(wrapper_base const volatile& w)
    {
        return w.m_self;
    }
  }
  
  inline void initialize_wrapper(PyObject* self, wrapper_base* w)
  {
      w->m_self = self;
  }

  inline void initialize_wrapper(PyObject* /*self*/, ...) {}

  
  
} // namespace detail

}} // namespace boost::python

#endif // WRAPPER_BASE_DWA2004722_HPP
