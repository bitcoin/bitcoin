// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
#ifndef THREAD_HEAP_ALLOC_HPP
#define THREAD_HEAP_ALLOC_HPP
#include <new>
#include <boost/thread/detail/config.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <stdexcept>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <boost/core/no_exceptions_support.hpp>

#include <boost/winapi/heap_memory.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        inline void* allocate_raw_heap_memory(unsigned size)
        {
            void* const heap_memory=winapi::HeapAlloc(winapi::GetProcessHeap(),0,size);
            if(!heap_memory)
            {
                boost::throw_exception(std::bad_alloc());
            }
            return heap_memory;
        }

        inline void free_raw_heap_memory(void* heap_memory)
        {
            BOOST_VERIFY(winapi::HeapFree(winapi::GetProcessHeap(),0,heap_memory)!=0);
        }
#if defined(BOOST_THREAD_PROVIDES_VARIADIC_THREAD) && ! defined (BOOST_NO_CXX11_RVALUE_REFERENCES)
        template<typename T,typename... Args>
        inline T* heap_new(Args&&... args)
        {
          void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
          BOOST_TRY
          {
              T* const data=new (heap_memory) T(static_cast<Args&&>(args)...);
              return data;
          }
          BOOST_CATCH(...)
          {
              free_raw_heap_memory(heap_memory);
              BOOST_RETHROW
          }
          BOOST_CATCH_END
        }
#else
        template<typename T>
        inline T* heap_new()
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T();
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
        template<typename T,typename A1>
        inline T* heap_new(A1&& a1)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(static_cast<A1&&>(a1));
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2>
        inline T* heap_new(A1&& a1,A2&& a2)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(static_cast<A1&&>(a1),static_cast<A2&&>(a2));
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1&& a1,A2&& a2,A3&& a3)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(static_cast<A1&&>(a1),static_cast<A2&&>(a2),
                                                  static_cast<A3&&>(a3));
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1&& a1,A2&& a2,A3&& a3,A4&& a4)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(static_cast<A1&&>(a1),static_cast<A2&&>(a2),
                                                  static_cast<A3&&>(a3),static_cast<A4&&>(a4));
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
#else
        template<typename T,typename A1>
        inline T* heap_new_impl(A1 a1)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }

        template<typename T,typename A1,typename A2>
        inline T* heap_new_impl(A1 a1,A2 a2)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }

        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }

        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4,typename A5>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4,a5);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4,typename A5,typename A6>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4,a5,a6);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4,a5,a6,a7);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4,a5,a6,a7,a8);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4,typename A5,typename A6,typename A7,typename A8,typename A9>
        inline T* heap_new_impl(A1 a1,A2 a2,A3 a3,A4 a4,A5 a5,A6 a6,A7 a7,A8 a8,A9 a9)
        {
            void* const heap_memory=allocate_raw_heap_memory(sizeof(T));
            BOOST_TRY
            {
                T* const data=new (heap_memory) T(a1,a2,a3,a4,a5,a6,a7,a8,a9);
                return data;
            }
            BOOST_CATCH(...)
            {
                free_raw_heap_memory(heap_memory);
                BOOST_RETHROW
            }
            BOOST_CATCH_END
        }


        template<typename T,typename A1>
        inline T* heap_new(A1 const& a1)
        {
            return heap_new_impl<T,A1 const&>(a1);
        }
        template<typename T,typename A1>
        inline T* heap_new(A1& a1)
        {
            return heap_new_impl<T,A1&>(a1);
        }

        template<typename T,typename A1,typename A2>
        inline T* heap_new(A1 const& a1,A2 const& a2)
        {
            return heap_new_impl<T,A1 const&,A2 const&>(a1,a2);
        }
        template<typename T,typename A1,typename A2>
        inline T* heap_new(A1& a1,A2 const& a2)
        {
            return heap_new_impl<T,A1&,A2 const&>(a1,a2);
        }
        template<typename T,typename A1,typename A2>
        inline T* heap_new(A1 const& a1,A2& a2)
        {
            return heap_new_impl<T,A1 const&,A2&>(a1,a2);
        }
        template<typename T,typename A1,typename A2>
        inline T* heap_new(A1& a1,A2& a2)
        {
            return heap_new_impl<T,A1&,A2&>(a1,a2);
        }

        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3 const& a3)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3 const&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1& a1,A2 const& a2,A3 const& a3)
        {
            return heap_new_impl<T,A1&,A2 const&,A3 const&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1 const& a1,A2& a2,A3 const& a3)
        {
            return heap_new_impl<T,A1 const&,A2&,A3 const&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1& a1,A2& a2,A3 const& a3)
        {
            return heap_new_impl<T,A1&,A2&,A3 const&>(a1,a2,a3);
        }

        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3& a3)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1& a1,A2 const& a2,A3& a3)
        {
            return heap_new_impl<T,A1&,A2 const&,A3&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1 const& a1,A2& a2,A3& a3)
        {
            return heap_new_impl<T,A1 const&,A2&,A3&>(a1,a2,a3);
        }
        template<typename T,typename A1,typename A2,typename A3>
        inline T* heap_new(A1& a1,A2& a2,A3& a3)
        {
            return heap_new_impl<T,A1&,A2&,A3&>(a1,a2,a3);
        }

        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3 const& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3 const&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2 const& a2,A3 const& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1&,A2 const&,A3 const&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2& a2,A3 const& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1 const&,A2&,A3 const&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2& a2,A3 const& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1&,A2&,A3 const&,A4 const&>(a1,a2,a3,a4);
        }

        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2 const& a2,A3& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1&,A2 const&,A3&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2& a2,A3& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1 const&,A2&,A3&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2& a2,A3& a3,A4 const& a4)
        {
            return heap_new_impl<T,A1&,A2&,A3&,A4 const&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3 const& a3,A4& a4)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3 const&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2 const& a2,A3 const& a3,A4& a4)
        {
            return heap_new_impl<T,A1&,A2 const&,A3 const&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2& a2,A3 const& a3,A4& a4)
        {
            return heap_new_impl<T,A1 const&,A2&,A3 const&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2& a2,A3 const& a3,A4& a4)
        {
            return heap_new_impl<T,A1&,A2&,A3 const&,A4&>(a1,a2,a3,a4);
        }

        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2 const& a2,A3& a3,A4& a4)
        {
            return heap_new_impl<T,A1 const&,A2 const&,A3&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2 const& a2,A3& a3,A4& a4)
        {
            return heap_new_impl<T,A1&,A2 const&,A3&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1 const& a1,A2& a2,A3& a3,A4& a4)
        {
            return heap_new_impl<T,A1 const&,A2&,A3&,A4&>(a1,a2,a3,a4);
        }
        template<typename T,typename A1,typename A2,typename A3,typename A4>
        inline T* heap_new(A1& a1,A2& a2,A3& a3,A4& a4)
        {
            return heap_new_impl<T,A1&,A2&,A3&,A4&>(a1,a2,a3,a4);
        }

#endif
#endif
        template<typename T>
        inline void heap_delete(T* data)
        {
            data->~T();
            free_raw_heap_memory(data);
        }

        template<typename T>
        struct do_heap_delete
        {
            void operator()(T* data) const
            {
                detail::heap_delete(data);
            }
        };
    }
}

#include <boost/config/abi_suffix.hpp>


#endif
