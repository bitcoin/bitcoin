// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef CLASS_DWA20011214_HPP
# define CLASS_DWA20011214_HPP

# include <boost/python/detail/prefix.hpp>
# include <boost/python/object_core.hpp>
# include <boost/python/type_id.hpp>
# include <cstddef>

namespace boost { namespace python {

namespace objects { 

struct BOOST_PYTHON_DECL class_base : python::api::object
{
    // constructor
    class_base(
        char const* name              // The name of the class
        
        , std::size_t num_types         // A list of class_ids. The first is the type
        , type_info const*const types    // this is wrapping. The rest are the types of
                                        // any bases.
        
        , char const* doc = 0           // Docstring, if any.
        );


    // Implementation detail. Hiding this in the private section would
    // require use of template friend declarations.
    void enable_pickling_(bool getstate_manages_dict);

 protected:
    void add_property(
        char const* name, object const& fget, char const* docstr);
    void add_property(char const* name, 
        object const& fget, object const& fset, char const* docstr);

    void add_static_property(char const* name, object const& fget);
    void add_static_property(char const* name, object const& fget, object const& fset);
    
    // Retrieve the underlying object
    void setattr(char const* name, object const&);

    // Set a special attribute in the class which tells Boost.Python
    // to allocate extra bytes for embedded C++ objects in Python
    // instances.
    void set_instance_size(std::size_t bytes);

    // Set an __init__ function which throws an appropriate exception
    // for abstract classes.
    void def_no_init();

    // Effects:
    //  setattr(self, staticmethod(getattr(self, method_name)))
    void make_method_static(const char *method_name);
};

}}} // namespace boost::python::objects

#endif // CLASS_DWA20011214_HPP
