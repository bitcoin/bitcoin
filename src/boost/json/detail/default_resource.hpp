//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DEFAULT_RESOURCE_HPP
#define BOOST_JSON_DEFAULT_RESOURCE_HPP

#include <boost/json/detail/config.hpp>
#include <new>

BOOST_JSON_NS_BEGIN
namespace detail {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // class needs to have dll-interface to be used by clients of class
#pragma warning(disable: 4275) // non dll-interface class used as base for dll-interface class
#endif

// A simple memory resource that uses operator new and delete.
class
    BOOST_SYMBOL_VISIBLE
    BOOST_JSON_CLASS_DECL
    default_resource final
    : public memory_resource
{
    union holder;

#ifndef BOOST_JSON_WEAK_CONSTINIT
# ifndef BOOST_JSON_NO_DESTROY
    static holder instance_;
# else
    BOOST_JSON_NO_DESTROY
    static default_resource instance_;
# endif
#endif

public:
    static
    memory_resource*
    get() noexcept
    {
    #ifdef BOOST_JSON_WEAK_CONSTINIT
        static default_resource instance_;
    #endif
        return reinterpret_cast<memory_resource*>(
            reinterpret_cast<std::uintptr_t*>(
                &instance_));
    }

    ~default_resource();

    void*
    do_allocate(
        std::size_t n,
        std::size_t) override;

    void
    do_deallocate(
        void* p,
        std::size_t,
        std::size_t) override;

    bool
    do_is_equal(
        memory_resource const& mr) const noexcept override;
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

union default_resource::
    holder
{
#ifndef BOOST_JSON_WEAK_CONSTINIT
    constexpr
#endif
    holder()
        : mr()
    {
    }

    ~holder()
    {
    }

    default_resource mr;
};

} // detail
BOOST_JSON_NS_END

#endif
