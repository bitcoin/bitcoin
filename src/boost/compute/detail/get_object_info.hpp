//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_DETAIL_GET_OBJECT_INFO_HPP
#define BOOST_COMPUTE_DETAIL_GET_OBJECT_INFO_HPP

#include <string>
#include <vector>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>

#include <boost/throw_exception.hpp>

#include <boost/compute/cl.hpp>
#include <boost/compute/exception/opencl_error.hpp>

namespace boost {
namespace compute {
namespace detail {

template<class Function, class Object, class AuxInfo>
struct bound_info_function
{
    bound_info_function(Function function, Object object, AuxInfo aux_info)
        : m_function(function),
          m_object(object),
          m_aux_info(aux_info)
    {
    }

    template<class Info>
    cl_int operator()(Info info, size_t input_size, const void *input,
                      size_t size, void *value, size_t *size_ret) const
    {
        return m_function(
            m_object, m_aux_info, info,
            input_size, input, size, value, size_ret
        );
    }

    template<class Info>
    cl_int operator()(Info info, size_t size, void *value, size_t *size_ret) const
    {
        return m_function(m_object, m_aux_info, info, size, value, size_ret);
    }

    Function m_function;
    Object m_object;
    AuxInfo m_aux_info;
};

template<class Function, class Object>
struct bound_info_function<Function, Object, void>
{
    bound_info_function(Function function, Object object)
        : m_function(function),
          m_object(object)
    {
    }

    template<class Info>
    cl_int operator()(Info info, size_t size, void *value, size_t *size_ret) const
    {
        return m_function(m_object, info, size, value, size_ret);
    }

    Function m_function;
    Object m_object;
};

template<class Function, class Object>
inline bound_info_function<Function, Object, void>
bind_info_function(Function f, Object o)
{
    return bound_info_function<Function, Object, void>(f, o);
}

template<class Function, class Object, class AuxInfo>
inline bound_info_function<Function, Object, AuxInfo>
bind_info_function(Function f, Object o, AuxInfo j)
{
    return bound_info_function<Function, Object, AuxInfo>(f, o, j);
}

// default implementation
template<class T>
struct get_object_info_impl
{
    template<class Function, class Info>
    T operator()(Function function, Info info) const
    {
        T value;

        cl_int ret = function(info, sizeof(T), &value, 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return value;
    }

    template<class Function, class Info>
    T operator()(Function function, Info info,
                 const size_t input_size, const void* input) const
    {
        T value;

        cl_int ret = function(info, input_size, input, sizeof(T), &value, 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return value;
    }
};

// specialization for bool
template<>
struct get_object_info_impl<bool>
{
    template<class Function, class Info>
    bool operator()(Function function, Info info) const
    {
        cl_bool value;

        cl_int ret = function(info, sizeof(cl_bool), &value, 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return value == CL_TRUE;
    }
};

// specialization for std::string
template<>
struct get_object_info_impl<std::string>
{
    template<class Function, class Info>
    std::string operator()(Function function, Info info) const
    {
        size_t size = 0;

        cl_int ret = function(info, 0, 0, &size);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        if(size == 0){
            return std::string();
        }

        std::string value(size - 1, 0);

        ret = function(info, size, &value[0], 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return value;
    }
};

// specialization for std::vector<T>
template<class T>
struct get_object_info_impl<std::vector<T> >
{
    template<class Function, class Info>
    std::vector<T> operator()(Function function, Info info) const
    {
        size_t size = 0;

        cl_int ret = function(info, 0, 0, &size);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        if(size == 0) return std::vector<T>();

        std::vector<T> vector(size / sizeof(T));
        ret = function(info, size, &vector[0], 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return vector;
    }

    template<class Function, class Info>
    std::vector<T> operator()(Function function, Info info,
                              const size_t input_size, const void* input) const
    {
        #ifdef BOOST_COMPUTE_CL_VERSION_2_1
        // For CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT in clGetKernelSubGroupInfo
        // we can't get param_value_size using param_value_size_ret
        if(info == CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT)
        {
            std::vector<T> vector(3);
            cl_int ret = function(
                info, input_size, input,
                sizeof(T) * vector.size(), &vector[0], 0
            );
            if(ret != CL_SUCCESS){
                BOOST_THROW_EXCEPTION(opencl_error(ret));
            }
            return vector;
        }
        #endif
        size_t size = 0;

        cl_int ret = function(info, input_size, input, 0, 0, &size);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        std::vector<T> vector(size / sizeof(T));
        ret = function(info, input_size, input, size, &vector[0], 0);
        if(ret != CL_SUCCESS){
            BOOST_THROW_EXCEPTION(opencl_error(ret));
        }

        return vector;
    }
};

// returns the value (of type T) from the given clGet*Info() function call.
template<class T, class Function, class Object, class Info>
inline T get_object_info(Function f, Object o, Info i)
{
    return get_object_info_impl<T>()(bind_info_function(f, o), i);
}

template<class T, class Function, class Object, class Info, class AuxInfo>
inline T get_object_info(Function f, Object o, Info i, AuxInfo j)
{
    return get_object_info_impl<T>()(bind_info_function(f, o, j), i);
}

template<class T, class Function, class Object, class Info, class AuxInfo>
inline T get_object_info(Function f, Object o, Info i, AuxInfo j, const size_t k, const void * l)
{
    return get_object_info_impl<T>()(bind_info_function(f, o, j), i, k, l);
}

// returns the value type for the clGet*Info() call on Object with Enum.
template<class Object, int Enum>
struct get_object_info_type;

// defines the object::get_info<Enum>() specialization
#define BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATION(object_type, result_type, value) \
    namespace detail { \
        template<> struct get_object_info_type<object_type, value> { typedef result_type type; }; \
    } \
    template<> inline result_type object_type::get_info<value>() const \
    { \
        return get_info<result_type>(value); \
    }

// used by BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS()
#define BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_IMPL(r, data, elem) \
    BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATION( \
        data, BOOST_PP_TUPLE_ELEM(2, 0, elem), BOOST_PP_TUPLE_ELEM(2, 1, elem) \
    )

// defines the object::get_info<Enum>() specialization for each
// (result_type, value) tuple in seq for object_type.
#define BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_SPECIALIZATIONS(object_type, seq) \
    BOOST_PP_SEQ_FOR_EACH( \
        BOOST_COMPUTE_DETAIL_DEFINE_GET_INFO_IMPL, object_type, seq \
    )

} // end detail namespace
} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_DETAIL_GET_OBJECT_INFO_HPP
