// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FUNCTION_OBJECT_DWA2002725_HPP
# define FUNCTION_OBJECT_DWA2002725_HPP
# include <boost/python/detail/prefix.hpp>
# include <boost/function/function2.hpp>
# include <boost/python/object_core.hpp>
# include <boost/python/args_fwd.hpp>
# include <boost/python/object/py_function.hpp>

namespace boost { namespace python {

namespace objects
{ 
  BOOST_PYTHON_DECL api::object function_object(
      py_function const& f
      , python::detail::keyword_range const&);

  BOOST_PYTHON_DECL api::object function_object(
      py_function const& f
      , python::detail::keyword_range const&);

  BOOST_PYTHON_DECL api::object function_object(py_function const& f);

  // Add an attribute to the name_space with the given name. If it is
  // a Boost.Python function object
  // (boost/python/object/function.hpp), and an existing function is
  // already there, add it as an overload.
  BOOST_PYTHON_DECL void add_to_namespace(
      object const& name_space, char const* name, object const& attribute);

  BOOST_PYTHON_DECL void add_to_namespace(
      object const& name_space, char const* name, object const& attribute, char const* doc);
}

}} // namespace boost::python::objects

#endif // FUNCTION_OBJECT_DWA2002725_HPP
