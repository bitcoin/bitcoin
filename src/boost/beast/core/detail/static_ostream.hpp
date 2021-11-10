//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_STATIC_OSTREAM_HPP
#define BOOST_BEAST_DETAIL_STATIC_OSTREAM_HPP

#include <locale>
#include <ostream>
#include <streambuf>

namespace boost {
namespace beast {
namespace detail {

// http://www.mr-edd.co.uk/blog/beginners_guide_streambuf

class static_ostream_buffer
    : public std::basic_streambuf<char>
{
    using CharT = char;
    using Traits = std::char_traits<CharT>;
    using int_type = typename
        std::basic_streambuf<CharT, Traits>::int_type;
    using traits_type = typename
        std::basic_streambuf<CharT, Traits>::traits_type;

    char* data_;
    std::size_t size_;
    std::size_t len_ = 0;
    std::string s_;

public:
    static_ostream_buffer(static_ostream_buffer&&) = delete;
    static_ostream_buffer(static_ostream_buffer const&) = delete;

    static_ostream_buffer(char* data, std::size_t size)
        : data_(data)
        , size_(size)
    {
        this->setp(data_, data_ + size - 1);
    }

    ~static_ostream_buffer() noexcept
    {
    }

    string_view
    str() const
    {
        if(! s_.empty())
            return {s_.data(), len_};
        return {data_, len_};
    }

    int_type
    overflow(int_type ch) override
    {
        if(! Traits::eq_int_type(ch, Traits::eof()))
        {
            Traits::assign(*this->pptr(),
                static_cast<CharT>(ch));
            flush(1);
            prepare();
            return ch;
        }
        flush();
        return traits_type::eof();
    }

    int
    sync() override
    {
        flush();
        prepare();
        return 0;
    }

private:
    void
    prepare()
    {
        static auto const growth_factor = 1.5;

        if(len_ < size_ - 1)
        {
            this->setp(
                data_ + len_, data_ + size_ - 2);
            return;
        }
        if(s_.empty())
        {
            s_.resize(static_cast<std::size_t>(
                growth_factor * len_));
            Traits::copy(&s_[0], data_, len_);
        }
        else
        {
            s_.resize(static_cast<std::size_t>(
                growth_factor * len_));
        }
        this->setp(&s_[len_], &s_[len_] +
            s_.size() - len_ - 1);
    }

    void
    flush(int extra = 0)
    {
        len_ += static_cast<std::size_t>(
            this->pptr() - this->pbase() + extra);
    }
};

class static_ostream : public std::basic_ostream<char>
{
    static_ostream_buffer osb_;

public:
    static_ostream(char* data, std::size_t size)
        : std::basic_ostream<char>(&this->osb_)
        , osb_(data, size)
    {
        imbue(std::locale::classic());
    }

    string_view
    str() const
    {
        return osb_.str();
    }
};

} // detail
} // beast
} // boost

#endif
