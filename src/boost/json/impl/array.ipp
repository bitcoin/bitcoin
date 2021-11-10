//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_ARRAY_IPP
#define BOOST_JSON_IMPL_ARRAY_IPP

#include <boost/json/array.hpp>
#include <boost/json/pilfer.hpp>
#include <boost/json/detail/except.hpp>
#include <boost/json/detail/hash_combine.hpp>
#include <cstdlib>
#include <limits>
#include <new>
#include <utility>

BOOST_JSON_NS_BEGIN

//----------------------------------------------------------

constexpr array::table::table() = default;

// empty arrays point here
BOOST_JSON_REQUIRE_CONST_INIT
array::table array::empty_;

auto
array::
table::
allocate(
    std::size_t capacity,
    storage_ptr const& sp) ->
        table*
{
    BOOST_ASSERT(capacity > 0);
    if(capacity > array::max_size())
        detail::throw_length_error(
            "array too large",
            BOOST_JSON_SOURCE_POS);
    auto p = reinterpret_cast<
        table*>(sp->allocate(
            sizeof(table) +
                capacity * sizeof(value),
            alignof(value)));
    p->capacity = static_cast<
        std::uint32_t>(capacity);
    return p;
}

void
array::
table::
deallocate(
    table* p,
    storage_ptr const& sp)
{
    if(p->capacity == 0)
        return;
    sp->deallocate(p,
        sizeof(table) +
            p->capacity * sizeof(value),
        alignof(value));
}

//----------------------------------------------------------

array::
revert_insert::
revert_insert(
    const_iterator pos,
    std::size_t n,
    array& arr)
    : arr_(&arr)
    , i_(pos - arr_->data())
    , n_(n)
{
    BOOST_ASSERT(
        pos >= arr_->begin() &&
        pos <= arr_->end());
    if( n_ <= arr_->capacity() -
        arr_->size())
    {
        // fast path
        p = arr_->data() + i_;
        if(n_ == 0)
            return;
        relocate(
            p + n_,
            p,
            arr_->size() - i_);
        arr_->t_->size = static_cast<
            std::uint32_t>(
                arr_->t_->size + n_);
        return;
    }
    if(n_ > max_size() - arr_->size())
        detail::throw_length_error(
            "array too large",
            BOOST_JSON_SOURCE_POS);
    auto t = table::allocate(
        arr_->growth(arr_->size() + n_),
            arr_->sp_);
    t->size = static_cast<std::uint32_t>(
        arr_->size() + n_);
    p = &(*t)[0] + i_;
    relocate(
        &(*t)[0],
        arr_->data(),
        i_);
    relocate(
        &(*t)[i_ + n_],
        arr_->data() + i_,
        arr_->size() - i_);
    t = detail::exchange(arr_->t_, t);
    table::deallocate(t, arr_->sp_);
}

array::
revert_insert::
~revert_insert()
{
    if(! arr_)
        return;
    BOOST_ASSERT(n_ != 0);
    auto const pos =
        arr_->data() + i_;
    arr_->destroy(pos, p);
    arr_->t_->size = static_cast<
        std::uint32_t>(
            arr_->t_->size - n_);
    relocate(
        pos,
        pos + n_,
        arr_->size() - i_);
}

//----------------------------------------------------------

void
array::
destroy(
    value* first, value* last) noexcept
{
    if(sp_.is_not_shared_and_deallocate_is_trivial())
        return;
    while(last-- != first)
        last->~value();
}

void
array::
destroy() noexcept
{
    if(sp_.is_not_shared_and_deallocate_is_trivial())
        return;
    auto last = end();
    auto const first = begin();
    while(last-- != first)
        last->~value();
    table::deallocate(t_, sp_);
}

//----------------------------------------------------------
//
// Special Members
//
//----------------------------------------------------------

array::
array(detail::unchecked_array&& ua)
    : sp_(ua.storage())
{
    BOOST_STATIC_ASSERT(
        alignof(table) == alignof(value));
    if(ua.size() == 0)
    {
        t_ = &empty_;
        return;
    }
    t_= table::allocate(
        ua.size(), sp_);
    t_->size = static_cast<
        std::uint32_t>(ua.size());
    ua.relocate(data());
}

array::
~array()
{
    destroy();
}

array::
array(
    std::size_t count,
    value const& v,
    storage_ptr sp)
    : sp_(std::move(sp))
{
    if(count == 0)
    {
        t_ = &empty_;
        return;
    }
    t_= table::allocate(
        count, sp_);
    t_->size = 0;
    revert_construct r(*this);
    while(count--)
    {
        ::new(end()) value(v, sp_);
        ++t_->size;
    }
    r.commit();
}

array::
array(
    std::size_t count,
    storage_ptr sp)
    : sp_(std::move(sp))
{
    if(count == 0)
    {
        t_ = &empty_;
        return;
    }
    t_ = table::allocate(
        count, sp_);
    t_->size = static_cast<
        std::uint32_t>(count);
    auto p = data();
    do
    {
        ::new(p++) value(sp_);
    }
    while(--count);
}

array::
array(array const& other)
    : array(other, other.sp_)
{
}

array::
array(
    array const& other,
    storage_ptr sp)
    : sp_(std::move(sp))
{
    if(other.empty())
    {
        t_ = &empty_;
        return;
    }
    t_ = table::allocate(
        other.size(), sp_);
    t_->size = 0;
    revert_construct r(*this);
    auto src = other.data();
    auto dest = data();
    auto const n = other.size();
    do
    {
        ::new(dest++) value(
            *src++, sp_);
        ++t_->size;
    }
    while(t_->size < n);
    r.commit();
}

array::
array(
    array&& other,
    storage_ptr sp)
    : sp_(std::move(sp))
{
    if(*sp_ == *other.sp_)
    {
        // same resource
        t_ = detail::exchange(
            other.t_, &empty_);
        return;
    }
    else if(other.empty())
    {
        t_ = &empty_;
        return;
    }
    // copy
    t_ = table::allocate(
        other.size(), sp_);
    t_->size = 0;
    revert_construct r(*this);
    auto src = other.data();
    auto dest = data();
    auto const n = other.size();
    do
    {
        ::new(dest++) value(
            *src++, sp_);
        ++t_->size;
    }
    while(t_->size < n);
    r.commit();
}

array::
array(
    std::initializer_list<
        value_ref> init,
    storage_ptr sp)
    : sp_(std::move(sp))
{
    if(init.size() == 0)
    {
        t_ = &empty_;
        return;
    }
    t_ = table::allocate(
        init.size(), sp_);
    t_->size = 0;
    revert_construct r(*this);
    value_ref::write_array(
        data(), init, sp_);
    t_->size = static_cast<
        std::uint32_t>(init.size());
    r.commit();
}

//----------------------------------------------------------

array&
array::
operator=(array const& other)
{
    array(other,
        storage()).swap(*this);
    return *this;
}

array&
array::
operator=(array&& other)
{
    array(std::move(other),
        storage()).swap(*this);
    return *this;
}

array&
array::
operator=(
    std::initializer_list<value_ref> init)
{
    array(init,
        storage()).swap(*this);
    return *this;
}

//----------------------------------------------------------
//
// Capacity
//
//----------------------------------------------------------

void
array::
shrink_to_fit() noexcept
{
    if(capacity() <= size())
        return;
    if(size() == 0)
    {
        table::deallocate(t_, sp_);
        t_ = &empty_;
        return;
    }

#ifndef BOOST_NO_EXCEPTIONS
    try
    {
#endif
        auto t = table::allocate(
            size(), sp_);
        relocate(
            &(*t)[0],
            data(),
            size());
        t->size = static_cast<
            std::uint32_t>(size());
        t = detail::exchange(
            t_, t);
        table::deallocate(t, sp_);
#ifndef BOOST_NO_EXCEPTIONS
    }
    catch(...)
    {
        // eat the exception
        return;
    }
#endif
}

//----------------------------------------------------------
//
// Modifiers
//
//----------------------------------------------------------

void
array::
clear() noexcept
{
    if(size() == 0)
        return;
    destroy(
        begin(), end());
    t_->size = 0;
}

auto
array::
insert(
    const_iterator pos,
    value const& v) ->
        iterator
{
    return emplace(pos, v);
}

auto
array::
insert(
    const_iterator pos,
    value&& v) ->
        iterator
{
    return emplace(pos, std::move(v));
}

auto
array::
insert(
    const_iterator pos,
    std::size_t count,
    value const& v) ->
        iterator
{
    revert_insert r(
        pos, count, *this);
    while(count--)
    {
        ::new(r.p) value(v, sp_);
        ++r.p;
    }
    return r.commit();
}

auto
array::
insert(
    const_iterator pos,
    std::initializer_list<
        value_ref> init) ->
    iterator
{
    revert_insert r(
        pos, init.size(), *this);
    value_ref::write_array(
        r.p, init, sp_);
    return r.commit();
}

auto
array::
erase(
    const_iterator pos) noexcept ->
    iterator
{
    BOOST_ASSERT(
        pos >= begin() &&
        pos <= end());
    auto const p = &(*t_)[0] +
        (pos - &(*t_)[0]);
    destroy(p, p + 1);
    relocate(p, p + 1, 1);
    --t_->size;
    return p;
}

auto
array::
erase(
    const_iterator first,
    const_iterator last) noexcept ->
        iterator
{
    std::size_t const n =
        last - first;
    auto const p = &(*t_)[0] +
        (first - &(*t_)[0]);
    destroy(p, p + n);
    relocate(p, p + n,
        t_->size - (last -
            &(*t_)[0]));
    t_->size = static_cast<
        std::uint32_t>(t_->size - n);
    return p;
}

void
array::
push_back(value const& v)
{
    emplace_back(v);
}

void
array::
push_back(value&& v)
{
    emplace_back(std::move(v));
}

void
array::
pop_back() noexcept
{
    auto const p = &back();
    destroy(p, p + 1);
    --t_->size;
}

void
array::
resize(std::size_t count)
{
    if(count <= t_->size)
    {
        // shrink
        destroy(
            &(*t_)[0] + count,
            &(*t_)[0] + t_->size);
        t_->size = static_cast<
            std::uint32_t>(count);
        return;
    }

    reserve(count);
    auto p = &(*t_)[t_->size];
    auto const end = &(*t_)[count];
    while(p != end)
        ::new(p++) value(sp_);
    t_->size = static_cast<
        std::uint32_t>(count);
}

void
array::
resize(
    std::size_t count,
    value const& v)
{
    if(count <= size())
    {
        // shrink
        destroy(
            data() + count,
            data() + size());
        t_->size = static_cast<
            std::uint32_t>(count);
        return;
    }
    count -= size();
    revert_insert r(
        end(), count, *this);
    while(count--)
    {
        ::new(r.p) value(v, sp_);
        ++r.p;
    }
    r.commit();
}

void
array::
swap(array& other)
{
    BOOST_ASSERT(this != &other);
    if(*sp_ == *other.sp_)
    {
        t_ = detail::exchange(
            other.t_, t_);
        return;
    }
    array temp1(
        std::move(*this),
        other.storage());
    array temp2(
        std::move(other),
        this->storage());
    this->~array();
    ::new(this) array(
        pilfer(temp2));
    other.~array();
    ::new(&other) array(
        pilfer(temp1));
}

//----------------------------------------------------------
//
// Private
//
//----------------------------------------------------------

std::size_t
array::
growth(
    std::size_t new_size) const
{
    if(new_size > max_size())
        detail::throw_length_error(
            "array too large",
            BOOST_JSON_SOURCE_POS);
    std::size_t const old = capacity();
    if(old > max_size() - old / 2)
        return new_size;
    std::size_t const g =
        old + old / 2; // 1.5x
    if(g < new_size)
        return new_size;
    return g;
}

// precondition: new_capacity > capacity()
void
array::
reserve_impl(
    std::size_t new_capacity)
{
    BOOST_ASSERT(
        new_capacity > t_->capacity);
    auto t = table::allocate(
        growth(new_capacity), sp_);
    relocate(
        &(*t)[0],
        &(*t_)[0],
        t_->size);
    t->size = t_->size;
    t = detail::exchange(t_, t);
    table::deallocate(t, sp_);
}

// precondition: pv is not aliased
value&
array::
push_back(
    pilfered<value> pv)
{
    auto const n = t_->size;
    if(n < t_->capacity)
    {
        // fast path
        auto& v = *::new(
            &(*t_)[n]) value(pv);
        ++t_->size;
        return v;
    }
    auto const t =
        detail::exchange(t_,
            table::allocate(
                growth(n + 1),
                    sp_));
    auto& v = *::new(
        &(*t_)[n]) value(pv);
    relocate(
        &(*t_)[0],
        &(*t)[0],
        n);
    t_->size = n + 1;
    table::deallocate(t, sp_);
    return v;
}

// precondition: pv is not aliased
auto
array::
insert(
    const_iterator pos,
    pilfered<value> pv) ->
        iterator
{
    BOOST_ASSERT(
        pos >= begin() &&
        pos <= end());
    std::size_t const n =
        t_->size;
    std::size_t const i =
        pos - &(*t_)[0];
    if(n < t_->capacity)
    {
        // fast path
        auto const p =
            &(*t_)[i];
        relocate(
            p + 1,
            p,
            n - i);
        ::new(p) value(pv);
        ++t_->size;
        return p;
    }
    auto t =
        table::allocate(
            growth(n + 1), sp_);
    auto const p = &(*t)[i];
    ::new(p) value(pv);
    relocate(
        &(*t)[0],
        &(*t_)[0],
        i);
    relocate(
        p + 1,
        &(*t_)[i],
        n - i);
    t->size = static_cast<
        std::uint32_t>(size() + 1);
    t = detail::exchange(t_, t);
    table::deallocate(t, sp_);
    return p;
}

//----------------------------------------------------------

bool
array::
equal(
    array const& other) const noexcept
{
    if(size() != other.size())
        return false;
    for(std::size_t i = 0; i < size(); ++i)
        if((*this)[i] != other[i])
            return false;
    return true;
}

BOOST_JSON_NS_END

//----------------------------------------------------------
//
// std::hash specialization
//
//----------------------------------------------------------

std::size_t
std::hash<::boost::json::array>::operator()(
    ::boost::json::array const& ja) const noexcept
{
  std::size_t seed = ja.size();
  for (const auto& jv : ja) {
    seed = ::boost::json::detail::hash_combine(
        seed,
        std::hash<::boost::json::value>{}(jv));
  }
  return seed;
}

//----------------------------------------------------------

#endif
