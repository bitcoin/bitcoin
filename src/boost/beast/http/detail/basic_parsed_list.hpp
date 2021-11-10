//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_DETAIL_BASIC_PARSED_LIST_HPP
#define BOOST_BEAST_HTTP_DETAIL_BASIC_PARSED_LIST_HPP

#include <boost/beast/core/string.hpp>
#include <boost/core/empty_value.hpp>
#include <cstddef>
#include <iterator>

namespace boost {
namespace beast {
namespace http {
namespace detail {

/** A list parser which presents the sequence as a container.
*/
template<class Policy>
class basic_parsed_list
{
    string_view s_;

public:
    /// The type of policy this list uses for parsing.
    using policy_type = Policy;

    /// The type of each element in the list.
    using value_type = typename Policy::value_type;

    /// A constant iterator to a list element.
#if BOOST_BEAST_DOXYGEN
    using const_iterator = __implementation_defined__;
#else
    class const_iterator;
#endif

    class const_iterator
        : private boost::empty_value<Policy>
    {
        basic_parsed_list const* list_ = nullptr;
        char const* it_ = nullptr;
        typename Policy::value_type v_;
        bool error_ = false;

    public:
        using value_type =
            typename Policy::value_type;
        using reference = value_type const&;
        using pointer = value_type const*;
        using difference_type = std::ptrdiff_t;
        using iterator_category =
            std::forward_iterator_tag;

        const_iterator() = default;

        bool
        operator==(
            const_iterator const& other) const
        {
            return
                other.list_ == list_ &&
                other.it_ == it_;
        }

        bool
        operator!=(
            const_iterator const& other) const
        {
            return ! (*this == other);
        }

        reference
        operator*() const
        {
            return v_;
        }

        const_iterator&
        operator++()
        {
            increment();
            return *this;
        }

        const_iterator
        operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        bool
        error() const
        {
            return error_;
        }

    private:
        friend class basic_parsed_list;

        const_iterator(
            basic_parsed_list const& list, bool at_end)
            : list_(&list)
            , it_(at_end ? nullptr :
                list.s_.data())
        {
            if(! at_end)
                increment();
        }

        void
        increment()
        {
            if(! this->get()(
                    v_, it_, list_->s_))
            {
                it_ = nullptr;
                error_ = true;
            }
        }
    };

    /// Construct a list from a string
    explicit
    basic_parsed_list(string_view s)
        : s_(s)
    {
    }

    /// Return a const iterator to the beginning of the list
    const_iterator begin() const;

    /// Return a const iterator to the end of the list
    const_iterator end() const;

    /// Return a const iterator to the beginning of the list
    const_iterator cbegin() const;

    /// Return a const iterator to the end of the list
    const_iterator cend() const;
};

template<class Policy>
inline
auto
basic_parsed_list<Policy>::
begin() const ->
    const_iterator
{
    return const_iterator{*this, false};
}

template<class Policy>
inline
auto
basic_parsed_list<Policy>::
end() const ->
    const_iterator
{
    return const_iterator{*this, true};
}

template<class Policy>
inline
auto
basic_parsed_list<Policy>::
cbegin() const ->
    const_iterator
{
    return const_iterator{*this, false};
}

template<class Policy>
inline
auto
basic_parsed_list<Policy>::
cend() const ->
    const_iterator
{
    return const_iterator{*this, true};
}

} // detail
} // http
} // beast
} // boost

#endif

