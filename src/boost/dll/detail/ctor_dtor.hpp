// Copyright 2016 Klemens Morgenstern, Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#ifndef BOOST_DLL_DETAIL_CTOR_DTOR_HPP_
#define BOOST_DLL_DETAIL_CTOR_DTOR_HPP_

#include <boost/dll/config.hpp>
#ifdef BOOST_HAS_PRAGMA_ONCE
# pragma once
#endif

#include <boost/dll/detail/aggressive_ptr_cast.hpp>
#include <boost/dll/detail/get_mem_fn_type.hpp>

#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows
#   include <boost/dll/detail/demangling/msvc.hpp>
#else
#   include <boost/dll/detail/demangling/itanium.hpp>
#endif


namespace boost { namespace dll { namespace detail {

/*!
 * This class stores a constructor.
 *
 * In some compilers there are several constructors in code, which may include an allocating one.
 * This can be used if the imported class shall be put on the heap, which is why the class provied both types.
 */
template<typename Signature>
struct constructor;

template<typename Class, typename ...Args>
struct constructor<Class(Args...)> {
    typedef typename detail::get_mem_fn_type<Class, void(Args...)>::mem_fn standard_t;
    typedef Class*(*allocating_t)(Args...);


    //! The standard, i.e. not allocating constructor. @warning May differ with the compiler. Use @ref constructor::call_standard instead.
    standard_t standard;
    //! The allocating constructor.  @warning May differ with the compiler. Use @ref constructor::call_allocating instead.
    allocating_t allocating;

    //! Call the standard constructor
    void call_standard  (Class * const ptr, Args...args){ (ptr->*standard)(static_cast<Args>(args)...); }

    //! Call the deleting destructor
    Class * call_allocating(Args...args){ return allocating(static_cast<Args>(args)...); }


    //! True if a allocating constructor could be loaded.
    bool has_allocating() const { return allocating != nullptr; }

    //! True if a standard constructor could be loaded.
    bool has_standard() const { return standard != nullptr; }

    //! False if neither the allocating nor the standard constructor is available.
    bool is_empty() const { return (allocating == nullptr) && (standard == nullptr) ; }

    constructor() = delete;
    constructor(const constructor &) = default;

    explicit constructor(standard_t standard, allocating_t allocating = nullptr)
        : standard(standard)
        , allocating(allocating)
    {}
};



template <typename Class>
struct destructor {
#if !defined(_WIN32)
    typedef void(*type)(Class* const);
#elif !defined(_WIN64)
    typedef void(__thiscall * type)(Class* const);
#else
    typedef void(__cdecl * type)(Class* const);
#endif

    typedef type standard_t;
    typedef type deleting_t;

    //! The standard, i.e. not deleting destructor. @warning May differ with the compiler. Use @ref destructor::call_standard instead.
    standard_t standard;
    //! The deleting destructor. @warning May differ with the compiler. Use @ref destructor::call_deallocating instead.
    deleting_t deleting;

    //! Call the standard constructor
    void call_standard(Class * const ptr){ standard(ptr); }

    //! Call the deleting destructor
    void call_deleting(Class * const ptr){ deleting(ptr); }

    //! True if a deleting destructor could be loaded.
    bool has_deleting() const { return deleting != nullptr; }

    //! True if a standard destructor could be loaded.
    bool has_standard() const { return standard != nullptr; }

    //! False if neither the deleting nor the standard destructor is available.
    bool is_empty() const { return (deleting == nullptr) && (standard == nullptr) ; }
    destructor() = delete;

    //! Copy destructor.
    destructor(const destructor &) = default;

    //! Construct it from both the standard destructor and the allocating destructor
    explicit destructor(const standard_t &standard, const deleting_t &deleting = nullptr)
        : standard(standard)
        , deleting(deleting)
    {}
};

#if defined(_MSC_VER) // MSVC, Clang-cl, and ICC on Windows

template<typename Signature, typename Lib>
constructor<Signature> load_ctor(Lib & lib, const mangled_storage_impl::ctor_sym & ct) {
    typedef typename constructor<Signature>::standard_t standard_t;
    standard_t ctor = lib.template get<standard_t>(ct);
    return constructor<Signature>(ctor);
}

template<typename Class, typename Lib>
destructor<Class> load_dtor(Lib & lib, const mangled_storage_impl::dtor_sym & dt) {
    typedef typename destructor<Class>::standard_t standard_t;
    //@apolukhin That does NOT work this way with MSVC-14 x32 via memcpy. The x64 is different.
    //standard_t dtor = &lib.template get< typename boost::remove_pointer<standard_t>::type >(dt);
    void * buf = &lib.template get<unsigned char>(dt);
    standard_t dtor;
    std::memcpy(&dtor, &buf, sizeof(dtor));
    return destructor<Class>(dtor);
}

#else

template<typename Signature, typename Lib>
constructor<Signature> load_ctor(Lib & lib, const mangled_storage_impl::ctor_sym & ct) {
    typedef typename constructor<Signature>::standard_t   stand;
    typedef typename constructor<Signature>::allocating_t alloc;

    stand s = nullptr;
    alloc a = nullptr;

    //see here for the abi http://mentorembedded.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor

    if (!ct.C1.empty())
    {
        //the only way this works on mingw/win.
        //For some reason there is always an 0xA in the following poniter, which screws with the this pointer.
        void *buf = &lib.template get<unsigned char>(ct.C1);
        std::memcpy(&s, &buf, sizeof(void*));
    }
    if (!ct.C3.empty())
    {
        void *buf = &lib.template get<unsigned char>(ct.C3);
        std::memcpy(&a, &buf, sizeof(void*));
    }

    return constructor<Signature>(s,a);
}

template<typename Class, typename Lib>
destructor<Class> load_dtor(Lib & lib, const mangled_storage_impl::dtor_sym & dt) {
    typedef typename destructor<Class>::standard_t stand;
    typedef typename destructor<Class>::deleting_t delet;

    stand s = nullptr;
    delet d = nullptr;

    //see here for the abi http://mentorembedded.github.io/cxx-abi/abi.html#mangling-special-ctor-dtor
    if (!dt.D1.empty()) {
        s = &lib.template get< typename boost::remove_pointer<stand>::type >(dt.D1);
    }

    if (!dt.D0.empty()) {
        d = &lib.template get< typename boost::remove_pointer<delet>::type >(dt.D0);
    }

    return destructor<Class>(s,d);

}

#endif

}}} // namespace boost::dll::detail

#endif /* BOOST_DLL_DETAIL_CTOR_DTOR_HPP_ */
