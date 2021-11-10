// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CAST_DWA200269_HPP
# define CAST_DWA200269_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/type_traits.hpp>

# include <boost/type.hpp>
# include <boost/python/base_type_traits.hpp>
# include <boost/python/detail/convertible.hpp>

namespace boost { namespace python { 

namespace detail
{
  template <class Source, class Target> inline Target* upcast_impl(Source*, Target*);
  
  template <class Source, class Target>
  inline Target* upcast(Source* p, yes_convertible, no_convertible, Target*)
  {
      return p;
  }

  template <class Source, class Target>
  inline Target* upcast(Source* p, no_convertible, no_convertible, Target*)
  {
      typedef typename base_type_traits<Source>::type base;
      
      return detail::upcast_impl((base*)p, (Target*)0);
  }

  template <bool is_same = true>
  struct upcaster
  {
      template <class T>
      static inline T* execute(T* x, T*) { return x; }
  };
  
  template <>
  struct upcaster<false>
  {
      template <class Source, class Target>
      static inline Target* execute(Source* x, Target*)
      {
          return detail::upcast(
              x, detail::convertible<Target*>::check(x)
              , detail::convertible<Source*>::check((Target*)0)
              , (Target*)0);
      }
  };


  template <class Target, class Source>
  inline Target* downcast(Source* p, yes_convertible)
  {
      return static_cast<Target*>(p);
  }

  template <class Target, class Source>
  inline Target* downcast(Source* p, no_convertible, boost::type<Target>* = 0)
  {
      typedef typename base_type_traits<Source>::type base;
      return (Target*)detail::downcast<base>(p, convertible<Source*>::check((base*)0));
  }

  template <class T>
  inline void assert_castable(boost::type<T>* = 0)
  {
      typedef char must_be_a_complete_type[sizeof(T)] BOOST_ATTRIBUTE_UNUSED;
  }

  template <class Source, class Target>
  inline Target* upcast_impl(Source* x, Target*)
  {
      typedef typename detail::add_cv<Source>::type src_t;
      typedef typename detail::add_cv<Target>::type target_t;
      bool const same = detail::is_same<src_t,target_t>::value;
      
      return detail::upcaster<same>::execute(x, (Target*)0);
  }
}

template <class Target, class Source>
inline Target* upcast(Source* x, Target* = 0)
{
    detail::assert_castable<Source>();
    detail::assert_castable<Target>();
    return detail::upcast_impl(x, (Target*)0);
    
}

template <class Target, class Source>
inline Target* downcast(Source* x, Target* = 0)
{
    detail::assert_castable<Source>();
    detail::assert_castable<Target>();
    return detail::downcast<Target>(x, detail::convertible<Source*>::check((Target*)0));
}

}} // namespace boost::python

#endif // CAST_DWA200269_HPP
