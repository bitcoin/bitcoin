// Copyright Ralf W. Grosse-Kunstleve 2006.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef DOCSTRING_OPTIONS_RWGK20060111_HPP
# define DOCSTRING_OPTIONS_RWGK20060111_HPP

#include <boost/python/object/function.hpp>

namespace boost { namespace python {

// Note: the static data members are defined in object/function.cpp

class BOOST_PYTHON_DECL docstring_options : boost::noncopyable
{
  public:
      docstring_options(bool show_all=true)
      {
          previous_show_user_defined_ = show_user_defined_;
          previous_show_py_signatures_ = show_py_signatures_;
          previous_show_cpp_signatures_ = show_cpp_signatures_;
          show_user_defined_ = show_all;
          show_cpp_signatures_ = show_all;
          show_py_signatures_ = show_all;
      }

      docstring_options(bool show_user_defined, bool show_signatures)
      {
          previous_show_user_defined_ = show_user_defined_;
          previous_show_cpp_signatures_ = show_cpp_signatures_;
          previous_show_py_signatures_ = show_py_signatures_;
          show_user_defined_ = show_user_defined;
          show_cpp_signatures_ = show_signatures;
          show_py_signatures_ = show_signatures;
      }

      docstring_options(bool show_user_defined, bool show_py_signatures, bool show_cpp_signatures)
      {
          previous_show_user_defined_ = show_user_defined_;
          previous_show_cpp_signatures_ = show_cpp_signatures_;
          previous_show_py_signatures_ = show_py_signatures_;
          show_user_defined_ = show_user_defined;
          show_cpp_signatures_ = show_cpp_signatures;
          show_py_signatures_ = show_py_signatures;
      }

      ~docstring_options()
      {
          show_user_defined_ = previous_show_user_defined_;
          show_cpp_signatures_ = previous_show_cpp_signatures_;
          show_py_signatures_ = previous_show_py_signatures_;
      }

      void
      disable_user_defined() { show_user_defined_ = false; }

      void
      enable_user_defined() { show_user_defined_ = true; }

      void
      disable_py_signatures() 
      {
        show_py_signatures_ = false; 
      }

      void
      enable_py_signatures() 
      {
        show_py_signatures_ = true; 
      }

      void
      disable_cpp_signatures() 
      {
        show_cpp_signatures_ = false; 
      }

      void
      enable_cpp_signatures() 
      {
        show_cpp_signatures_ = true; 
      }

      void
      disable_signatures() 
      {
        show_cpp_signatures_ = false; 
        show_py_signatures_ = false; 
      }

      void
      enable_signatures() 
      {
        show_cpp_signatures_ = true; 
        show_py_signatures_ = true; 
      }

      void
      disable_all()
      {
        show_user_defined_ = false;
        show_cpp_signatures_ = false;
        show_py_signatures_ = false;
      }

      void
      enable_all()
      {
        show_user_defined_ = true;
        show_cpp_signatures_ = true;
        show_py_signatures_ = true;
      }

      friend struct objects::function;

  private:
      static volatile bool show_user_defined_;
      static volatile bool show_cpp_signatures_;
      static volatile bool show_py_signatures_;
      bool previous_show_user_defined_;
      bool previous_show_cpp_signatures_;
      bool previous_show_py_signatures_;
};

}} // namespace boost::python

#endif // DOCSTRING_OPTIONS_RWGK20060111_HPP
