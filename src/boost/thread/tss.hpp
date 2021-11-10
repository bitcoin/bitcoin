#ifndef BOOST_THREAD_TSS_HPP
#define BOOST_THREAD_TSS_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams

#include <boost/thread/detail/config.hpp>

#include <boost/type_traits/add_reference.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        namespace thread
        {
            typedef void(*cleanup_func_t)(void*);
            typedef void(*cleanup_caller_t)(cleanup_func_t, void*);
        }

        BOOST_THREAD_DECL void set_tss_data(void const* key,detail::thread::cleanup_caller_t caller,detail::thread::cleanup_func_t func,void* tss_data,bool cleanup_existing);
        BOOST_THREAD_DECL void* get_tss_data(void const* key);
    }

    template <typename T>
    class thread_specific_ptr
    {
    private:
        thread_specific_ptr(thread_specific_ptr&);
        thread_specific_ptr& operator=(thread_specific_ptr&);

        typedef void(*original_cleanup_func_t)(T*);

        static void default_deleter(T* data)
        {
            delete data;
        }

        static void cleanup_caller(detail::thread::cleanup_func_t cleanup_function,void* data)
        {
            reinterpret_cast<original_cleanup_func_t>(cleanup_function)(static_cast<T*>(data));
        }


        detail::thread::cleanup_func_t cleanup;

    public:
        typedef T element_type;

        thread_specific_ptr():
            cleanup(reinterpret_cast<detail::thread::cleanup_func_t>(&default_deleter))
        {}
        explicit thread_specific_ptr(void (*func_)(T*))
          : cleanup(reinterpret_cast<detail::thread::cleanup_func_t>(func_))
        {}
        ~thread_specific_ptr()
        {
            detail::set_tss_data(this,0,0,0,true);
        }

        T* get() const
        {
            return static_cast<T*>(detail::get_tss_data(this));
        }
        T* operator->() const
        {
            return get();
        }
        typename add_reference<T>::type operator*() const
        {
            return *get();
        }
        T* release()
        {
            T* const temp=get();
            detail::set_tss_data(this,0,0,0,false);
            return temp;
        }
        void reset(T* new_value=0)
        {
            T* const current_value=get();
            if(current_value!=new_value)
            {
                detail::set_tss_data(this,&cleanup_caller,cleanup,new_value,true);
            }
        }
    };
}

#include <boost/config/abi_suffix.hpp>

#endif
