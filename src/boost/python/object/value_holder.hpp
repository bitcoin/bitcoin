#if !defined(BOOST_PP_IS_ITERATING)

// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef VALUE_HOLDER_DWA20011215_HPP
#  define VALUE_HOLDER_DWA20011215_HPP 

#  include <boost/python/object/value_holder_fwd.hpp>

#  include <boost/python/instance_holder.hpp>
#  include <boost/python/type_id.hpp>
#  include <boost/python/wrapper.hpp>

#  include <boost/python/object/inheritance_query.hpp>
#  include <boost/python/object/forward.hpp>

#  include <boost/python/detail/force_instantiate.hpp>
#  include <boost/python/detail/preprocessor.hpp>

#  include <boost/preprocessor/comma_if.hpp>
#  include <boost/preprocessor/enum_params.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/repeat.hpp>
#  include <boost/preprocessor/debug/line.hpp>

#  include <boost/preprocessor/repetition/enum_params.hpp>
#  include <boost/preprocessor/repetition/enum_binary_params.hpp>

#  include <boost/utility/addressof.hpp>

namespace boost { namespace python { namespace objects { 

#define BOOST_PYTHON_UNFORWARD_LOCAL(z, n, _) BOOST_PP_COMMA_IF(n) objects::do_unforward(a##n,0)

template <class Value>
struct value_holder : instance_holder
{
    typedef Value held_type;
    typedef Value value_type;

    // Forward construction to the held object
#  define BOOST_PP_ITERATION_PARAMS_1 (4, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/object/value_holder.hpp>, 1))
#  include BOOST_PP_ITERATE()

 private: // required holder implementation
    void* holds(type_info, bool null_ptr_only);
    
    template <class T>
    inline void* holds_wrapped(type_info dst_t, wrapper<T>*,T* p)
    {
        return python::type_id<T>() == dst_t ? p : 0;
    }
    
    inline void* holds_wrapped(type_info, ...)
    {
        return 0;
    }
 private: // data members
    Value m_held;
};

template <class Value, class Held>
struct value_holder_back_reference : instance_holder
{
    typedef Held held_type;
    typedef Value value_type;
    
    // Forward construction to the held object
#  define BOOST_PP_ITERATION_PARAMS_1 (4, (0, BOOST_PYTHON_MAX_ARITY, <boost/python/object/value_holder.hpp>, 2))
#  include BOOST_PP_ITERATE()

private: // required holder implementation
    void* holds(type_info, bool null_ptr_only);

 private: // data members
    Held m_held;
};

#  undef BOOST_PYTHON_UNFORWARD_LOCAL

template <class Value>
void* value_holder<Value>::holds(type_info dst_t, bool /*null_ptr_only*/)
{
    if (void* wrapped = holds_wrapped(dst_t, boost::addressof(m_held), boost::addressof(m_held)))
        return wrapped;
    
    type_info src_t = python::type_id<Value>();
    return src_t == dst_t ? boost::addressof(m_held)
        : find_static_type(boost::addressof(m_held), src_t, dst_t);
}

template <class Value, class Held>
void* value_holder_back_reference<Value,Held>::holds(
    type_info dst_t, bool /*null_ptr_only*/)
{
    type_info src_t = python::type_id<Value>();
    Value* x = &m_held;
    
    if (dst_t == src_t)
        return x;
    else if (dst_t == python::type_id<Held>())
        return &m_held;
    else
        return find_static_type(x, src_t, dst_t);
}

}}} // namespace boost::python::objects

# endif // VALUE_HOLDER_DWA20011215_HPP

// --------------- value_holder ---------------

// For gcc 4.4 compatability, we must include the
// BOOST_PP_ITERATION_DEPTH test inside an #else clause.
#else // BOOST_PP_IS_ITERATING
#if BOOST_PP_ITERATION_DEPTH() == 1 && BOOST_PP_ITERATION_FLAGS() == 1
# if !(BOOST_WORKAROUND(__MWERKS__, > 0x3100)                      \
        && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3201)))
#  line BOOST_PP_LINE(__LINE__, value_holder.hpp(value_holder))
# endif

# define N BOOST_PP_ITERATION()

# if (N != 0)
    template <BOOST_PP_ENUM_PARAMS_Z(1, N, class A)>
# endif
    value_holder(
      PyObject* self BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, a))
        : m_held(
            BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_UNFORWARD_LOCAL, nil)
            )
    {
        python::detail::initialize_wrapper(self, boost::addressof(this->m_held));
    }

# undef N

// --------------- value_holder_back_reference ---------------

#elif BOOST_PP_ITERATION_DEPTH() == 1 && BOOST_PP_ITERATION_FLAGS() == 2
# if !(BOOST_WORKAROUND(__MWERKS__, > 0x3100)                      \
        && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3201)))
#  line BOOST_PP_LINE(__LINE__, value_holder.hpp(value_holder_back_reference))
# endif

# define N BOOST_PP_ITERATION()

# if (N != 0)
    template <BOOST_PP_ENUM_PARAMS_Z(1, N, class A)>
# endif
    value_holder_back_reference(
        PyObject* p BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, a))
        : m_held(
            p BOOST_PP_COMMA_IF(N)
            BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_UNFORWARD_LOCAL, nil)
            )
    {
    }

# undef N

#endif // BOOST_PP_ITERATION_DEPTH()
#endif
