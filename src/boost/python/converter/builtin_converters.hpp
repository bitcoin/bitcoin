// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BUILTIN_CONVERTERS_DWA2002124_HPP
# define BUILTIN_CONVERTERS_DWA2002124_HPP
# include <boost/python/detail/prefix.hpp>
# include <boost/python/detail/none.hpp>
# include <boost/python/handle.hpp>
# include <boost/python/ssize_t.hpp>
# include <boost/implicit_cast.hpp>
# include <string>
# include <complex>
# include <boost/limits.hpp>

// Since all we can use to decide how to convert an object to_python
// is its C++ type, there can be only one such converter for each
// type. Therefore, for built-in conversions we can bypass registry
// lookups using explicit specializations of arg_to_python and
// result_to_python.

namespace boost { namespace python {

namespace converter
{
  template <class T> struct arg_to_python;
  BOOST_PYTHON_DECL PyObject* do_return_to_python(char);
  BOOST_PYTHON_DECL PyObject* do_return_to_python(char const*);
  BOOST_PYTHON_DECL PyObject* do_return_to_python(PyObject*);
  BOOST_PYTHON_DECL PyObject* do_arg_to_python(PyObject*);
}

// Provide specializations of to_python_value
template <class T> struct to_python_value;

namespace detail
{
  // Since there's no registry lookup, always report the existence of
  // a converter.
  struct builtin_to_python
  {
      // This information helps make_getter() decide whether to try to
      // return an internal reference or not. I don't like it much,
      // but it will have to serve for now.
      BOOST_STATIC_CONSTANT(bool, uses_registry = false);
  };
}

// Use expr to create the PyObject corresponding to x
# define BOOST_PYTHON_RETURN_TO_PYTHON_BY_VALUE(T, expr, pytype)\
    template <> struct to_python_value<T&>                      \
        : detail::builtin_to_python                             \
    {                                                           \
        inline PyObject* operator()(T const& x) const           \
        {                                                       \
            return (expr);                                      \
        }                                                       \
        inline PyTypeObject const* get_pytype() const           \
        {                                                       \
            return (pytype);                                    \
        }                                                       \
    };                                                          \
    template <> struct to_python_value<T const&>                \
        : detail::builtin_to_python                             \
    {                                                           \
        inline PyObject* operator()(T const& x) const           \
        {                                                       \
            return (expr);                                      \
        }                                                       \
        inline PyTypeObject const* get_pytype() const           \
        {                                                       \
            return (pytype);                                    \
        }                                                       \
    };

# define BOOST_PYTHON_ARG_TO_PYTHON_BY_VALUE(T, expr)   \
    namespace converter                                 \
    {                                                   \
      template <> struct arg_to_python< T >             \
        : handle<>                                      \
      {                                                 \
          arg_to_python(T const& x)                     \
            : python::handle<>(expr) {}                 \
      };                                                \
    } 

// Specialize argument and return value converters for T using expr
# define BOOST_PYTHON_TO_PYTHON_BY_VALUE(T, expr, pytype)  \
        BOOST_PYTHON_RETURN_TO_PYTHON_BY_VALUE(T,expr, pytype)  \
        BOOST_PYTHON_ARG_TO_PYTHON_BY_VALUE(T,expr)

// Specialize converters for signed and unsigned T to Python Int
#if PY_VERSION_HEX >= 0x03000000

# define BOOST_PYTHON_TO_INT(T)                                         \
    BOOST_PYTHON_TO_PYTHON_BY_VALUE(signed T, ::PyLong_FromLong(x), &PyLong_Type)      \
    BOOST_PYTHON_TO_PYTHON_BY_VALUE(unsigned T, ::PyLong_FromUnsignedLong(x), &PyLong_Type)

#else

# define BOOST_PYTHON_TO_INT(T)                                         \
    BOOST_PYTHON_TO_PYTHON_BY_VALUE(signed T, ::PyInt_FromLong(x), &PyInt_Type)      \
    BOOST_PYTHON_TO_PYTHON_BY_VALUE(                                    \
        unsigned T                                                      \
        , static_cast<unsigned long>(x) > static_cast<unsigned long>(   \
                (std::numeric_limits<long>::max)())                     \
        ? ::PyLong_FromUnsignedLong(x)                                  \
        : ::PyInt_FromLong(x), &PyInt_Type)
#endif

// Bool is not signed.
#if PY_VERSION_HEX >= 0x02030000
BOOST_PYTHON_TO_PYTHON_BY_VALUE(bool, ::PyBool_FromLong(x), &PyBool_Type)
#else
BOOST_PYTHON_TO_PYTHON_BY_VALUE(bool, ::PyInt_FromLong(x), &PyInt_Type)
#endif
  
// note: handles signed char and unsigned char, but not char (see below)
BOOST_PYTHON_TO_INT(char)

BOOST_PYTHON_TO_INT(short)
BOOST_PYTHON_TO_INT(int)
BOOST_PYTHON_TO_INT(long)

# if defined(_MSC_VER) && defined(_WIN64) && PY_VERSION_HEX < 0x03000000
/* Under 64-bit Windows std::size_t is "unsigned long long". To avoid
   getting a Python long for each std::size_t the value is checked before
   the conversion. A std::size_t is converted to a simple Python int
   if possible; a Python long appears only if the value is too small or
   too large to fit into a simple int. */
BOOST_PYTHON_TO_PYTHON_BY_VALUE(
    signed BOOST_PYTHON_LONG_LONG,
    (   x < static_cast<signed BOOST_PYTHON_LONG_LONG>(
            (std::numeric_limits<long>::min)())
     || x > static_cast<signed BOOST_PYTHON_LONG_LONG>(
            (std::numeric_limits<long>::max)()))
    ? ::PyLong_FromLongLong(x)
    : ::PyInt_FromLong(static_cast<long>(x)), &PyInt_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(
    unsigned BOOST_PYTHON_LONG_LONG,
    x > static_cast<unsigned BOOST_PYTHON_LONG_LONG>(
      (std::numeric_limits<long>::max)())
    ? ::PyLong_FromUnsignedLongLong(x)
    : ::PyInt_FromLong(static_cast<long>(x)), &PyInt_Type)
//
# elif defined(HAVE_LONG_LONG) // using Python's macro instead of Boost's
                               // - we don't seem to get the config right
                               // all the time.
BOOST_PYTHON_TO_PYTHON_BY_VALUE(signed BOOST_PYTHON_LONG_LONG, ::PyLong_FromLongLong(x), &PyLong_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(unsigned BOOST_PYTHON_LONG_LONG, ::PyLong_FromUnsignedLongLong(x), &PyLong_Type)
# endif
    
# undef BOOST_TO_PYTHON_INT

#if PY_VERSION_HEX >= 0x03000000
BOOST_PYTHON_TO_PYTHON_BY_VALUE(char, converter::do_return_to_python(x), &PyUnicode_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(char const*, converter::do_return_to_python(x), &PyUnicode_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::string, ::PyUnicode_FromStringAndSize(x.data(),implicit_cast<ssize_t>(x.size())), &PyUnicode_Type)
#else
BOOST_PYTHON_TO_PYTHON_BY_VALUE(char, converter::do_return_to_python(x), &PyString_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(char const*, converter::do_return_to_python(x), &PyString_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::string, ::PyString_FromStringAndSize(x.data(),implicit_cast<ssize_t>(x.size())), &PyString_Type)
#endif

#if defined(Py_USING_UNICODE) && !defined(BOOST_NO_STD_WSTRING)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::wstring, ::PyUnicode_FromWideChar(x.data(),implicit_cast<ssize_t>(x.size())), &PyUnicode_Type)
# endif 
BOOST_PYTHON_TO_PYTHON_BY_VALUE(float, ::PyFloat_FromDouble(x), &PyFloat_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(double, ::PyFloat_FromDouble(x), &PyFloat_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(long double, ::PyFloat_FromDouble(x), &PyFloat_Type)
BOOST_PYTHON_RETURN_TO_PYTHON_BY_VALUE(PyObject*, converter::do_return_to_python(x), 0)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::complex<float>, ::PyComplex_FromDoubles(x.real(), x.imag()), &PyComplex_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::complex<double>, ::PyComplex_FromDoubles(x.real(), x.imag()), &PyComplex_Type)
BOOST_PYTHON_TO_PYTHON_BY_VALUE(std::complex<long double>, ::PyComplex_FromDoubles(x.real(), x.imag()), &PyComplex_Type)

# undef BOOST_PYTHON_RETURN_TO_PYTHON_BY_VALUE
# undef BOOST_PYTHON_ARG_TO_PYTHON_BY_VALUE
# undef BOOST_PYTHON_TO_PYTHON_BY_VALUE
# undef BOOST_PYTHON_TO_INT
    
namespace converter
{ 

  void initialize_builtin_converters();

}

}} // namespace boost::python::converter

#endif // BUILTIN_CONVERTERS_DWA2002124_HPP
