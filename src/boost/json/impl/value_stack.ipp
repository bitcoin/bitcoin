//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_VALUE_STACK_IPP
#define BOOST_JSON_IMPL_VALUE_STACK_IPP

#include <boost/json/value_stack.hpp>
#include <cstring>
#include <stdexcept>
#include <utility>

BOOST_JSON_NS_BEGIN

//--------------------------------------

value_stack::
stack::
~stack()
{
    clear();
    if( begin_ != temp_ &&
        begin_ != nullptr)
        sp_->deallocate(
            begin_,
            (end_ - begin_) *
                sizeof(value));
}

value_stack::
stack::
stack(
    storage_ptr sp,
    void* temp,
    std::size_t size) noexcept
    : sp_(std::move(sp))
    , temp_(temp)
{
    if(size >= min_size_ *
        sizeof(value))
    {
        begin_ = reinterpret_cast<
            value*>(temp);
        top_ = begin_;
        end_ = begin_ +
            size / sizeof(value);
    }
    else
    {
        begin_ = nullptr;
        top_ = nullptr;
        end_ = nullptr;
    }
}

void
value_stack::
stack::
run_dtors(bool b) noexcept
{
    run_dtors_ = b;
}

std::size_t
value_stack::
stack::
size() const noexcept
{
    return top_ - begin_;
}

bool
value_stack::
stack::
has_chars()
{
    return chars_ != 0;
}

//--------------------------------------

// destroy the values but
// not the stack allocation.
void
value_stack::
stack::
clear() noexcept
{
    if(top_ != begin_)
    {
        if(run_dtors_)
            for(auto it = top_;
                it-- != begin_;)
                it->~value();
        top_ = begin_;
    }
    chars_ = 0;
}

void
value_stack::
stack::
maybe_grow()
{
    if(top_ >= end_)
        grow_one();
}

// make room for at least one more value
void
value_stack::
stack::
grow_one()
{
    BOOST_ASSERT(chars_ == 0);
    std::size_t const capacity =
        end_ - begin_;
    std::size_t new_cap = min_size_;
    // VFALCO check overflow here
    while(new_cap < capacity + 1)
        new_cap <<= 1;
    auto const begin =
        reinterpret_cast<value*>(
            sp_->allocate(
                new_cap * sizeof(value)));
    if(begin_)
    {
        std::memcpy(
            reinterpret_cast<char*>(begin),
            reinterpret_cast<char*>(begin_),
            size() * sizeof(value));
        if(begin_ != temp_)
            sp_->deallocate(begin_,
                capacity * sizeof(value));
    }
    // book-keeping
    top_ = begin + (top_ - begin_);
    end_ = begin + new_cap;
    begin_ = begin;
}

// make room for nchars additional characters.
void
value_stack::
stack::
grow(std::size_t nchars)
{
    // needed capacity in values
    std::size_t const needed =
        size() +
        1 +
        ((chars_ + nchars +
            sizeof(value) - 1) /
                sizeof(value));
    std::size_t const capacity =
        end_ - begin_;
    BOOST_ASSERT(
        needed > capacity);
    std::size_t new_cap = min_size_;
    // VFALCO check overflow here
    while(new_cap < needed)
        new_cap <<= 1;
    auto const begin =
        reinterpret_cast<value*>(
            sp_->allocate(
                new_cap * sizeof(value)));
    if(begin_)
    {
        std::size_t amount =
            size() * sizeof(value);
        if(chars_ > 0)
            amount += sizeof(value) + chars_;
        std::memcpy(
            reinterpret_cast<char*>(begin),
            reinterpret_cast<char*>(begin_),
            amount);
        if(begin_ != temp_)
            sp_->deallocate(begin_,
                capacity * sizeof(value));
    }
    // book-keeping
    top_ = begin + (top_ - begin_);
    end_ = begin + new_cap;
    begin_ = begin;
}

//--------------------------------------

void
value_stack::
stack::
append(string_view s)
{
    std::size_t const bytes_avail =
        reinterpret_cast<
            char const*>(end_) -
        reinterpret_cast<
            char const*>(top_);
    // make sure there is room for
    // pushing one more value without
    // clobbering the string.
    if(sizeof(value) + chars_ +
            s.size() > bytes_avail)
        grow(s.size());

    // copy the new piece
    std::memcpy(
        reinterpret_cast<char*>(
            top_ + 1) + chars_,
        s.data(), s.size());
    chars_ += s.size();

    // ensure a pushed value cannot
    // clobber the released string.
    BOOST_ASSERT(
        reinterpret_cast<char*>(
            top_ + 1) + chars_ <=
        reinterpret_cast<char*>(
            end_));
}

string_view
value_stack::
stack::
release_string() noexcept
{
    // ensure a pushed value cannot
    // clobber the released string.
    BOOST_ASSERT(
        reinterpret_cast<char*>(
            top_ + 1) + chars_ <=
        reinterpret_cast<char*>(
            end_));
    auto const n = chars_;
    chars_ = 0;
    return { reinterpret_cast<
        char const*>(top_ + 1), n };
}

// transfer ownership of the top n
// elements of the stack to the caller
value*
value_stack::
stack::
release(std::size_t n) noexcept
{
    BOOST_ASSERT(n <= size());
    BOOST_ASSERT(chars_ == 0);
    top_ -= n;
    return top_;
}

template<class... Args>
value&
value_stack::
stack::
push(Args&&... args)
{
    BOOST_ASSERT(chars_ == 0);
    if(top_ >= end_)
        grow_one();
    value& jv = detail::access::
        construct_value(top_,
            std::forward<Args>(args)...);
    ++top_;
    return jv;
}

template<class Unchecked>
void
value_stack::
stack::
exchange(Unchecked&& u)
{
    BOOST_ASSERT(chars_ == 0);
    union U
    {
        value v;
        U() {}
        ~U() {}
    } jv;
    // construct value on the stack
    // to avoid clobbering top_[0],
    // which belongs to `u`.
    detail::access::
        construct_value(
            &jv.v, std::move(u));
    std::memcpy(
        reinterpret_cast<
            char*>(top_),
        &jv.v, sizeof(value));
    ++top_;
}

//----------------------------------------------------------

value_stack::
~value_stack()
{
    // default dtor is here so the
    // definition goes in the library
    // instead of the caller's TU.
}

value_stack::
value_stack(
    storage_ptr sp,
    unsigned char* temp_buffer,
    std::size_t temp_size) noexcept
    : st_(
        std::move(sp),
        temp_buffer,
        temp_size)
{
}

void
value_stack::
reset(storage_ptr sp) noexcept
{
    st_.clear();

    sp_.~storage_ptr();
    ::new(&sp_) storage_ptr(
        pilfer(sp));

    // `stack` needs this
    // to clean up correctly
    st_.run_dtors(
        ! sp_.is_not_shared_and_deallocate_is_trivial());
}

value
value_stack::
release() noexcept
{
    // This means the caller did not
    // cause a single top level element
    // to be produced.
    BOOST_ASSERT(st_.size() == 1);

    // give up shared ownership
    sp_ = {};

    return pilfer(*st_.release(1));
}

//----------------------------------------------------------

void
value_stack::
push_array(std::size_t n)
{
    // we already have room if n > 0
    if(BOOST_JSON_UNLIKELY(n == 0))
        st_.maybe_grow();
    detail::unchecked_array ua(
        st_.release(n), n, sp_);
    st_.exchange(std::move(ua));
}

void
value_stack::
push_object(std::size_t n)
{
    // we already have room if n > 0
    if(BOOST_JSON_UNLIKELY(n == 0))
        st_.maybe_grow();
    detail::unchecked_object uo(
        st_.release(n * 2), n, sp_);
    st_.exchange(std::move(uo));
}

void
value_stack::
push_chars(
    string_view s)
{
    st_.append(s);
}

void
value_stack::
push_key(
    string_view s)
{
    if(! st_.has_chars())
    {
        st_.push(detail::key_t{}, s, sp_);
        return;
    }
    auto part = st_.release_string();
    st_.push(detail::key_t{}, part, s, sp_);
}

void
value_stack::
push_string(
    string_view s)
{
    if(! st_.has_chars())
    {
        // fast path
        st_.push(s, sp_);
        return;
    }

    // VFALCO We could add a special
    // private ctor to string that just
    // creates uninitialized space,
    // to reduce member function calls.
    auto part = st_.release_string();
    auto& str = st_.push(
        string_kind, sp_).get_string();
    str.reserve(
        part.size() + s.size());
    std::memcpy(
        str.data(),
        part.data(), part.size());
    std::memcpy(
        str.data() + part.size(),
        s.data(), s.size());
    str.grow(part.size() + s.size());
}

void
value_stack::
push_int64(
    int64_t i)
{
    st_.push(i, sp_);
}

void
value_stack::
push_uint64(
    uint64_t u)
{
    st_.push(u, sp_);
}

void
value_stack::
push_double(
    double d)
{
    st_.push(d, sp_);
}

void
value_stack::
push_bool(
    bool b)
{
    st_.push(b, sp_);
}

void
value_stack::
push_null()
{
    st_.push(nullptr, sp_);
}

BOOST_JSON_NS_END

#endif
