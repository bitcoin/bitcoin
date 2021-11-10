//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_ARRAY_HPP
#define BOOST_JSON_IMPL_ARRAY_HPP

#include <boost/json/value.hpp>
#include <boost/json/detail/except.hpp>
#include <algorithm>
#include <stdexcept>
#include <type_traits>

BOOST_JSON_NS_BEGIN

//----------------------------------------------------------

struct alignas(value)
    array::table
{
    std::uint32_t size = 0;
    std::uint32_t capacity = 0;

    constexpr table();

    value&
    operator[](std::size_t pos) noexcept
    {
        return (reinterpret_cast<
            value*>(this + 1))[pos];
    }

    BOOST_JSON_DECL
    static
    table*
    allocate(
        std::size_t capacity,
        storage_ptr const& sp);

    BOOST_JSON_DECL
    static
    void
    deallocate(
        table* p,
        storage_ptr const& sp);
};

//----------------------------------------------------------

class array::revert_construct
{
    array* arr_;

public:
    explicit
    revert_construct(
        array& arr) noexcept
        : arr_(&arr)
    {
    }

    ~revert_construct()
    {
        if(! arr_)
            return;
        arr_->destroy();
    }

    void
    commit() noexcept
    {
        arr_ = nullptr;
    }
};

//----------------------------------------------------------

class array::revert_insert
{
    array* arr_;
    std::size_t const i_;
    std::size_t const n_;

public:
    value* p;

    BOOST_JSON_DECL
    revert_insert(
        const_iterator pos,
        std::size_t n,
        array& arr);

    BOOST_JSON_DECL
    ~revert_insert();

    value*
    commit() noexcept
    {
        auto it =
            arr_->data() + i_;
        arr_ = nullptr;
        return it;
    }
};

//----------------------------------------------------------

void
array::
relocate(
    value* dest,
    value* src,
    std::size_t n) noexcept
{
    if(n == 0)
        return;
    std::memmove(
        static_cast<void*>(dest),
        static_cast<void const*>(src),
        n * sizeof(value));
}

//----------------------------------------------------------
//
// Construction
//
//----------------------------------------------------------

template<class InputIt, class>
array::
array(
    InputIt first, InputIt last,
    storage_ptr sp)
    : array(
        first, last,
        std::move(sp),
        iter_cat<InputIt>{})
{
    BOOST_STATIC_ASSERT(
        std::is_constructible<value,
            decltype(*first)>::value);
}

//----------------------------------------------------------
//
// Modifiers
//
//----------------------------------------------------------

template<class InputIt, class>
auto
array::
insert(
    const_iterator pos,
    InputIt first, InputIt last) ->
        iterator
{
    BOOST_STATIC_ASSERT(
        std::is_constructible<value,
            decltype(*first)>::value);
    return insert(pos, first, last,
        iter_cat<InputIt>{});
}

template<class Arg>
auto
array::
emplace(
    const_iterator pos,
    Arg&& arg) ->
        iterator
{
    BOOST_ASSERT(
        pos >= begin() &&
        pos <= end());
    value jv(
        std::forward<Arg>(arg),
        storage());
    return insert(pos, pilfer(jv));
}

template<class Arg>
value&
array::
emplace_back(Arg&& arg)
{
    value jv(
        std::forward<Arg>(arg),
        storage());
    return push_back(pilfer(jv));
}

//----------------------------------------------------------
//
// Element access
//
//----------------------------------------------------------

value&
array::
at(std::size_t pos)
{
    if(pos >= t_->size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    return (*t_)[pos];
}

value const&
array::
at(std::size_t pos) const
{
    if(pos >= t_->size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    return (*t_)[pos];
}

value&
array::
operator[](std::size_t pos) noexcept
{
    BOOST_ASSERT(pos < t_->size);
    return (*t_)[pos];
}

value const&
array::
operator[](std::size_t pos) const noexcept
{
    BOOST_ASSERT(pos < t_->size);
    return (*t_)[pos];
}

value&
array::
front() noexcept
{
    BOOST_ASSERT(t_->size > 0);
    return (*t_)[0];
}

value const&
array::
front() const noexcept
{
    BOOST_ASSERT(t_->size > 0);
    return (*t_)[0];
}

value&
array::
back() noexcept
{
    BOOST_ASSERT(
        t_->size > 0);
    return (*t_)[t_->size - 1];
}

value const&
array::
back() const noexcept
{
    BOOST_ASSERT(
        t_->size > 0);
    return (*t_)[t_->size - 1];
}

value*
array::
data() noexcept
{
    return &(*t_)[0];
}

value const*
array::
data() const noexcept
{
    return &(*t_)[0];
}

value const*
array::
if_contains(
    std::size_t pos) const noexcept
{
    if( pos < t_->size )
        return &(*t_)[pos];
    return nullptr;
}

value*
array::
if_contains(
    std::size_t pos) noexcept
{
    if( pos < t_->size )
        return &(*t_)[pos];
    return nullptr;
}

//----------------------------------------------------------
//
// Iterators
//
//----------------------------------------------------------

auto
array::
begin() noexcept ->
    iterator
{
    return &(*t_)[0];
}

auto
array::
begin() const noexcept ->
    const_iterator
{
    return &(*t_)[0];
}

auto
array::
cbegin() const noexcept ->
    const_iterator
{
    return &(*t_)[0];
}

auto
array::
end() noexcept ->
    iterator
{
    return &(*t_)[t_->size];
}

auto
array::
end() const noexcept ->
    const_iterator
{
    return &(*t_)[t_->size];
}

auto
array::
cend() const noexcept ->
    const_iterator
{
    return &(*t_)[t_->size];
}

auto
array::
rbegin() noexcept ->
    reverse_iterator
{
    return reverse_iterator(end());
}

auto
array::
rbegin() const noexcept ->
    const_reverse_iterator
{
    return const_reverse_iterator(end());
}

auto
array::
crbegin() const noexcept ->
    const_reverse_iterator
{
    return const_reverse_iterator(end());
}

auto
array::
rend() noexcept ->
    reverse_iterator
{
    return reverse_iterator(begin());
}

auto
array::
rend() const noexcept ->
    const_reverse_iterator
{
    return const_reverse_iterator(begin());
}

auto
array::
crend() const noexcept ->
    const_reverse_iterator
{
    return const_reverse_iterator(begin());
}

//----------------------------------------------------------
//
// Capacity
//
//----------------------------------------------------------

std::size_t
array::
size() const noexcept
{
    return t_->size;
}

constexpr
std::size_t
array::
max_size() noexcept
{
    // max_size depends on the address model
    using min = std::integral_constant<std::size_t,
        (std::size_t(-1) - sizeof(table)) / sizeof(value)>;
    return min::value < BOOST_JSON_MAX_STRUCTURED_SIZE ?
        min::value : BOOST_JSON_MAX_STRUCTURED_SIZE;
}

std::size_t
array::
capacity() const noexcept
{
    return t_->capacity;
}

bool
array::
empty() const noexcept
{
    return t_->size == 0;
}

void
array::
reserve(
    std::size_t new_capacity)
{
    // never shrink
    if(new_capacity <= t_->capacity)
        return;
    reserve_impl(new_capacity);
}

//----------------------------------------------------------
//
// private
//
//----------------------------------------------------------

template<class InputIt>
array::
array(
    InputIt first, InputIt last,
    storage_ptr sp,
    std::input_iterator_tag)
    : sp_(std::move(sp))
    , t_(&empty_)
{
    revert_construct r(*this);
    while(first != last)
    {
        reserve(size() + 1);
        ::new(end()) value(
            *first++, sp_);
        ++t_->size;
    }
    r.commit();
}

template<class InputIt>
array::
array(
    InputIt first, InputIt last,
    storage_ptr sp,
    std::forward_iterator_tag)
    : sp_(std::move(sp))
{
    std::size_t n =
        std::distance(first, last);
    if( n == 0 )
    {
        t_ = &empty_;
        return;
    }

    t_ = table::allocate(n, sp_);
    t_->size = 0;
    revert_construct r(*this);
    while(n--)
    {
        ::new(end()) value(
            *first++, sp_);
        ++t_->size;
    }
    r.commit();
}

template<class InputIt>
auto
array::
insert(
    const_iterator pos,
    InputIt first, InputIt last,
    std::input_iterator_tag) ->
        iterator
{
    BOOST_ASSERT(
        pos >= begin() && pos <= end());
    if(first == last)
        return data() + (pos - data());
    array temp(first, last, sp_);
    revert_insert r(
        pos, temp.size(), *this);
    relocate(
        r.p,
        temp.data(),
        temp.size());
    temp.t_->size = 0;
    return r.commit();
}

template<class InputIt>
auto
array::
insert(
    const_iterator pos,
    InputIt first, InputIt last,
    std::forward_iterator_tag) ->
        iterator
{
    std::size_t n =
        std::distance(first, last);
    revert_insert r(pos, n, *this);
    while(n--)
    {
        ::new(r.p) value(*first++);
        ++r.p;
    }
    return r.commit();
}

BOOST_JSON_NS_END

#endif
