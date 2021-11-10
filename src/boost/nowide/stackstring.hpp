//
//  Copyright (c) 2012 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_NOWIDE_STACKSTRING_HPP_INCLUDED
#define BOOST_NOWIDE_STACKSTRING_HPP_INCLUDED

#include <boost/nowide/convert.hpp>
#include <boost/nowide/utf/utf.hpp>
#include <cassert>
#include <cstring>

namespace boost {
namespace nowide {

    ///
    /// \brief A class that allows to create a temporary wide or narrow UTF strings from
    /// wide or narrow UTF source.
    ///
    /// It uses a stack buffer if the string is short enough
    /// otherwise allocates a buffer on the heap.
    ///
    /// Invalid UTF characters are replaced by the substitution character, see #BOOST_NOWIDE_REPLACEMENT_CHARACTER
    ///
    /// If a NULL pointer is passed to the constructor or convert method, NULL will be returned by c_str.
    /// Similarily a default constructed stackstring will return NULL on calling c_str.
    ///
    template<typename CharOut = wchar_t, typename CharIn = char, size_t BufferSize = 256>
    class basic_stackstring
    {
    public:
        /// Size of the stack buffer
        static const size_t buffer_size = BufferSize;
        /// Type of the output character (converted to)
        using output_char = CharOut;
        /// Type of the input character (converted from)
        using input_char = CharIn;

        /// Creates a NULL stackstring
        basic_stackstring() : data_(NULL)
        {
            buffer_[0] = 0;
        }
        /// Convert the NULL terminated string input and store in internal buffer
        /// If input is NULL, nothing will be stored
        explicit basic_stackstring(const input_char* input) : data_(NULL)
        {
            convert(input);
        }
        /// Convert the sequence [begin, end) and store in internal buffer
        /// If begin is NULL, nothing will be stored
        basic_stackstring(const input_char* begin, const input_char* end) : data_(NULL)
        {
            convert(begin, end);
        }
        /// Copy construct from other
        basic_stackstring(const basic_stackstring& other) : data_(NULL)
        {
            *this = other;
        }
        /// Copy assign from other
        basic_stackstring& operator=(const basic_stackstring& other)
        {
            if(this != &other)
            {
                clear();
                const size_t len = other.length();
                if(other.uses_stack_memory())
                    data_ = buffer_;
                else if(other.data_)
                    data_ = new output_char[len + 1];
                else
                {
                    data_ = NULL;
                    return *this;
                }
                std::memcpy(data_, other.data_, sizeof(output_char) * (len + 1));
            }
            return *this;
        }

        ~basic_stackstring()
        {
            clear();
        }

        /// Convert the NULL terminated string input and store in internal buffer
        /// If input is NULL, the current buffer will be reset to NULL
        output_char* convert(const input_char* input)
        {
            if(input)
                return convert(input, input + utf::strlen(input));
            clear();
            return get();
        }
        /// Convert the sequence [begin, end) and store in internal buffer
        /// If begin is NULL, the current buffer will be reset to NULL
        output_char* convert(const input_char* begin, const input_char* end)
        {
            clear();

            if(begin)
            {
                const size_t input_len = end - begin;
                // Minimum size required: 1 output char per input char + trailing NULL
                const size_t min_output_size = input_len + 1;
                // If there is a chance the converted string fits on stack, try it
                if(min_output_size <= buffer_size && utf::convert_buffer(buffer_, buffer_size, begin, end))
                    data_ = buffer_;
                else
                {
                    // Fallback: Allocate a buffer that is surely large enough on heap
                    // Max size: Every input char is transcoded to the output char with maximum with + trailing NULL
                    const size_t max_output_size = input_len * utf::utf_traits<output_char>::max_width + 1;
                    data_ = new output_char[max_output_size];
                    const bool success = utf::convert_buffer(data_, max_output_size, begin, end) == data_;
                    assert(success);
                    (void)success;
                }
            }
            return get();
        }
        /// Return the converted, NULL-terminated string or NULL if no string was converted
        output_char* get()
        {
            return data_;
        }
        /// Return the converted, NULL-terminated string or NULL if no string was converted
        const output_char* get() const
        {
            return data_;
        }
        /// Reset the internal buffer to NULL
        void clear()
        {
            if(!uses_stack_memory())
                delete[] data_;
            data_ = NULL;
        }
        /// Swap lhs with rhs
        friend void swap(basic_stackstring& lhs, basic_stackstring& rhs)
        {
            if(lhs.uses_stack_memory())
            {
                if(rhs.uses_stack_memory())
                {
                    for(size_t i = 0; i < buffer_size; i++)
                        std::swap(lhs.buffer_[i], rhs.buffer_[i]);
                } else
                {
                    lhs.data_ = rhs.data_;
                    rhs.data_ = rhs.buffer_;
                    for(size_t i = 0; i < buffer_size; i++)
                        rhs.buffer_[i] = lhs.buffer_[i];
                }
            } else if(rhs.uses_stack_memory())
            {
                rhs.data_ = lhs.data_;
                lhs.data_ = lhs.buffer_;
                for(size_t i = 0; i < buffer_size; i++)
                    lhs.buffer_[i] = rhs.buffer_[i];
            } else
                std::swap(lhs.data_, rhs.data_);
        }

    protected:
        /// True if the stack memory is used
        bool uses_stack_memory() const
        {
            return data_ == buffer_;
        }
        /// Return the current length of the string excluding the NULL terminator
        /// If NULL is stored returns NULL
        size_t length() const
        {
            if(!data_)
                return 0;
            size_t len = 0;
            while(data_[len])
                len++;
            return len;
        }

    private:
        output_char buffer_[buffer_size];
        output_char* data_;
    }; // basic_stackstring

    ///
    /// Convenience typedef
    ///
    using wstackstring = basic_stackstring<wchar_t, char, 256>;
    ///
    /// Convenience typedef
    ///
    using stackstring = basic_stackstring<char, wchar_t, 256>;
    ///
    /// Convenience typedef
    ///
    using wshort_stackstring = basic_stackstring<wchar_t, char, 16>;
    ///
    /// Convenience typedef
    ///
    using short_stackstring = basic_stackstring<char, wchar_t, 16>;

} // namespace nowide
} // namespace boost

#endif
