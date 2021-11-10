//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_STRING_HPP
#define BOOST_JSON_IMPL_STRING_HPP

#include <utility>

BOOST_JSON_NS_BEGIN

string::
string(
    detail::key_t const&,
    string_view s,
    storage_ptr sp)
    : sp_(std::move(sp))
    , impl_(detail::key_t{},
        s, sp_)
{
}

string::
string(
    detail::key_t const&,
    string_view s1,
    string_view s2,
    storage_ptr sp)
    : sp_(std::move(sp))
    , impl_(detail::key_t{},
        s1, s2, sp_)
{
}

template<class InputIt, class>
string::
string(
    InputIt first,
    InputIt last,
    storage_ptr sp)
    : sp_(std::move(sp))
    , impl_(first, last, sp_,
        iter_cat<InputIt>{})
{
}

template<class InputIt, class>
string&
string::
assign(
    InputIt first,
    InputIt last)
{
    assign(first, last,
        iter_cat<InputIt>{});
    return *this;
}

template<class InputIt, class>
string&
string::
append(InputIt first, InputIt last)
{
    append(first, last,
        iter_cat<InputIt>{});
    return *this;
}

// KRYSTIAN TODO: this can be done without copies when
// reallocation is not needed, when the iterator is a
// FowardIterator or better, as we can use std::distance
template<class InputIt, class>
auto
string::
insert(
    size_type pos,
    InputIt first,
    InputIt last) ->
        string&
{
    struct cleanup
    {
        detail::string_impl& s;
        storage_ptr const& sp;

        ~cleanup()
        {
            s.destroy(sp);
        }
    };

    // We use the default storage because
    // the allocation is immediately freed.
    storage_ptr dsp;
    detail::string_impl tmp(
        first, last, dsp,
        iter_cat<InputIt>{});
    cleanup c{tmp, dsp};
    std::memcpy(
        impl_.insert_unchecked(pos, tmp.size(), sp_),
        tmp.data(),
        tmp.size());
    return *this;
}

// KRYSTIAN TODO: this can be done without copies when
// reallocation is not needed, when the iterator is a
// FowardIterator or better, as we can use std::distance
template<class InputIt, class>
auto
string::
replace(
    const_iterator first,
    const_iterator last,
    InputIt first2,
    InputIt last2) ->
        string&
{
    struct cleanup
    {
        detail::string_impl& s;
        storage_ptr const& sp;

        ~cleanup()
        {
            s.destroy(sp);
        }
    };

    // We use the default storage because
    // the allocation is immediately freed.
    storage_ptr dsp;
    detail::string_impl tmp(
        first2, last2, dsp,
        iter_cat<InputIt>{});
    cleanup c{tmp, dsp};
    std::memcpy(
        impl_.replace_unchecked(
            first - begin(),
            last - first,
            tmp.size(),
            sp_),
        tmp.data(),
        tmp.size());
    return *this;
}

//----------------------------------------------------------

template<class InputIt>
void
string::
assign(
    InputIt first,
    InputIt last,
    std::random_access_iterator_tag)
{
    auto dest = impl_.assign(static_cast<
        size_type>(last - first), sp_);
    while(first != last)
        *dest++ = *first++;
}

template<class InputIt>
void
string::
assign(
    InputIt first,
    InputIt last,
    std::input_iterator_tag)
{
    if(first == last)
    {
        impl_.term(0);
        return;
    }
    detail::string_impl tmp(
        first, last, sp_,
        std::input_iterator_tag{});
    impl_.destroy(sp_);
    impl_ = tmp;
}

template<class InputIt>
void
string::
append(
    InputIt first,
    InputIt last,
    std::random_access_iterator_tag)
{
    auto const n = static_cast<
        size_type>(last - first);
    std::copy(first, last,
        impl_.append(n, sp_));
}

template<class InputIt>
void
string::
append(
    InputIt first,
    InputIt last,
    std::input_iterator_tag)
{
    struct cleanup
    {
        detail::string_impl& s;
        storage_ptr const& sp;

        ~cleanup()
        {
            s.destroy(sp);
        }
    };

    // We use the default storage because
    // the allocation is immediately freed.
    storage_ptr dsp;
    detail::string_impl tmp(
        first, last, dsp,
        std::input_iterator_tag{});
    cleanup c{tmp, dsp};
    std::memcpy(
        impl_.append(tmp.size(), sp_),
        tmp.data(), tmp.size());
}

BOOST_JSON_NS_END

#endif
