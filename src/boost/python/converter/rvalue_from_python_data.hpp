// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef FROM_PYTHON_AUX_DATA_DWA2002128_HPP
# define FROM_PYTHON_AUX_DATA_DWA2002128_HPP

# include <boost/python/converter/constructor_function.hpp>
# include <boost/python/detail/referent_storage.hpp>
# include <boost/python/detail/destroy.hpp>
# include <boost/python/detail/type_traits.hpp>
# include <boost/align/align.hpp>
# include <boost/static_assert.hpp>
# include <cstddef>

// Data management for potential rvalue conversions from Python to C++
// types. When a client requests a conversion to T* or T&, we
// generally require that an object of type T exists in the source
// Python object, and the code here does not apply**. This implements
// conversions which may create new temporaries of type T. The classic
// example is a conversion which converts a Python tuple to a
// std::vector. Since no std::vector lvalue exists in the Python
// object -- it must be created "on-the-fly" by the converter, and
// which must manage the lifetime of the created object.
//
// Note that the client is not precluded from using a registered
// lvalue conversion to T in this case. In other words, we will
// happily accept a Python object which /does/ contain a std::vector
// lvalue, provided an appropriate converter is registered. So, while
// this is an rvalue conversion from the client's point-of-view, the
// converter registry may serve up lvalue or rvalue conversions for
// the target type.
//
// ** C++ argument from_python conversions to T const& are an
// exception to the rule for references: since in C++, const
// references can bind to temporary rvalues, we allow rvalue
// converters to be chosen when the target type is T const& for some
// T.
namespace boost { namespace python { namespace converter { 

// Conversions begin by filling in and returning a copy of this
// structure. The process looks up a converter in the rvalue converter
// registry for the target type. It calls the convertible() function
// of each registered converter, passing the source PyObject* as an
// argument, until a non-null result is returned. This result goes in
// the convertible field, and the converter's construct() function is
// stored in the construct field.
//
// If no appropriate converter is found, conversion fails and the
// convertible field is null. When used in argument conversion for
// wrapped C++ functions, it causes overload resolution to reject the
// current function but not to fail completely. If an exception is
// thrown, overload resolution stops and the exception propagates back
// through the caller.
//
// If an lvalue converter is matched, its convertible() function is
// expected to return a pointer to the stored T object; its
// construct() function will be NULL. The convertible() function of
// rvalue converters may return any non-singular pointer; the actual
// target object will only be available once the converter's
// construct() function is called.
struct rvalue_from_python_stage1_data
{
    void* convertible;
    constructor_function construct;
};

// Augments rvalue_from_python_stage1_data by adding storage for
// constructing an object of remove_reference<T>::type. The
// construct() function of rvalue converters (stored in m_construct
// above) will cast the rvalue_from_python_stage1_data to an
// appropriate instantiation of this template in order to access that
// storage.
template <class T>
struct rvalue_from_python_storage
{
    rvalue_from_python_stage1_data stage1;

    // Storage for the result, in case an rvalue must be constructed
    typename python::detail::referent_storage<
        typename boost::python::detail::add_lvalue_reference<T>::type
    >::type storage;
};

// Augments rvalue_from_python_storage<T> with a destructor. If
// stage1.convertible == storage.bytes, it indicates that an object of
// remove_reference<T>::type has been constructed in storage and
// should will be destroyed in ~rvalue_from_python_data(). It is
// crucial that successful rvalue conversions establish this equality
// and that unsuccessful ones do not.
template <class T>
struct rvalue_from_python_data : rvalue_from_python_storage<T>
{
# if (!defined(__MWERKS__) || __MWERKS__ >= 0x3000) \
        && (!defined(__EDG_VERSION__) || __EDG_VERSION__ >= 245) \
        && (!defined(__DECCXX_VER) || __DECCXX_VER > 60590014) \
        && !defined(BOOST_PYTHON_SYNOPSIS) /* Synopsis' OpenCXX has trouble parsing this */
    // This must always be a POD struct with m_data its first member.
    BOOST_STATIC_ASSERT(BOOST_PYTHON_OFFSETOF(rvalue_from_python_storage<T>,stage1) == 0);
# endif
    
    // The usual constructor 
    rvalue_from_python_data(rvalue_from_python_stage1_data const&);

    // This constructor just sets m_convertible -- used by
    // implicitly_convertible<> to perform the final step of the
    // conversion, where the construct() function is already known.
    rvalue_from_python_data(void* convertible);

    // Destroys any object constructed in the storage.
    ~rvalue_from_python_data();
 private:
    typedef typename boost::python::detail::add_lvalue_reference<
                typename boost::python::detail::add_cv<T>::type>::type ref_type;
};

//
// Implementataions
//
template <class T>
inline rvalue_from_python_data<T>::rvalue_from_python_data(rvalue_from_python_stage1_data const& _stage1)
{
    this->stage1 = _stage1;
}

template <class T>
inline rvalue_from_python_data<T>::rvalue_from_python_data(void* convertible)
{
    this->stage1.convertible = convertible;
}

template <class T>
inline rvalue_from_python_data<T>::~rvalue_from_python_data()
{
    if (this->stage1.convertible == this->storage.bytes)
    {
        size_t allocated = sizeof(this->storage);
        void *ptr = this->storage.bytes;
        void *aligned_storage =
            ::boost::alignment::align(boost::python::detail::alignment_of<T>::value, 0, ptr, allocated);
        python::detail::destroy_referent<ref_type>(aligned_storage);
    }
}

}}} // namespace boost::python::converter

#endif // FROM_PYTHON_AUX_DATA_DWA2002128_HPP
