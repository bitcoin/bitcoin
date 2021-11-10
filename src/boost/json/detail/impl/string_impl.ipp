//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_IMPL_STRING_IMPL_IPP
#define BOOST_JSON_DETAIL_IMPL_STRING_IMPL_IPP

#include <boost/json/detail/string_impl.hpp>
#include <boost/json/detail/except.hpp>
#include <cstring>
#include <functional>

BOOST_JSON_NS_BEGIN
namespace detail {

inline
bool
ptr_in_range(
    const char* first,
    const char* last,
    const char* ptr) noexcept
{
    return std::less<const char*>()(ptr, last) &&
        std::greater_equal<const char*>()(ptr, first);
}

string_impl::
string_impl() noexcept
{
    s_.k = short_string_;
    s_.buf[sbo_chars_] =
        static_cast<char>(
            sbo_chars_);
    s_.buf[0] = 0;
}

string_impl::
string_impl(
    std::size_t size,
    storage_ptr const& sp)
{
    if(size <= sbo_chars_)
    {
        s_.k = short_string_;
        s_.buf[sbo_chars_] =
            static_cast<char>(
                sbo_chars_ - size);
        s_.buf[size] = 0;
    }
    else
    {
        s_.k = kind::string;
        auto const n = growth(
            size, sbo_chars_ + 1);
        p_.t = ::new(sp->allocate(
            sizeof(table) +
                n + 1,
            alignof(table))) table{
                static_cast<
                    std::uint32_t>(size),
                static_cast<
                    std::uint32_t>(n)};
        data()[n] = 0;
    }
}

// construct a key, unchecked
string_impl::
string_impl(
    key_t,
    string_view s,
    storage_ptr const& sp)
{
    BOOST_ASSERT(
        s.size() <= max_size());
    k_.k = key_string_;
    k_.n = static_cast<
        std::uint32_t>(s.size());
    k_.s = reinterpret_cast<char*>(
        sp->allocate(s.size() + 1,
            alignof(char)));
    k_.s[s.size()] = 0; // null term
    std::memcpy(&k_.s[0],
        s.data(), s.size());
}

// construct a key, unchecked
string_impl::
string_impl(
    key_t,
    string_view s1,
    string_view s2,
    storage_ptr const& sp)
{
    auto len = s1.size() + s2.size();
    BOOST_ASSERT(len <= max_size());
    k_.k = key_string_;
    k_.n = static_cast<
        std::uint32_t>(len);
    k_.s = reinterpret_cast<char*>(
        sp->allocate(len + 1,
            alignof(char)));
    k_.s[len] = 0; // null term
    std::memcpy(&k_.s[0],
        s1.data(), s1.size());
    std::memcpy(&k_.s[s1.size()],
        s2.data(), s2.size());
}

std::uint32_t
string_impl::
growth(
    std::size_t new_size,
    std::size_t capacity)
{
    if(new_size > max_size())
        detail::throw_length_error(
            "string too large",
            BOOST_JSON_SOURCE_POS);
    // growth factor 2
    if( capacity >
        max_size() - capacity)
        return static_cast<
            std::uint32_t>(max_size()); // overflow
    return static_cast<std::uint32_t>(
        (std::max)(capacity * 2, new_size));
}

char*
string_impl::
assign(
    std::size_t new_size,
    storage_ptr const& sp)
{
    if(new_size > capacity())
    {
        string_impl tmp(growth(
            new_size,
            capacity()), sp);
        destroy(sp);
        *this = tmp;
    }
    term(new_size);
    return data();
}

char*
string_impl::
append(
    std::size_t n,
    storage_ptr const& sp)
{
    if(n > max_size() - size())
        detail::throw_length_error(
            "string too large",
            BOOST_JSON_SOURCE_POS);
    if(n <= capacity() - size())
    {
        term(size() + n);
        return end() - n;
    }
    string_impl tmp(growth(
        size() + n, capacity()), sp);
    std::memcpy(
        tmp.data(), data(), size());
    tmp.term(size() + n);
    destroy(sp);
    *this = tmp;
    return end() - n;
}

void
string_impl::
insert(
    std::size_t pos,
    const char* s,
    std::size_t n,
    storage_ptr const& sp)
{
    const auto curr_size = size();
    if(pos > curr_size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    const auto curr_data = data();
    if(n <= capacity() - curr_size)
    {
        const bool inside = detail::ptr_in_range(curr_data, curr_data + curr_size, s);
        if (!inside || (inside && ((s - curr_data) + n <= pos)))
        {
            std::memmove(&curr_data[pos + n], &curr_data[pos], curr_size - pos + 1);
            std::memcpy(&curr_data[pos], s, n);
        }
        else
        {
            const std::size_t offset = s - curr_data;
            std::memmove(&curr_data[pos + n], &curr_data[pos], curr_size - pos + 1);
            if (offset < pos)
            {
                const std::size_t diff = pos - offset;
                std::memcpy(&curr_data[pos], &curr_data[offset], diff);
                std::memcpy(&curr_data[pos + diff], &curr_data[pos + n], n - diff);
            }
            else
            {
                std::memcpy(&curr_data[pos], &curr_data[offset + n], n);
            }
        }
        size(curr_size + n);
    }
    else
    {
        if(n > max_size() - curr_size)
            detail::throw_length_error(
                "string too large",
                BOOST_JSON_SOURCE_POS);
        string_impl tmp(growth(
            curr_size + n, capacity()), sp);
        tmp.size(curr_size + n);
        std::memcpy(
            tmp.data(),
            curr_data,
            pos);
        std::memcpy(
            tmp.data() + pos + n,
            curr_data + pos,
            curr_size + 1 - pos);
        std::memcpy(
            tmp.data() + pos,
            s,
            n);
        destroy(sp);
        *this = tmp;
    }
}

char*
string_impl::
insert_unchecked(
    std::size_t pos,
    std::size_t n,
    storage_ptr const& sp)
{
    const auto curr_size = size();
    if(pos > curr_size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    const auto curr_data = data();
    if(n <= capacity() - size())
    {
        auto const dest =
            curr_data + pos;
        std::memmove(
            dest + n,
            dest,
            curr_size + 1 - pos);
        size(curr_size + n);
        return dest;
    }
    if(n > max_size() - curr_size)
        detail::throw_length_error(
            "string too large",
            BOOST_JSON_SOURCE_POS);
    string_impl tmp(growth(
        curr_size + n, capacity()), sp);
    tmp.size(curr_size + n);
    std::memcpy(
        tmp.data(),
        curr_data,
        pos);
    std::memcpy(
        tmp.data() + pos + n,
        curr_data + pos,
        curr_size + 1 - pos);
    destroy(sp);
    *this = tmp;
    return data() + pos;
}

void
string_impl::
replace(
    std::size_t pos,
    std::size_t n1,
    const char* s,
    std::size_t n2,
    storage_ptr const& sp)
{
    const auto curr_size = size();
    if (pos > curr_size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    const auto curr_data = data();
    n1 = (std::min)(n1, curr_size - pos);
    const auto delta = (std::max)(n1, n2) -
        (std::min)(n1, n2);
    // if we are shrinking in size or we have enough
    // capacity, dont reallocate
    if (n1 > n2 || delta <= capacity() - curr_size)
    {
        const bool inside = detail::ptr_in_range(curr_data, curr_data + curr_size, s);
        // there is nothing to replace; return
        if (inside && s == curr_data + pos && n1 == n2)
            return;
        if (!inside || (inside && ((s - curr_data) + n2 <= pos)))
        {
            // source outside
            std::memmove(&curr_data[pos + n2], &curr_data[pos + n1], curr_size - pos - n1 + 1);
            std::memcpy(&curr_data[pos], s, n2);
        }
        else
        {
            // source inside
            const std::size_t offset = s - curr_data;
            if (n2 >= n1)
            {
                // grow/unchanged
                const std::size_t diff = offset <= pos + n1 ? (std::min)((pos + n1) - offset, n2) : 0;
                // shift all right of splice point by n2 - n1 to the right
                std::memmove(&curr_data[pos + n2], &curr_data[pos + n1], curr_size - pos - n1 + 1);
                // copy all before splice point
                std::memmove(&curr_data[pos], &curr_data[offset], diff);
                // copy all after splice point
                std::memmove(&curr_data[pos + diff], &curr_data[(offset - n1) + n2 + diff], n2 - diff);
            }
            else
            {
                // shrink
                // copy all elements into place
                std::memmove(&curr_data[pos], &curr_data[offset], n2);
                // shift all elements after splice point left
                std::memmove(&curr_data[pos + n2], &curr_data[pos + n1], curr_size - pos - n1 + 1);
            }
        }
        size((curr_size - n1) + n2);
    }
    else
    {
        if (delta > max_size() - curr_size)
            detail::throw_length_error(
                "string too large",
                BOOST_JSON_SOURCE_POS);
        // would exceed capacity, reallocate
        string_impl tmp(growth(
            curr_size + delta, capacity()), sp);
        tmp.size(curr_size + delta);
        std::memcpy(
            tmp.data(),
            curr_data,
            pos);
        std::memcpy(
            tmp.data() + pos + n2,
            curr_data + pos + n1,
            curr_size - pos - n1 + 1);
        std::memcpy(
            tmp.data() + pos,
            s,
            n2);
        destroy(sp);
        *this = tmp;
    }
}

// unlike the replace overload, this function does
// not move any characters
char*
string_impl::
replace_unchecked(
    std::size_t pos,
    std::size_t n1,
    std::size_t n2,
    storage_ptr const& sp)
{
    const auto curr_size = size();
    if(pos > curr_size)
        detail::throw_out_of_range(
            BOOST_JSON_SOURCE_POS);
    const auto curr_data = data();
    const auto delta = (std::max)(n1, n2) -
        (std::min)(n1, n2);
    // if the size doesn't change, we don't need to
    // do anything
    if (!delta)
      return curr_data + pos;
    // if we are shrinking in size or we have enough
    // capacity, dont reallocate
    if(n1 > n2 || delta <= capacity() - curr_size)
    {
        auto const replace_pos = curr_data + pos;
        std::memmove(
            replace_pos + n2,
            replace_pos + n1,
            curr_size - pos - n1 + 1);
        size((curr_size - n1) + n2);
        return replace_pos;
    }
    if(delta > max_size() - curr_size)
        detail::throw_length_error(
            "string too large",
            BOOST_JSON_SOURCE_POS);
    // would exceed capacity, reallocate
    string_impl tmp(growth(
        curr_size + delta, capacity()), sp);
    tmp.size(curr_size + delta);
    std::memcpy(
        tmp.data(),
        curr_data,
        pos);
    std::memcpy(
        tmp.data() + pos + n2,
        curr_data + pos + n1,
        curr_size - pos - n1 + 1);
    destroy(sp);
    *this = tmp;
    return data() + pos;
}

void
string_impl::
shrink_to_fit(
    storage_ptr const& sp) noexcept
{
    if(s_.k == short_string_)
        return;
    auto const t = p_.t;
    if(t->size <= sbo_chars_)
    {
        s_.k = short_string_;
        std::memcpy(
            s_.buf, data(), t->size);
        s_.buf[sbo_chars_] =
            static_cast<char>(
                sbo_chars_ - t->size);
        s_.buf[t->size] = 0;
        sp->deallocate(t,
            sizeof(table) +
                t->capacity + 1,
            alignof(table));
        return;
    }
    if(t->size >= t->capacity)
        return;
#ifndef BOOST_NO_EXCEPTIONS
    try
    {
#endif
        string_impl tmp(t->size, sp);
        std::memcpy(
            tmp.data(),
            data(),
            size());
        destroy(sp);
        *this = tmp;
#ifndef BOOST_NO_EXCEPTIONS
    }
    catch(std::exception const&)
    {
        // eat the exception
    }
#endif
}

} // detail
BOOST_JSON_NS_END

#endif
