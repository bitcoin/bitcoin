// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef LONG_DWA2002627_HPP
# define LONG_DWA2002627_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/object.hpp>
# include <boost/python/converter/pytype_object_mgr_traits.hpp>

namespace boost { namespace python { 

namespace detail
{
  struct BOOST_PYTHON_DECL long_base : object
  {
   protected:
      long_base(); // new long_
      explicit long_base(object_cref rhs);
      explicit long_base(object_cref rhs, object_cref base);
      
      BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(long_base, object)
          
   private:
      static detail::new_reference call(object const&);
      static detail::new_reference call(object const&, object const&);
  };
}

class long_ : public detail::long_base
{
    typedef detail::long_base base;
 public:
    long_() {} // new long_

    template <class T>
    explicit long_(T const& rhs)
        : detail::long_base(object(rhs))
    {
    }

    template <class T, class U>
    explicit long_(T const& rhs, U const& base)
        : detail::long_base(object(rhs), object(base))
    {
    }
    
 public: // implementation detail -- for internal use only
    BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(long_, base)
};

//
// Converter Specializations
//
namespace converter
{
  template <>
  struct object_manager_traits<long_>
      : pytype_object_manager_traits<&PyLong_Type,long_>
  {
  };
}

}} // namespace boost::python

#endif // LONG_DWA2002627_HPP
