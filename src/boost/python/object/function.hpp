// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FUNCTION_DWA20011214_HPP
# define FUNCTION_DWA20011214_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/args_fwd.hpp>
# include <boost/python/handle.hpp>
# include <boost/function/function2.hpp>
# include <boost/python/object_core.hpp>
# include <boost/python/object/py_function.hpp>

namespace boost { namespace python { namespace objects { 


struct BOOST_PYTHON_DECL function : PyObject
{
    function(
        py_function const&
        , python::detail::keyword const* names_and_defaults
        , unsigned num_keywords);
      
    ~function();
    
    PyObject* call(PyObject*, PyObject*) const;

    // Add an attribute to the name_space with the given name. If it is
    // a function object (this class), and an existing function is
    // already there, add it as an overload.
    static void add_to_namespace(
        object const& name_space, char const* name, object const& attribute);

    static void add_to_namespace(
        object const& name_space, char const* name, object const& attribute, char const* doc);

    object const& doc() const;
    void doc(object const& x);
    
    object const& name() const;

    object const& get_namespace() const { return m_namespace; }
    
 private: // helper functions
    object signature(bool show_return_type=false) const;
    object signatures(bool show_return_type=false) const;
    void argument_error(PyObject* args, PyObject* keywords) const;
    void add_overload(handle<function> const&);
    
 private: // data members
    py_function m_fn;
    handle<function> m_overloads;
    object m_name;
    object m_namespace;
    object m_doc;
    object m_arg_names;
    unsigned m_nkeyword_values;
    friend class function_doc_signature_generator;
};

//
// implementations
//
inline object const& function::doc() const
{
    return this->m_doc;
}

inline void function::doc(object const& x)
{
    this->m_doc = x;
}

inline object const& function::name() const
{
    return this->m_name;
}
  
}}} // namespace boost::python::objects

#endif // FUNCTION_DWA20011214_HPP
