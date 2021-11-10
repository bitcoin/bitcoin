// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef TO_PYTHON_CONVERTER_DWA200221_HPP
# define TO_PYTHON_CONVERTER_DWA200221_HPP

# include <boost/python/detail/prefix.hpp>

# include <boost/python/converter/registry.hpp>
# include <boost/python/converter/as_to_python_function.hpp>
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
# include <boost/python/converter/pytype_function.hpp>
#endif
# include <boost/python/type_id.hpp>

namespace boost { namespace python { 

#if 0 //get_pytype member detection
namespace detail
{
    typedef char yes_type;
    typedef struct {char a[2]; } no_type;
    template<PyTypeObject const * (*f)()> struct test_get_pytype1 { };
    template<PyTypeObject * (*f)()>          struct test_get_pytype2 { };

    template<class T> yes_type tester(test_get_pytype1<&T::get_pytype>*);

    template<class T> yes_type tester(test_get_pytype2<&T::get_pytype>*);

    template<class T> no_type tester(...);

    template<class T>
    struct test_get_pytype_base  
    {
        BOOST_STATIC_CONSTANT(bool, value= (sizeof(detail::tester<T>(0)) == sizeof(yes_type)));
    };

    template<class T>
    struct test_get_pytype : boost::mpl::bool_<test_get_pytype_base<T>::value> 
    {
    };

}
#endif

template < class T, class Conversion, bool has_get_pytype=false >
struct to_python_converter 
{
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
    typedef boost::mpl::bool_<has_get_pytype> HasGetPytype;

    static PyTypeObject const* get_pytype_1(boost::mpl::true_ *)
    {
        return Conversion::get_pytype();
    }

    static PyTypeObject const* get_pytype_1(boost::mpl::false_ *)
    {
        return 0;
    }
    static PyTypeObject const* get_pytype_impl()
    {
        return get_pytype_1((HasGetPytype*)0);
    }
#endif
    
    to_python_converter();
};

//
// implementation
//

template <class T, class Conversion ,bool has_get_pytype>
to_python_converter<T,Conversion, has_get_pytype>::to_python_converter()
{
    typedef converter::as_to_python_function<
        T, Conversion
        > normalized;
            
    converter::registry::insert(
        &normalized::convert
        , type_id<T>()
#ifndef BOOST_PYTHON_NO_PY_SIGNATURES
        , &get_pytype_impl
#endif
        );
}

}} // namespace boost::python

#endif // TO_PYTHON_CONVERTER_DWA200221_HPP

