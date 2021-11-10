// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FUNCTION_HANDLE_DWA2002725_HPP
# define FUNCTION_HANDLE_DWA2002725_HPP
# include <boost/python/handle.hpp>
# include <boost/python/detail/caller.hpp>
# include <boost/python/default_call_policies.hpp>
# include <boost/python/object/py_function.hpp>
# include <boost/python/signature.hpp>

namespace boost { namespace python { namespace objects { 

BOOST_PYTHON_DECL handle<> function_handle_impl(py_function const& f);

// Just like function_object, but returns a handle<> instead. Using
// this for arg_to_python<> allows us to break a circular dependency
// between object and arg_to_python.
template <class F, class Signature>
inline handle<> function_handle(F const& f, Signature)
{
    enum { n_arguments = mpl::size<Signature>::value - 1 };

    return objects::function_handle_impl(
        python::detail::caller<
            F,default_call_policies,Signature
        >(
            f, default_call_policies()
         )
    );
}

// Just like make_function, but returns a handle<> intead. Same
// reasoning as above.
template <class F>
handle<> make_function_handle(F f)
{
    return objects::function_handle(f, python::detail::get_signature(f));
}

}}} // namespace boost::python::objects

#endif // FUNCTION_HANDLE_DWA2002725_HPP
