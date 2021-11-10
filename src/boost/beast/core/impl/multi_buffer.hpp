//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_MULTI_BUFFER_HPP
#define BOOST_BEAST_IMPL_MULTI_BUFFER_HPP

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/config/workaround.hpp>
#include <boost/core/exchange.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <exception>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {

/*  These diagrams illustrate the layout and state variables.

1   Input and output contained entirely in one element:

    0                           out_
    |<------+-----------+--------------------------------+----->|
          in_pos_    out_pos_                         out_end_


2   Output contained in first and second elements:

                 out_
    |<------+-----------+------>|   |<-------------------+----->|
          in_pos_    out_pos_                         out_end_


3   Output contained in the second element:

                                                  out_
    |<------+------------------>|   |<----+--------------+----->|
          in_pos_                      out_pos_       out_end_


4   Output contained in second and third elements:

                                 out_
    |<------+------->|   |<-------+------>|   |<---------+----->|
          in_pos_               out_pos_              out_end_


5   Input sequence is empty:

                 out_
    |<------+------------------>|   |<-------------------+----->|
         out_pos_                                     out_end_
          in_pos_

6   Output sequence is empty:

                                                    out_
    |<------+------------------>|   |<------+------------------>|
          in_pos_                        out_pos_
                                         out_end_


7   The end of output can point to the end of an element.
    But out_pos_ should never point to the end:

                                                    out_
    |<------+------------------>|   |<------+------------------>|
          in_pos_                        out_pos_            out_end_


8   When the input sequence entirely fills the last element and
    the output sequence is empty, out_ will point to the end of
    the list of buffers, and out_pos_ and out_end_ will be 0:


    |<------+------------------>|   out_     == list_.end()
          in_pos_                   out_pos_ == 0
                                    out_end_ == 0
*/

//------------------------------------------------------------------------------

#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
# pragma warning (push)
# pragma warning (disable: 4521) // multiple copy constructors specified
# pragma warning (disable: 4522) // multiple assignment operators specified
#endif

template<class Allocator>
template<bool isMutable>
class basic_multi_buffer<Allocator>::subrange
{
    basic_multi_buffer const* b_;
    const_iter begin_;
    const_iter end_;
    size_type begin_pos_;   // offset in begin_
    size_type last_pos_;    // offset in std::prev(end_)

    friend class basic_multi_buffer;

    subrange(
        basic_multi_buffer const& b,
        size_type pos,
        size_type n) noexcept
        : b_(&b)
    {
        auto const set_empty = [&]
        {
            begin_ = b_->list_.end();
            end_ = b_->list_.end();
            begin_pos_ = 0;
            last_pos_ = 0;
        };

        // VFALCO Handle this trivial case of
        // pos larger than total size, otherwise
        // the addition to pos can overflow.
        //if(pos >= b_->in_size_)
        // skip unused prefix
        pos = pos + b_->in_pos_;

        // iterate the buffers
        auto it = b_->list_.begin();

        // is the list empty?
        if(it == b_->list_.end())
        {
            set_empty();
            return;
        }
        
        // is the requested size zero?
        if(n == 0)
        {
            set_empty();
            return;
        }


        // get last buffer and its size
        auto const last =
            std::prev(b_->list_.end());
        auto const last_end =
            [&]
            {
                if(b_->out_end_ == 0)
                    return last->size();
                return b_->out_end_;
            }();

        // only one buffer in list?
        if(it == last)
        {
            if(pos >= last_end)
            {
                set_empty();
                return;
            }

            begin_ = it;
            begin_pos_ = pos;
            end_ = std::next(it);
            if(n > last_end - pos)
                last_pos_ = last_end;
            else
                last_pos_ = pos + n;
            return;
        }

        for(;;)
        {
            // is pos in this buffer?
            if(pos < it->size())
            {
                begin_ = it;
                begin_pos_ = pos;

                // does this buffer satisfy n?
                auto const avail =
                    it->size() - pos;
                if(n <= avail)
                {
                    end_ = ++it;
                    last_pos_ = pos + n;
                    return;
                }

                n -= avail;
                ++it;
                break;
            }

            pos -= it->size();
            ++it;

            // did we reach the last buffer?
            if(it == last)
            {
                // is pos past the end?
                if(pos >= last_end)
                {
                    set_empty();
                    return;
                }

                // satisfy the request
                begin_ = it;
                begin_pos_ = pos;
                end_ = std::next(it);
                if(n < last_end - pos)
                    last_pos_ = pos + n;
                else
                    last_pos_ = last_end;
                return;
            }
        }

        // find pos+n
        for(;;)
        {
            if(it == last)
            {
                end_ = ++it;
                if(n >= last_end)
                    last_pos_ = last_end;
                else
                    last_pos_ = n;
                return;
            }
            if(n <= it->size())
            {
                end_ = ++it;
                last_pos_ = n;
                return;
            }
            
            n -= it->size();
            ++it;
        }
    }

public:
    using value_type = typename
        std::conditional<
            isMutable,
            net::mutable_buffer,
            net::const_buffer>::type;

    class const_iterator;

    subrange() = delete;
#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
    subrange(subrange const& other)
        : b_(other.b_)
        , begin_(other.begin_)
        , end_(other.end_)
        , begin_pos_(other.begin_pos_)
        , last_pos_(other.last_pos_)
    {
    }

    subrange& operator=(subrange const& other)
    {
        b_ = other.b_;
        begin_ = other.begin_;
        end_ = other.end_;
        begin_pos_ = other.begin_pos_;
        last_pos_ = other.last_pos_;
        return *this;
    }
#else
    subrange(subrange const&) = default;
    subrange& operator=(subrange const&) = default;
#endif

    template<
        bool isMutable_ = isMutable,
        class = typename std::enable_if<! isMutable_>::type>
    subrange(
        subrange<true> const& other) noexcept
        : b_(other.b_)
        , begin_(other.begin_)
        , end_(other.end_)
        , begin_pos_(other.begin_pos_)
        , last_pos_(other.last_pos_)
   {
    }

    template<
        bool isMutable_ = isMutable,
        class = typename std::enable_if<! isMutable_>::type>
    subrange& operator=(
        subrange<true> const& other) noexcept
    {
        b_ = other.b_;
        begin_ = other.begin_;
        end_ = other.end_;
        begin_pos_ = other.begin_pos_;
        last_pos_ = other.last_pos_;
        return *this;
    }

    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    std::size_t
    buffer_bytes() const noexcept
    {
        return b_->size();
    }
};

#if BOOST_WORKAROUND(BOOST_MSVC, < 1910)
# pragma warning (pop)
#endif

//------------------------------------------------------------------------------

template<class Allocator>
template<bool isMutable>
class
    basic_multi_buffer<Allocator>::
    subrange<isMutable>::
    const_iterator
{
    friend class subrange;

    subrange const* sr_ = nullptr;
    typename list_type::const_iterator it_;

    const_iterator(
        subrange const& sr, typename
        list_type::const_iterator const& it) noexcept
        : sr_(&sr)
        , it_(it)
    {
    }

public:
    using value_type =
        typename subrange::value_type;
    using pointer = value_type const*;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::bidirectional_iterator_tag;

    const_iterator() = default;
    const_iterator(
        const_iterator const& other) = default;
    const_iterator& operator=(
        const_iterator const& other) = default;

    bool
    operator==(
        const_iterator const& other) const noexcept
    {
        return sr_ == other.sr_ && it_ == other.it_;
    }

    bool
    operator!=(
        const_iterator const& other) const noexcept
    {
        return !(*this == other);
    }

    reference
    operator*() const noexcept
    {
        value_type result;
        BOOST_ASSERT(sr_->last_pos_ != 0);
        if(it_ == std::prev(sr_->end_))
            result = {
                it_->data(), sr_->last_pos_ };
        else
            result = {
                it_->data(), it_->size() };
        if(it_ == sr_->begin_)
            result += sr_->begin_pos_;
        return result;
    }

    pointer
    operator->() const = delete;

    const_iterator&
    operator++() noexcept
    {
        ++it_;
        return *this;
    }

    const_iterator
    operator++(int) noexcept
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    const_iterator&
    operator--() noexcept
    {
        --it_;
        return *this;
    }

    const_iterator
    operator--(int) noexcept
    {
        auto temp = *this;
        --(*this);
        return temp;
    }
};

//------------------------------------------------------------------------------

template<class Allocator>
template<bool isMutable>
auto
basic_multi_buffer<Allocator>::
subrange<isMutable>::
begin() const noexcept ->
    const_iterator
{
    return const_iterator(
        *this, begin_);
}

template<class Allocator>
template<bool isMutable>
auto
basic_multi_buffer<Allocator>::
subrange<isMutable>::
end() const noexcept ->
    const_iterator
{
    return const_iterator(
        *this, end_);
}

//------------------------------------------------------------------------------

template<class Allocator>
basic_multi_buffer<Allocator>::
~basic_multi_buffer()
{
    destroy(list_);
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer() noexcept(default_nothrow)
    : max_(alloc_traits::max_size(this->get()))
    , out_(list_.end())
{
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    std::size_t limit) noexcept(default_nothrow)
    : max_(limit)
    , out_(list_.end())
{
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    Allocator const& alloc) noexcept
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc)
    , max_(alloc_traits::max_size(this->get()))
    , out_(list_.end())
{
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    std::size_t limit,
    Allocator const& alloc) noexcept
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc)
    , max_(limit)
    , out_(list_.end())
{
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    basic_multi_buffer&& other) noexcept
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), std::move(other.get()))
    , max_(other.max_)
    , in_size_(boost::exchange(other.in_size_, 0))
    , in_pos_(boost::exchange(other.in_pos_, 0))
    , out_pos_(boost::exchange(other.out_pos_, 0))
    , out_end_(boost::exchange(other.out_end_, 0))
{
    auto const at_end =
        other.out_ == other.list_.end();
    list_ = std::move(other.list_);
    out_ = at_end ? list_.end() : other.out_;
    other.out_ = other.list_.end();
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    basic_multi_buffer&& other,
    Allocator const& alloc)
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc)
    , max_(other.max_)
{
    if(this->get() != other.get())
    {
        out_ = list_.end();
        copy_from(other);
        return;
    }

    auto const at_end =
        other.out_ == other.list_.end();
    list_ = std::move(other.list_);
    out_ = at_end ? list_.end() : other.out_;
    in_size_ = other.in_size_;
    in_pos_ = other.in_pos_;
    out_pos_ = other.out_pos_;
    out_end_ = other.out_end_;
    other.in_size_ = 0;
    other.out_ = other.list_.end();
    other.in_pos_ = 0;
    other.out_pos_ = 0;
    other.out_end_ = 0;
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    basic_multi_buffer const& other)
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc_traits::
            select_on_container_copy_construction(
                other.get()))
    , max_(other.max_)
    , out_(list_.end())
{
    copy_from(other);
}

template<class Allocator>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    basic_multi_buffer const& other,
    Allocator const& alloc)
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc)
    , max_(other.max_)
    , out_(list_.end())
{
    copy_from(other);
}

template<class Allocator>
template<class OtherAlloc>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
        basic_multi_buffer<OtherAlloc> const& other)
    : out_(list_.end())
{
    copy_from(other);
}

template<class Allocator>
template<class OtherAlloc>
basic_multi_buffer<Allocator>::
basic_multi_buffer(
    basic_multi_buffer<OtherAlloc> const& other,
        allocator_type const& alloc)
    : boost::empty_value<Allocator>(
        boost::empty_init_t(), alloc)
    , max_(other.max_)
    , out_(list_.end())
{
    copy_from(other);
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
operator=(basic_multi_buffer&& other) ->
    basic_multi_buffer&
{
    if(this == &other)
        return *this;
    clear();
    max_ = other.max_;
    move_assign(other, pocma{});
    return *this;
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
operator=(basic_multi_buffer const& other) ->
basic_multi_buffer&
{
    if(this == &other)
        return *this;
    copy_assign(other, pocca{});
    return *this;
}

template<class Allocator>
template<class OtherAlloc>
auto
basic_multi_buffer<Allocator>::
operator=(
    basic_multi_buffer<OtherAlloc> const& other) ->
        basic_multi_buffer&
{
    copy_from(other);
    return *this;
}

//------------------------------------------------------------------------------

template<class Allocator>
std::size_t
basic_multi_buffer<Allocator>::
capacity() const noexcept
{
    auto pos = out_;
    if(pos == list_.end())
        return in_size_;
    auto n = pos->size() - out_pos_;
    while(++pos != list_.end())
        n += pos->size();
    return in_size_ + n;
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
data() const noexcept ->
    const_buffers_type
{
    return const_buffers_type(
        *this, 0, in_size_);
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
data() noexcept ->
    mutable_buffers_type
{
    return mutable_buffers_type(
        *this, 0, in_size_);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
reserve(std::size_t n)
{
    // VFALCO The amount needs to be adjusted for
    //        the sizeof(element) plus padding
    if(n > alloc_traits::max_size(this->get()))
        BOOST_THROW_EXCEPTION(std::length_error(
        "A basic_multi_buffer exceeded the allocator's maximum size"));
    std::size_t total = in_size_;
    if(n <= total)
        return;
    if(out_ != list_.end())
    {
        total += out_->size() - out_pos_;
        if(n <= total)
            return;
        for(auto it = out_;;)
        {
            if(++it == list_.end())
                break;
            total += it->size();
            if(n <= total)
                return;
        }
    }
    BOOST_ASSERT(n > total);
    (void)prepare(n - size());
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
shrink_to_fit()
{
    // empty list
    if(list_.empty())
        return;

    // zero readable bytes
    if(in_size_ == 0)
    {
        destroy(list_);
        list_.clear();
        out_ = list_.end();
        in_size_ = 0;
        in_pos_ = 0;
        out_pos_ = 0;
        out_end_ = 0;
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
        return;
    }

    // one or more unused output buffers
    if(out_ != list_.end())
    {
        if(out_ != list_.iterator_to(list_.back()))
        {
            // unused list
            list_type extra;
            extra.splice(
                extra.end(),
                list_,
                std::next(out_),
                list_.end());
            destroy(extra);
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
        }

        // unused out_
        BOOST_ASSERT(out_ ==
            list_.iterator_to(list_.back()));
        if(out_pos_ == 0)
        {
            BOOST_ASSERT(out_ != list_.begin());
            auto& e = *out_;
            list_.erase(out_);
            out_ = list_.end();
            destroy(e);
            out_end_ = 0;
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
        }
    }

    auto const replace =
        [&](iter pos, element& e)
        {
            auto it =
                list_.insert(pos, e);
            auto& e0 = *pos;
            list_.erase(pos);
            destroy(e0);
            return it;
        };

    // partial last buffer
    if(list_.size() > 1 && out_ != list_.end())
    {
        BOOST_ASSERT(out_ ==
            list_.iterator_to(list_.back()));
        BOOST_ASSERT(out_pos_ != 0);
        auto& e = alloc(out_pos_);
        std::memcpy(
            e.data(),
            out_->data(),
            out_pos_);
        replace(out_, e);
        out_ = list_.end();
        out_pos_ = 0;
        out_end_ = 0;
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
    }

    // partial first buffer
    if(in_pos_ != 0)
    {
        if(out_ != list_.begin())
        {
            auto const n =
                list_.front().size() - in_pos_;
            auto& e = alloc(n);
            std::memcpy(
                e.data(),
                list_.front().data() + in_pos_,
                n);
            replace(list_.begin(), e);
            in_pos_ = 0;
        }
        else
        {
            BOOST_ASSERT(list_.size() == 1);
            BOOST_ASSERT(out_pos_ > in_pos_);
            auto const n = out_pos_ - in_pos_;
            auto& e = alloc(n);
            std::memcpy(
                e.data(),
                list_.front().data() + in_pos_,
                n);
            replace(list_.begin(), e);
            in_pos_ = 0;
            out_ = list_.end();
        }
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
    }
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
clear() noexcept
{
    out_ = list_.begin();
    in_size_ = 0;
    in_pos_ = 0;
    out_pos_ = 0;
    out_end_ = 0;
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
prepare(size_type n) ->
    mutable_buffers_type
{
    auto const n0 = n;
    if(in_size_ > max_ || n > (max_ - in_size_))
        BOOST_THROW_EXCEPTION(std::length_error{
            "basic_multi_buffer too long"});
    list_type reuse;
    std::size_t total = in_size_;
    // put all empty buffers on reuse list
    if(out_ != list_.end())
    {
        total += out_->size() - out_pos_;
        if(out_ != list_.iterator_to(list_.back()))
        {
            out_end_ = out_->size();
            reuse.splice(reuse.end(), list_,
                std::next(out_), list_.end());
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
        }
        auto const avail = out_->size() - out_pos_;
        if(n > avail)
        {
            out_end_ = out_->size();
            n -= avail;
        }
        else
        {
            out_end_ = out_pos_ + n;
            n = 0;
        }
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
    }
    // get space from reuse buffers
    while(n > 0 && ! reuse.empty())
    {
        auto& e = reuse.front();
        reuse.erase(reuse.iterator_to(e));
        list_.push_back(e);
        total += e.size();
        if(n > e.size())
        {
            out_end_ = e.size();
            n -= e.size();
        }
        else
        {
            out_end_ = n;
            n = 0;
        }
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
    }
    BOOST_ASSERT(total <= max_);
    if(! reuse.empty() || n > 0)
    {
        destroy(reuse);
        if(n > 0)
        {
            std::size_t const growth_factor = 2;
            std::size_t altn = in_size_ * growth_factor;
	    // Overflow detection:
            if(in_size_ > altn)
                altn = (std::numeric_limits<std::size_t>::max)();
            else
                altn = (std::max<std::size_t>)(512, altn);
            auto const size =
                (std::min<std::size_t>)(
                    max_ - total,
                    (std::max<std::size_t>)(n, altn));
            auto& e = alloc(size);
            list_.push_back(e);
            if(out_ == list_.end())
                out_ = list_.iterator_to(e);
            out_end_ = n;
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
        }
    }
    auto const result =
        mutable_buffers_type(
            *this, in_size_, n0);
    BOOST_ASSERT(
        net::buffer_size(result) == n0);
    return result;
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
commit(size_type n) noexcept
{
    if(list_.empty())
        return;
    if(out_ == list_.end())
        return;
    auto const back =
        list_.iterator_to(list_.back());
    while(out_ != back)
    {
        auto const avail =
            out_->size() - out_pos_;
        if(n < avail)
        {
            out_pos_ += n;
            in_size_ += n;
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
            return;
        }
        ++out_;
        n -= avail;
        out_pos_ = 0;
        in_size_ += avail;
    #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
        debug_check();
    #endif
    }

    n = (std::min)(n, out_end_ - out_pos_);
    out_pos_ += n;
    in_size_ += n;
    if(out_pos_ == out_->size())
    {
        ++out_;
        out_pos_ = 0;
        out_end_ = 0;
    }
#if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
    debug_check();
#endif
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
consume(size_type n) noexcept
{
    if(list_.empty())
        return;
    for(;;)
    {
        if(list_.begin() != out_)
        {
            auto const avail =
                list_.front().size() - in_pos_;
            if(n < avail)
            {
                in_size_ -= n;
                in_pos_ += n;
            #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
                debug_check();
            #endif
                break;
            }
            n -= avail;
            in_size_ -= avail;
            in_pos_ = 0;
            auto& e = list_.front();
            list_.erase(list_.iterator_to(e));
            destroy(e);
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
        }
        else
        {
            auto const avail = out_pos_ - in_pos_;
            if(n < avail)
            {
                in_size_ -= n;
                in_pos_ += n;
            }
            else
            {
                in_size_ = 0;
                if(out_ != list_.iterator_to(list_.back()) ||
                    out_pos_ != out_end_)
                {
                    in_pos_ = out_pos_;
                }
                else
                {
                    // Input and output sequences are empty, reuse buffer.
                    // Alternatively we could deallocate it.
                    in_pos_ = 0;
                    out_pos_ = 0;
                    out_end_ = 0;
                }
            }
        #if BOOST_BEAST_MULTI_BUFFER_DEBUG_CHECK
            debug_check();
        #endif
            break;
        }
    }
}

template<class Allocator>
template<class OtherAlloc>
void
basic_multi_buffer<Allocator>::
copy_from(basic_multi_buffer<OtherAlloc> const& other)
{
    clear();
    max_ = other.max_;
    if(other.size() == 0)
        return;
    commit(net::buffer_copy(
        prepare(other.size()), other.data()));
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
move_assign(basic_multi_buffer& other, std::true_type) noexcept
{
    this->get() = std::move(other.get());
    auto const at_end =
        other.out_ == other.list_.end();
    list_ = std::move(other.list_);
    out_ = at_end ? list_.end() : other.out_;

    in_size_ = other.in_size_;
    in_pos_ = other.in_pos_;
    out_pos_ = other.out_pos_;
    out_end_ = other.out_end_;
    max_ = other.max_;

    other.in_size_ = 0;
    other.out_ = other.list_.end();
    other.in_pos_ = 0;
    other.out_pos_ = 0;
    other.out_end_ = 0;
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
move_assign(basic_multi_buffer& other, std::false_type)
{
    if(this->get() != other.get())
    {
        copy_from(other);
    }
    else
    {
        move_assign(other, std::true_type{});
    }
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
copy_assign(
    basic_multi_buffer const& other, std::false_type)
{
    copy_from(other);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
copy_assign(
    basic_multi_buffer const& other, std::true_type)
{
    clear();
    this->get() = other.get();
    copy_from(other);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
swap(basic_multi_buffer& other) noexcept
{
    swap(other, typename
        alloc_traits::propagate_on_container_swap{});
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
swap(basic_multi_buffer& other, std::true_type) noexcept
{
    using std::swap;
    auto const at_end0 =
        out_ == list_.end();
    auto const at_end1 =
        other.out_ == other.list_.end();
    swap(this->get(), other.get());
    swap(list_, other.list_);
    swap(out_, other.out_);
    if(at_end1)
        out_ = list_.end();
    if(at_end0)
        other.out_ = other.list_.end();
    swap(in_size_, other.in_size_);
    swap(in_pos_, other.in_pos_);
    swap(out_pos_, other.out_pos_);
    swap(out_end_, other.out_end_);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
swap(basic_multi_buffer& other, std::false_type) noexcept
{
    BOOST_ASSERT(this->get() == other.get());
    using std::swap;
    auto const at_end0 =
        out_ == list_.end();
    auto const at_end1 =
        other.out_ == other.list_.end();
    swap(list_, other.list_);
    swap(out_, other.out_);
    if(at_end1)
        out_ = list_.end();
    if(at_end0)
        other.out_ = other.list_.end();
    swap(in_size_, other.in_size_);
    swap(in_pos_, other.in_pos_);
    swap(out_pos_, other.out_pos_);
    swap(out_end_, other.out_end_);
}

template<class Allocator>
void
swap(
    basic_multi_buffer<Allocator>& lhs,
    basic_multi_buffer<Allocator>& rhs) noexcept
{
    lhs.swap(rhs);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
destroy(list_type& list) noexcept
{
    for(auto it = list.begin();
            it != list.end();)
        destroy(*it++);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
destroy(const_iter it)
{
    auto& e = list_.erase(it);
    destroy(e);
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
destroy(element& e)
{
    auto a = rebind_type{this->get()};
    auto const n =
        (sizeof(element) + e.size() +
            sizeof(align_type) - 1) /
        sizeof(align_type);
    e.~element();
    alloc_traits::deallocate(a,
        reinterpret_cast<align_type*>(&e), n);
}

template<class Allocator>
auto
basic_multi_buffer<Allocator>::
alloc(std::size_t size) ->
    element&
{
    if(size > alloc_traits::max_size(this->get()))
        BOOST_THROW_EXCEPTION(std::length_error(
        "A basic_multi_buffer exceeded the allocator's maximum size"));
    auto a = rebind_type{this->get()};
    auto const p = alloc_traits::allocate(a,
        (sizeof(element) + size + sizeof(align_type) - 1) /
            sizeof(align_type));
    return *(::new(p) element(size));
}

template<class Allocator>
void
basic_multi_buffer<Allocator>::
debug_check() const
{
#ifndef NDEBUG
    BOOST_ASSERT(buffer_bytes(data()) == in_size_);
    if(list_.empty())
    {
        BOOST_ASSERT(in_pos_ == 0);
        BOOST_ASSERT(in_size_ == 0);
        BOOST_ASSERT(out_pos_ == 0);
        BOOST_ASSERT(out_end_ == 0);
        BOOST_ASSERT(out_ == list_.end());
        return;
    }

    auto const& front = list_.front();

    BOOST_ASSERT(in_pos_ < front.size());

    if(out_ == list_.end())
    {
        BOOST_ASSERT(out_pos_ == 0);
        BOOST_ASSERT(out_end_ == 0);
    }
    else
    {
        auto const& out = *out_;
        auto const& back = list_.back();

        BOOST_ASSERT(out_end_ <= back.size());
        BOOST_ASSERT(out_pos_ <  out.size());
        BOOST_ASSERT(&out != &front || out_pos_ >= in_pos_);
        BOOST_ASSERT(&out != &front || out_pos_ - in_pos_ == in_size_);
        BOOST_ASSERT(&out != &back  || out_pos_ <= out_end_);
    }
#endif
}

} // beast
} // boost

#endif
