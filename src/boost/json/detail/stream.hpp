//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_STREAM_HPP
#define BOOST_JSON_DETAIL_STREAM_HPP

BOOST_JSON_NS_BEGIN
namespace detail {

class const_stream
{
    friend class local_const_stream;

    char const* p_;
    char const* end_;

public:
    const_stream() = default;
    const_stream(
        const_stream const&) = default;

    const_stream(
        char const* data,
        std::size_t size) noexcept
        : p_(data)
        , end_(data + size)
    {
    }

    size_t
    used(char const* begin) const noexcept
    {
        return static_cast<
            size_t>(p_ - begin);
    }

    size_t
    remain() const noexcept
    {
        return end_ - p_;
    }

    char const*
    data() const noexcept
    {
        return p_;
    }

    operator bool() const noexcept
    {
        return p_ < end_;
    }

    // unchecked
    char
    operator*() const noexcept
    {
        BOOST_ASSERT(p_ < end_);
        return *p_;
    }

    // unchecked
    const_stream&
    operator++() noexcept
    {
        BOOST_ASSERT(p_ < end_);
        ++p_;
        return *this;
    }

    void
    skip(std::size_t n) noexcept
    {
        BOOST_ASSERT(n <= remain());
        p_ += n;
    }

    void
    skip_to(const char* p) noexcept
    {
        BOOST_ASSERT(p <= end_ && p >= p_);
        p_ = p;
    }
};

class local_const_stream
    : public const_stream
{
    const_stream& src_;

public:
    explicit
    local_const_stream(
        const_stream& src) noexcept
        : const_stream(src)
        , src_(src)
    {
    }

    ~local_const_stream()
    {
        src_.p_ = p_;
    }

    void
    clip(std::size_t n) noexcept
    {
        if(static_cast<std::size_t>(
            src_.end_ - p_) > n)
            end_ = p_ + n;
        else
            end_ = src_.end_;
    }
};

class const_stream_wrapper
{
    const char*& p_;
    const char* const end_;

    friend class clipped_const_stream;
public:
    const_stream_wrapper(
        const char*& p,
        const char* end)
        : p_(p)
        , end_(end)
    {
    }

    void operator++() noexcept
    {
        ++p_;
    }

    void operator+=(std::size_t n) noexcept
    {
        p_ += n;
    }

    void operator=(const char* p) noexcept
    {
        p_ = p;
    }

    char operator*() const noexcept
    {
        return *p_;
    }

    operator bool() const noexcept
    {
        return p_ < end_;
    }

    const char* begin() const noexcept
    {
        return p_;
    }

    const char* end() const noexcept
    {
        return end_;
    }

    std::size_t remain() const noexcept
    {
        return end_ - p_;
    }

    std::size_t remain(const char* p) const noexcept
    {
        return end_ - p;
    }

    std::size_t used(const char* p) const noexcept
    {
        return p_ - p;
    }
};

class clipped_const_stream
    : public const_stream_wrapper
{
    const char* clip_;

public:
    clipped_const_stream(
        const char*& p,
        const char* end)
        : const_stream_wrapper(p, end)
        , clip_(end)
    {
    }

    void operator=(const char* p)
    {
        p_ = p;
    }

    const char* end() const noexcept
    {
        return clip_;
    }

    operator bool() const noexcept
    {
        return p_ < clip_;
    }

    std::size_t remain() const noexcept
    {
        return clip_ - p_;
    }

    std::size_t remain(const char* p) const noexcept
    {
        return clip_ - p;
    }

    void
    clip(std::size_t n) noexcept
    {
        if(static_cast<std::size_t>(
            end_ - p_) > n)
            clip_ = p_ + n;
        else
            clip_ = end_;
    }
};

//--------------------------------------

class stream
{
    friend class local_stream;

    char* p_;
    char* end_;

public:
    stream(
        stream const&) = default;

    stream(
        char* data,
        std::size_t size) noexcept
        : p_(data)
        , end_(data + size)
    {
    }

    size_t
    used(char* begin) const noexcept
    {
        return static_cast<
            size_t>(p_ - begin);
    }

    size_t
    remain() const noexcept
    {
        return end_ - p_;
    }

    char*
    data() noexcept
    {
        return p_;
    }

    operator bool() const noexcept
    {
        return p_ < end_;
    }

    // unchecked
    char&
    operator*() noexcept
    {
        BOOST_ASSERT(p_ < end_);
        return *p_;
    }

    // unchecked
    stream&
    operator++() noexcept
    {
        BOOST_ASSERT(p_ < end_);
        ++p_;
        return *this;
    }

    // unchecked
    void
    append(
        char const* src,
        std::size_t n) noexcept
    {
        BOOST_ASSERT(remain() >= n);
        std::memcpy(p_, src, n);
        p_ += n;
    }

    // unchecked
    void
    append(char c) noexcept
    {
        BOOST_ASSERT(p_ < end_);
        *p_++ = c;
    }

    void
    advance(std::size_t n) noexcept
    {
        BOOST_ASSERT(remain() >= n);
        p_ += n;
    }
};

class local_stream
    : public stream
{
    stream& src_;

public:
    explicit
    local_stream(
        stream& src)
        : stream(src)
        , src_(src)
    {
    }

    ~local_stream()
    {
        src_.p_ = p_;
    }
};

} // detail
BOOST_JSON_NS_END

#endif
