// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef OBJECT_CORE_DWA2002615_HPP
# define OBJECT_CORE_DWA2002615_HPP

# define BOOST_PYTHON_OBJECT_HAS_IS_NONE // added 2010-03-15 by rwgk

# include <boost/python/detail/prefix.hpp>

# include <boost/type.hpp>

# include <boost/python/call.hpp>
# include <boost/python/handle_fwd.hpp>
# include <boost/python/errors.hpp>
# include <boost/python/refcount.hpp>
# include <boost/python/detail/preprocessor.hpp>
# include <boost/python/tag.hpp>
# include <boost/python/def_visitor.hpp>

# include <boost/python/detail/raw_pyobject.hpp>
# include <boost/python/detail/dependent.hpp>

# include <boost/python/object/forward.hpp>
# include <boost/python/object/add_to_namespace.hpp>

# include <boost/preprocessor/iterate.hpp>
# include <boost/preprocessor/debug/line.hpp>

# include <boost/python/detail/is_xxx.hpp>
# include <boost/python/detail/string_literal.hpp>
# include <boost/python/detail/def_helper_fwd.hpp>
# include <boost/python/detail/type_traits.hpp>

namespace boost { namespace python { 

namespace detail
{
  class kwds_proxy; 
  class args_proxy; 
} 

namespace converter
{
  template <class T> struct arg_to_python;
}

// Put this in an inner namespace so that the generalized operators won't take over
namespace api
{
  
// This file contains the definition of the object class and enough to
// construct/copy it, but not enough to do operations like
// attribute/item access or addition.

  template <class Policies> class proxy;
  
  struct const_attribute_policies;
  struct attribute_policies;
  struct const_objattribute_policies;
  struct objattribute_policies;
  struct const_item_policies;
  struct item_policies;
  struct const_slice_policies;
  struct slice_policies;
  class slice_nil;

  typedef proxy<const_attribute_policies> const_object_attribute;
  typedef proxy<attribute_policies> object_attribute;
  typedef proxy<const_objattribute_policies> const_object_objattribute;
  typedef proxy<objattribute_policies> object_objattribute;
  typedef proxy<const_item_policies> const_object_item;
  typedef proxy<item_policies> object_item;
  typedef proxy<const_slice_policies> const_object_slice;
  typedef proxy<slice_policies> object_slice;

  //
  // is_proxy -- proxy type detection
  //
  BOOST_PYTHON_IS_XXX_DEF(proxy, boost::python::api::proxy, 1)

  template <class T> struct object_initializer;
  
  class object;
  typedef PyObject* (object::*bool_type)() const;
  
  template <class U>
  class object_operators : public def_visitor<U>
  {
   protected:
      typedef object const& object_cref;
   public:
      // function call
      //
      object operator()() const;

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PYTHON_MAX_ARITY, <boost/python/object_call.hpp>))
# include BOOST_PP_ITERATE()
    
      detail::args_proxy operator* () const; 
      object operator()(detail::args_proxy const &args) const; 
      object operator()(detail::args_proxy const &args, 
                        detail::kwds_proxy const &kwds) const; 

      // truth value testing
      //
      operator bool_type() const;
      bool operator!() const; // needed for vc6

      // Attribute access
      //
      const_object_attribute attr(char const*) const;
      object_attribute attr(char const*);
      const_object_objattribute attr(object const&) const;
      object_objattribute attr(object const&);

      // Wrap 'in' operator (aka. __contains__)
      template <class T>
      object contains(T const& key) const;
      
      // item access
      //
      const_object_item operator[](object_cref) const;
      object_item operator[](object_cref);
    
      template <class T>
      const_object_item
      operator[](T const& key) const;
    
      template <class T>
      object_item
      operator[](T const& key);

      // slicing
      //
      const_object_slice slice(object_cref, object_cref) const;
      object_slice slice(object_cref, object_cref);

      const_object_slice slice(slice_nil, object_cref) const;
      object_slice slice(slice_nil, object_cref);
                             
      const_object_slice slice(object_cref, slice_nil) const;
      object_slice slice(object_cref, slice_nil);

      const_object_slice slice(slice_nil, slice_nil) const;
      object_slice slice(slice_nil, slice_nil);

      template <class T, class V>
      const_object_slice
      slice(T const& start, V const& end) const;
    
      template <class T, class V>
      object_slice
      slice(T const& start, V const& end);
      
   private: // def visitation for adding callable objects as class methods
      
      template <class ClassT, class DocStringT>
      void visit(ClassT& cl, char const* name, python::detail::def_helper<DocStringT> const& helper) const
      {
          // It's too late to specify anything other than docstrings if
          // the callable object is already wrapped.
          BOOST_STATIC_ASSERT(
              (detail::is_same<char const*,DocStringT>::value
               || detail::is_string_literal<DocStringT const>::value));
        
          objects::add_to_namespace(cl, name, this->derived_visitor(), helper.doc());
      }

      friend class python::def_visitor_access;
      
   private:
     // there is a confirmed CWPro8 codegen bug here. We prevent the
     // early destruction of a temporary by binding a named object
     // instead.
# if __MWERKS__ < 0x3000 || __MWERKS__ > 0x3003
    typedef object const& object_cref2;
# else
    typedef object const object_cref2;
# endif
  };

  
  // VC6 and VC7 require this base class in order to generate the
  // correct copy constructor for object. We can't define it there
  // explicitly or it will complain of ambiguity.
  struct object_base : object_operators<object>
  {
      // copy constructor without NULL checking, for efficiency. 
      inline object_base(object_base const&);
      inline object_base(PyObject* ptr);
      
      inline object_base& operator=(object_base const& rhs);
      inline ~object_base();
        
      // Underlying object access -- returns a borrowed reference
      inline PyObject* ptr() const;

      inline bool is_none() const;

   private:
      PyObject* m_ptr;
  };

  template <class T, class U>
  struct is_derived
    : boost::python::detail::is_convertible<
          typename detail::remove_reference<T>::type*
        , U const*
      >
  {};

  template <class T>
  typename objects::unforward_cref<T>::type do_unforward_cref(T const& x)
  {
      return x;
  }

  class object;
  
  template <class T>
  PyObject* object_base_initializer(T const& x)
  {
      typedef typename is_derived<
          BOOST_DEDUCED_TYPENAME objects::unforward_cref<T>::type
        , object
      >::type is_obj;

      return object_initializer<
          BOOST_DEDUCED_TYPENAME unwrap_reference<T>::type
      >::get(
            x
          , is_obj()
      );
  }
  
  class object : public object_base
  {
   public:
      // default constructor creates a None object
      object();
      
      // explicit conversion from any C++ object to Python
      template <class T>
      explicit object(T const& x)
        : object_base(object_base_initializer(x))
      {
      }

      // Throw error_already_set() if the handle is null.
      BOOST_PYTHON_DECL explicit object(handle<> const&);
   private:
      
   public: // implementation detail -- for internal use only
      explicit object(detail::borrowed_reference);
      explicit object(detail::new_reference);
      explicit object(detail::new_non_null_reference);
  };

  // Macros for forwarding constructors in classes derived from
  // object. Derived classes will usually want these as an
  // implementation detail
# define BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(derived, base)               \
    inline explicit derived(::boost::python::detail::borrowed_reference p)     \
        : base(p) {}                                                           \
    inline explicit derived(::boost::python::detail::new_reference p)          \
        : base(p) {}                                                           \
    inline explicit derived(::boost::python::detail::new_non_null_reference p) \
        : base(p) {}

  //
  // object_initializer -- get the handle to construct the object with,
  // based on whether T is a proxy or derived from object
  //
  template <bool is_proxy = false, bool is_object_manager = false>
  struct object_initializer_impl
  {
      static PyObject*
      get(object const& x, detail::true_)
      {
          return python::incref(x.ptr());
      }
      
      template <class T>
      static PyObject*
      get(T const& x, detail::false_)
      {
          return python::incref(converter::arg_to_python<T>(x).get());
      }
  };
      
  template <>
  struct object_initializer_impl<true, false>
  {
      template <class Policies>
      static PyObject* 
      get(proxy<Policies> const& x, detail::false_)
      {
          return python::incref(x.operator object().ptr());
      }
  };

  template <>
  struct object_initializer_impl<false, true>
  {
      template <class T, class U>
      static PyObject*
      get(T const& x, U)
      {
          return python::incref(get_managed_object(x, boost::python::tag));
      }
  };

  template <>
  struct object_initializer_impl<true, true>
  {}; // empty implementation should cause an error

  template <class T>
  struct object_initializer : object_initializer_impl<
      is_proxy<T>::value
    , converter::is_object_manager<T>::value
  >
  {};

}
using api::object;
template <class T> struct extract;

//
// implementation
//

namespace detail 
{

class call_proxy 
{ 
public: 
  call_proxy(object target) : m_target(target) {} 
  operator object() const { return m_target;} 
 
 private: 
    object m_target; 
}; 
 
class kwds_proxy : public call_proxy 
{ 
public: 
  kwds_proxy(object o = object()) : call_proxy(o) {} 
}; 
class args_proxy : public call_proxy 
{ 
public: 
  args_proxy(object o) : call_proxy(o) {} 
  kwds_proxy operator* () const { return kwds_proxy(*this);} 
}; 
} 
 
template <typename U> 
detail::args_proxy api::object_operators<U>::operator* () const 
{ 
  object_cref2 x = *static_cast<U const*>(this); 
  return boost::python::detail::args_proxy(x); 
} 
 
template <typename U> 
object api::object_operators<U>::operator()(detail::args_proxy const &args) const 
{ 
  U const& self = *static_cast<U const*>(this); 
  PyObject *result = PyObject_Call(get_managed_object(self, boost::python::tag), 
                                   args.operator object().ptr(), 
                                   0); 
  return object(boost::python::detail::new_reference(result)); 
 
} 
 
template <typename U> 
object api::object_operators<U>::operator()(detail::args_proxy const &args, 
                                            detail::kwds_proxy const &kwds) const 
{ 
  U const& self = *static_cast<U const*>(this); 
  PyObject *result = PyObject_Call(get_managed_object(self, boost::python::tag), 
                                   args.operator object().ptr(), 
                                   kwds.operator object().ptr()); 
  return object(boost::python::detail::new_reference(result)); 
 
}  


template <typename U>
template <class T>
object api::object_operators<U>::contains(T const& key) const
{
    return this->attr("__contains__")(object(key));
}


inline object::object()
    : object_base(python::incref(Py_None))
{}

// copy constructor without NULL checking, for efficiency
inline api::object_base::object_base(object_base const& rhs)
    : m_ptr(python::incref(rhs.m_ptr))
{}

inline api::object_base::object_base(PyObject* p)
    : m_ptr(p)
{}

inline api::object_base& api::object_base::operator=(api::object_base const& rhs)
{
    Py_INCREF(rhs.m_ptr);
    Py_DECREF(this->m_ptr);
    this->m_ptr = rhs.m_ptr;
    return *this;
}

inline api::object_base::~object_base()
{
    assert( Py_REFCNT(m_ptr) > 0 );
    Py_DECREF(m_ptr);
}

inline object::object(detail::borrowed_reference p)
    : object_base(python::incref((PyObject*)p))
{}

inline object::object(detail::new_reference p)
    : object_base(expect_non_null((PyObject*)p))
{}

inline object::object(detail::new_non_null_reference p)
    : object_base((PyObject*)p)
{}

inline PyObject* api::object_base::ptr() const
{
    return m_ptr;
}

inline bool api::object_base::is_none() const
{
    return (m_ptr == Py_None);
}

//
// Converter specialization implementations
//
namespace converter
{
  template <class T> struct object_manager_traits;
  
  template <>
  struct object_manager_traits<object>
  {
      BOOST_STATIC_CONSTANT(bool, is_specialized = true);
      static bool check(PyObject*) { return true; }
      
      static python::detail::new_non_null_reference adopt(PyObject* x)
      {
          return python::detail::new_non_null_reference(x);
      }
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
      static PyTypeObject const *get_pytype() {return 0;}
#endif
  };
}

inline PyObject* get_managed_object(object const& x, tag_t)
{
    return x.ptr();
}

}} // namespace boost::python

# include <boost/python/slice_nil.hpp>

#endif // OBJECT_CORE_DWA2002615_HPP
