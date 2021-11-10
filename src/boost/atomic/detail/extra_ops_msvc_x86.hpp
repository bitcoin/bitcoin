/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2017 Andrey Semashev
 */
/*!
 * \file   atomic/detail/extra_ops_msvc_x86.hpp
 *
 * This header contains implementation of the extra atomic operations for x86.
 */

#ifndef BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_X86_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_X86_HPP_INCLUDED_

#include <cstddef>
#include <boost/memory_order.hpp>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/interlocked.hpp>
#include <boost/atomic/detail/storage_traits.hpp>
#include <boost/atomic/detail/extra_operations_fwd.hpp>
#include <boost/atomic/detail/extra_ops_generic.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

#if defined(_M_IX86)

template< typename Base, bool Signed >
struct extra_operations< Base, 1u, Signed, true > :
    public extra_operations_generic< Base, 1u, Signed >
{
    typedef extra_operations_generic< Base, 1u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            mov old_val, al
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            mov new_val, dl
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool negate_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            test dl, dl
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, byte ptr [edi]
            align 16
        again:
            mov dl, al
            and dl, cl
            lock cmpxchg byte ptr [edi], dl
            jne again
            mov v, dl
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, byte ptr [edi]
            align 16
        again:
            mov dl, al
            or dl, cl
            lock cmpxchg byte ptr [edi], dl
            jne again
            mov v, dl
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, byte ptr [edi]
            align 16
        again:
            mov dl, al
            xor dl, cl
            lock cmpxchg byte ptr [edi], dl
            jne again
            mov v, dl
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            mov old_val, al
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            mov new_val, dl
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool complement_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
            test dl, dl
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            movzx eax, byte ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dl
            lock cmpxchg byte ptr [ecx], dl
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock add byte ptr [edx], al
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock sub byte ptr [edx], al
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock neg byte ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock and byte ptr [edx], al
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock or byte ptr [edx], al
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock xor byte ptr [edx], al
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock not byte ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE bool add_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock add byte ptr [edx], al
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool sub_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock sub byte ptr [edx], al
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool and_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock and byte ptr [edx], al
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool or_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock or byte ptr [edx], al
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool xor_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock xor byte ptr [edx], al
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }
};

template< typename Base, bool Signed >
struct extra_operations< Base, 2u, Signed, true > :
    public extra_operations_generic< Base, 2u, Signed >
{
    typedef extra_operations_generic< Base, 2u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            mov old_val, ax
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            mov new_val, dx
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool negate_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            test dx, dx
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg dx
            lock cmpxchg word ptr [ecx], dx
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, word ptr [edi]
            align 16
        again:
            mov dx, ax
            and dx, cx
            lock cmpxchg word ptr [edi], dx
            jne again
            mov v, dx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, word ptr [edi]
            align 16
        again:
            mov dx, ax
            or dx, cx
            lock cmpxchg word ptr [edi], dx
            jne again
            mov v, dx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            movzx ecx, v
            xor edx, edx
            movzx eax, word ptr [edi]
            align 16
        again:
            mov dx, ax
            xor dx, cx
            lock cmpxchg word ptr [edi], dx
            jne again
            mov v, dx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            mov old_val, ax
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            mov new_val, dx
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool complement_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dx
            lock cmpxchg word ptr [ecx], dx
            jne again
            test dx, dx
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            movzx eax, word ptr [ecx]
            align 16
        again:
            mov edx, eax
            not dx
            lock cmpxchg word ptr [ecx], dx
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock add word ptr [edx], ax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock sub word ptr [edx], ax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock neg word ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock and word ptr [edx], ax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock or word ptr [edx], ax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock xor word ptr [edx], ax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock not word ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE bool add_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock add word ptr [edx], ax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool sub_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock sub word ptr [edx], ax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool and_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock and word ptr [edx], ax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool or_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock or word ptr [edx], ax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool xor_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            movzx eax, v
            lock xor word ptr [edx], ax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock bts word ptr [edx], ax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock btr word ptr [edx], ax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool bit_test_and_complement(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock btc word ptr [edx], ax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }
};

#endif // defined(_M_IX86)

#if defined(_M_IX86) || (defined(BOOST_ATOMIC_INTERLOCKED_BTS) && defined(BOOST_ATOMIC_INTERLOCKED_BTR))

template< typename Base, bool Signed >
struct extra_operations< Base, 4u, Signed, true > :
    public extra_operations_generic< Base, 4u, Signed >
{
    typedef extra_operations_generic< Base, 4u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

#if defined(_M_IX86)
    static BOOST_FORCEINLINE storage_type fetch_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            mov old_val, eax
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            mov new_val, edx
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool negate_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            test edx, edx
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            neg edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE storage_type bitwise_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            mov ecx, v
            xor edx, edx
            mov eax, dword ptr [edi]
            align 16
        again:
            mov edx, eax
            and edx, ecx
            lock cmpxchg dword ptr [edi], edx
            jne again
            mov v, edx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            mov ecx, v
            xor edx, edx
            mov eax, dword ptr [edi]
            align 16
        again:
            mov edx, eax
            or edx, ecx
            lock cmpxchg dword ptr [edi], edx
            jne again
            mov v, edx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type bitwise_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edi, storage
            mov ecx, v
            xor edx, edx
            mov eax, dword ptr [edi]
            align 16
        again:
            mov edx, eax
            xor edx, ecx
            lock cmpxchg dword ptr [edi], edx
            jne again
            mov v, edx
        };
        base_type::fence_after(order);
        return v;
    }

    static BOOST_FORCEINLINE storage_type fetch_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type old_val;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            not edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            mov old_val, eax
        };
        base_type::fence_after(order);
        return old_val;
    }

    static BOOST_FORCEINLINE storage_type bitwise_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        storage_type new_val;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            not edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            mov new_val, edx
        };
        base_type::fence_after(order);
        return new_val;
    }

    static BOOST_FORCEINLINE bool complement_and_test(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            not edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
            test edx, edx
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov ecx, storage
            mov eax, dword ptr [ecx]
            align 16
        again:
            mov edx, eax
            not edx
            lock cmpxchg dword ptr [ecx], edx
            jne again
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_add(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            mov eax, v
            lock add dword ptr [edx], eax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_sub(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            mov eax, v
            lock sub dword ptr [edx], eax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_negate(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock neg dword ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_and(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            mov eax, v
            lock and dword ptr [edx], eax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_or(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            mov eax, v
            lock or dword ptr [edx], eax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_xor(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            mov eax, v
            lock xor dword ptr [edx], eax
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE void opaque_complement(storage_type volatile& storage, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        __asm
        {
            mov edx, storage
            lock not dword ptr [edx]
        };
        base_type::fence_after(order);
    }

    static BOOST_FORCEINLINE bool add_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, v
            lock add dword ptr [edx], eax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool sub_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, v
            lock sub dword ptr [edx], eax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool and_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, v
            lock and dword ptr [edx], eax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool or_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, v
            lock or dword ptr [edx], eax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool xor_and_test(storage_type volatile& storage, storage_type v, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, v
            lock xor dword ptr [edx], eax
            setnz result
        };
        base_type::fence_after(order);
        return result;
    }

    static BOOST_FORCEINLINE bool bit_test_and_complement(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock btc dword ptr [edx], eax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }
#endif // defined(_M_IX86)

#if defined(BOOST_ATOMIC_INTERLOCKED_BTS)
    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order) BOOST_NOEXCEPT
    {
        return !!BOOST_ATOMIC_INTERLOCKED_BTS(&storage, bit_number);
    }
#elif defined(_M_IX86)
    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock bts dword ptr [edx], eax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }
#endif

#if defined(BOOST_ATOMIC_INTERLOCKED_BTR)
    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order) BOOST_NOEXCEPT
    {
        return !!BOOST_ATOMIC_INTERLOCKED_BTR(&storage, bit_number);
    }
#elif defined(_M_IX86)
    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        base_type::fence_before(order);
        bool result;
        __asm
        {
            mov edx, storage
            mov eax, bit_number
            lock btr dword ptr [edx], eax
            setc result
        };
        base_type::fence_after(order);
        return result;
    }
#endif
};

#endif // defined(_M_IX86) || (defined(BOOST_ATOMIC_INTERLOCKED_BTS) && defined(BOOST_ATOMIC_INTERLOCKED_BTR))

#if defined(BOOST_ATOMIC_INTERLOCKED_BTS64) && defined(BOOST_ATOMIC_INTERLOCKED_BTR64)

template< typename Base, bool Signed >
struct extra_operations< Base, 8u, Signed, true > :
    public extra_operations_generic< Base, 8u, Signed >
{
    typedef extra_operations_generic< Base, 8u, Signed > base_type;
    typedef typename base_type::storage_type storage_type;

    static BOOST_FORCEINLINE bool bit_test_and_set(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        return !!BOOST_ATOMIC_INTERLOCKED_BTS64(&storage, bit_number);
    }

    static BOOST_FORCEINLINE bool bit_test_and_reset(storage_type volatile& storage, unsigned int bit_number, memory_order order) BOOST_NOEXCEPT
    {
        return !!BOOST_ATOMIC_INTERLOCKED_BTR64(&storage, bit_number);
    }
};

#endif // defined(BOOST_ATOMIC_INTERLOCKED_BTS64) && defined(BOOST_ATOMIC_INTERLOCKED_BTR64)

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_EXTRA_OPS_MSVC_X86_HPP_INCLUDED_
