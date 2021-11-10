/*
   Copyright (c) Marshall Clow 2012-2015.
   Copyright (c) Beman Dawes 2015
   Copyright (c) Glen Joseph Fernandes 2019 (glenjofe@gmail.com)

   Distributed under the Boost Software License, Version 1.0. (See accompanying
   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    For more information, see http://www.boost.org

    Based on the StringRef implementation in LLVM (http://llvm.org) and
    N3422 by Jeffrey Yasskin
        http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3442.html
    Updated July 2015 to reflect the Library Fundamentals TS
        http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4480.html
*/

#ifndef BOOST_STRING_VIEW_HPP
#define BOOST_STRING_VIEW_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/io/ostream_put.hpp>
#include <boost/utility/string_view_fwd.hpp>
#include <boost/throw_exception.hpp>

#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <string>
#include <cstring>
#include <iosfwd>

#if defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || (defined(BOOST_GCC) && ((BOOST_GCC+0) / 100) <= 406)
// GCC 4.6 cannot handle a defaulted function with noexcept specifier
#define BOOST_STRING_VIEW_NO_CXX11_DEFAULTED_NOEXCEPT_FUNCTIONS
#endif

namespace boost {

    namespace detail {
    //  A helper functor because sometimes we don't have lambdas
        template <typename charT, typename traits>
        class string_view_traits_eq {
        public:
            string_view_traits_eq ( charT ch ) : ch_(ch) {}
            bool operator()( charT val ) const { return traits::eq (ch_, val); }
            charT ch_;
            };
        }

    template<typename charT, typename traits>  // traits defaulted in string_view_fwd.hpp
    class basic_string_view {
    public:
      // types
      typedef traits                                traits_type;
      typedef charT                                 value_type;
      typedef charT*                                pointer;
      typedef const charT*                          const_pointer;
      typedef charT&                                reference;
      typedef const charT&                          const_reference;
      typedef const_pointer                         const_iterator; // impl-defined
      typedef const_iterator                        iterator;
      typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
      typedef const_reverse_iterator                reverse_iterator;
      typedef std::size_t                           size_type;
      typedef std::ptrdiff_t                        difference_type;
      static BOOST_CONSTEXPR_OR_CONST size_type     npos = size_type(-1);

      // construct/copy
      BOOST_CONSTEXPR basic_string_view() BOOST_NOEXCEPT
        : ptr_(NULL), len_(0) {}

      // by defaulting these functions, basic_string_ref becomes
      //  trivially copy/move constructible.
      BOOST_CONSTEXPR basic_string_view(const basic_string_view &rhs) BOOST_NOEXCEPT
#ifndef BOOST_STRING_VIEW_NO_CXX11_DEFAULTED_NOEXCEPT_FUNCTIONS
        = default;
#else
        : ptr_(rhs.ptr_), len_(rhs.len_) {}
#endif

      basic_string_view& operator=(const basic_string_view &rhs) BOOST_NOEXCEPT
#ifndef BOOST_STRING_VIEW_NO_CXX11_DEFAULTED_NOEXCEPT_FUNCTIONS
            = default;
#else
        {
        ptr_ = rhs.ptr_;
        len_ = rhs.len_;
        return *this;
        }
#endif

      template<typename Allocator>
        basic_string_view(const std::basic_string<charT, traits, Allocator>& str) BOOST_NOEXCEPT
          : ptr_(str.data()), len_(str.length()) {}

// #if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES) && !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
//       // Constructing a string_view from a temporary string is a bad idea
//       template<typename Allocator>
//         basic_string_view(      std::basic_string<charT, traits, Allocator>&&)
//           = delete;
// #endif

      BOOST_CONSTEXPR basic_string_view(const charT* str)
        : ptr_(str), len_(traits::length(str)) {}

      BOOST_CONSTEXPR basic_string_view(const charT* str, size_type len)
        : ptr_(str), len_(len) {}

        // iterators
        BOOST_CONSTEXPR const_iterator   begin() const BOOST_NOEXCEPT { return ptr_; }
        BOOST_CONSTEXPR const_iterator  cbegin() const BOOST_NOEXCEPT { return ptr_; }
        BOOST_CONSTEXPR const_iterator     end() const BOOST_NOEXCEPT { return ptr_ + len_; }
        BOOST_CONSTEXPR const_iterator    cend() const BOOST_NOEXCEPT { return ptr_ + len_; }
                const_reverse_iterator  rbegin() const BOOST_NOEXCEPT { return const_reverse_iterator(end()); }
                const_reverse_iterator crbegin() const BOOST_NOEXCEPT { return const_reverse_iterator(end()); }
                const_reverse_iterator    rend() const BOOST_NOEXCEPT { return const_reverse_iterator(begin()); }
                const_reverse_iterator   crend() const BOOST_NOEXCEPT { return const_reverse_iterator(begin()); }

        // capacity
        BOOST_CONSTEXPR size_type size()     const BOOST_NOEXCEPT { return len_; }
        BOOST_CONSTEXPR size_type length()   const BOOST_NOEXCEPT { return len_; }
        BOOST_CONSTEXPR size_type max_size() const BOOST_NOEXCEPT { return len_; }
        BOOST_CONSTEXPR bool empty()         const BOOST_NOEXCEPT { return len_ == 0; }

        // element access
        BOOST_CONSTEXPR const_reference operator[](size_type pos) const BOOST_NOEXCEPT { return ptr_[pos]; }

        BOOST_CONSTEXPR const_reference at(size_t pos) const {
            return pos >= len_ ? BOOST_THROW_EXCEPTION(std::out_of_range("boost::string_view::at")), ptr_[0] : ptr_[pos];
            }

        BOOST_CONSTEXPR const_reference front() const                { return ptr_[0]; }
        BOOST_CONSTEXPR const_reference back()  const                { return ptr_[len_-1]; }
        BOOST_CONSTEXPR const_pointer data()    const BOOST_NOEXCEPT { return ptr_; }

        // modifiers
        void clear() BOOST_NOEXCEPT { len_ = 0; }          // Boost extension

        BOOST_CXX14_CONSTEXPR void remove_prefix(size_type n) {
            if ( n > len_ )
                n = len_;
            ptr_ += n;
            len_ -= n;
            }

        BOOST_CXX14_CONSTEXPR void remove_suffix(size_type n) {
            if ( n > len_ )
                n = len_;
            len_ -= n;
            }

        BOOST_CXX14_CONSTEXPR void swap(basic_string_view& s) BOOST_NOEXCEPT {
            std::swap(ptr_, s.ptr_);
            std::swap(len_, s.len_);
            }

        // basic_string_view string operations
#ifndef BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS
        template<typename Allocator>
        explicit operator std::basic_string<charT, traits, Allocator>() const {
            return std::basic_string<charT, traits, Allocator>(begin(), end());
            }
#endif

#ifndef BOOST_NO_CXX11_FUNCTION_TEMPLATE_DEFAULT_ARGS
        template<typename Allocator = std::allocator<charT> >
        std::basic_string<charT, traits, Allocator> to_string(const Allocator& a = Allocator()) const {
            return std::basic_string<charT, traits, Allocator>(begin(), end(), a);
            }
#else
        std::basic_string<charT, traits> to_string() const {
            return std::basic_string<charT, traits>(begin(), end());
            }

        template<typename Allocator>
        std::basic_string<charT, traits, Allocator> to_string(const Allocator& a) const {
            return std::basic_string<charT, traits, Allocator>(begin(), end(), a);
            }
#endif

        size_type copy(charT* s, size_type n, size_type pos=0) const {
            if (pos > size())
                BOOST_THROW_EXCEPTION(std::out_of_range("string_view::copy" ));
            size_type rlen = (std::min)(n, len_ - pos);
            traits_type::copy(s, data() + pos, rlen);
            return rlen;
            }

        BOOST_CXX14_CONSTEXPR basic_string_view substr(size_type pos, size_type n=npos) const {
            if ( pos > size())
                BOOST_THROW_EXCEPTION( std::out_of_range ( "string_view::substr" ) );
            return basic_string_view(data() + pos, (std::min)(size() - pos, n));
            }

        BOOST_CXX14_CONSTEXPR int compare(basic_string_view x) const BOOST_NOEXCEPT {
            const int cmp = traits::compare(ptr_, x.ptr_, (std::min)(len_, x.len_));
            return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
            }

        BOOST_CXX14_CONSTEXPR int compare(size_type pos1, size_type n1, basic_string_view x)
          const BOOST_NOEXCEPT {
            return substr(pos1, n1).compare(x);
            }

        BOOST_CXX14_CONSTEXPR int compare(size_type pos1, size_type n1,
          basic_string_view x, size_type pos2, size_type n2) const {
            return substr(pos1, n1).compare(x.substr(pos2, n2));
            }

        BOOST_CXX14_CONSTEXPR int compare(const charT* x) const {
            return compare(basic_string_view(x));
            }

        BOOST_CXX14_CONSTEXPR int compare(size_type pos1, size_type n1, const charT* x) const {
            return substr(pos1, n1).compare(basic_string_view(x));
            }

        BOOST_CXX14_CONSTEXPR int compare(size_type pos1, size_type n1,
          const charT* x, size_type n2) const {
            return substr(pos1, n1).compare(basic_string_view(x, n2));
            }

        //  Searches
        BOOST_CONSTEXPR bool starts_with(charT c) const BOOST_NOEXCEPT {              // Boost extension
            return !empty() && traits::eq(c, front());
            }

        BOOST_CONSTEXPR bool starts_with(basic_string_view x) const BOOST_NOEXCEPT {  // Boost extension
            return len_ >= x.len_ && traits::compare(ptr_, x.ptr_, x.len_) == 0;
            }

        BOOST_CONSTEXPR bool ends_with(charT c) const BOOST_NOEXCEPT {                // Boost extension
            return !empty() && traits::eq(c, back());
            }

        BOOST_CONSTEXPR bool ends_with(basic_string_view x) const BOOST_NOEXCEPT {    // Boost extension
            return len_ >= x.len_ &&
               traits::compare(ptr_ + len_ - x.len_, x.ptr_, x.len_) == 0;
            }

        //  find
        BOOST_CXX14_CONSTEXPR size_type find(basic_string_view s, size_type pos = 0) const BOOST_NOEXCEPT {
            if (pos > size())
              return npos;
            if (s.empty())
              return pos;
            if (s.size() > size() - pos)
                return npos;
            const charT* cur = ptr_ + pos;
            const charT* last = cend() - s.size() + 1;
            for (; cur != last ; ++cur) {
                cur = traits::find(cur, last - cur, s[0]);
                if (!cur)
                    return npos;
                if (traits::compare(cur, s.cbegin(), s.size()) == 0)
                    return cur - ptr_;
            }
            return npos;
            }
        BOOST_CXX14_CONSTEXPR size_type find(charT c, size_type pos = 0) const BOOST_NOEXCEPT {
            if (pos > size())
              return npos;
            const charT* ret_ptr = traits::find(ptr_ + pos, len_ - pos, c);
            if (ret_ptr)
              return ret_ptr - ptr_;
            return npos;
            }
        BOOST_CXX14_CONSTEXPR size_type find(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return find(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type find(const charT* s, size_type pos = 0) const BOOST_NOEXCEPT
            { return find(basic_string_view(s), pos); }

        //  rfind
        BOOST_CXX14_CONSTEXPR size_type rfind(basic_string_view s, size_type pos = npos) const BOOST_NOEXCEPT {
            if (len_ < s.len_)
              return npos;
            if (pos > len_ - s.len_)
              pos = len_ - s.len_;
            if (s.len_ == 0u)     // an empty string is always found
              return pos;
            for (const charT* cur = ptr_ + pos; ; --cur) {
                if (traits::compare(cur, s.ptr_, s.len_) == 0)
                  return cur - ptr_;
                if (cur == ptr_)
                  return npos;
                };
            }
        BOOST_CXX14_CONSTEXPR size_type rfind(charT c, size_type pos = npos) const BOOST_NOEXCEPT
            { return rfind(basic_string_view(&c, 1), pos); }
        BOOST_CXX14_CONSTEXPR size_type rfind(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return rfind(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type rfind(const charT* s, size_type pos = npos) const BOOST_NOEXCEPT
            { return rfind(basic_string_view(s), pos); }

        //  find_first_of
        BOOST_CXX14_CONSTEXPR size_type find_first_of(basic_string_view s, size_type pos = 0) const BOOST_NOEXCEPT {
            if (pos >= len_ || s.len_ == 0)
              return npos;
            const_iterator iter = std::find_first_of
                (this->cbegin () + pos, this->cend (), s.cbegin (), s.cend (), traits::eq);
            return iter == this->cend () ? npos : std::distance ( this->cbegin (), iter );
            }
        BOOST_CXX14_CONSTEXPR size_type find_first_of(charT c, size_type pos = 0) const BOOST_NOEXCEPT
            { return find(c, pos); }
        BOOST_CXX14_CONSTEXPR size_type find_first_of(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return find_first_of(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_first_of(const charT* s, size_type pos = 0) const BOOST_NOEXCEPT
            { return find_first_of(basic_string_view(s), pos); }

        //  find_last_of
        BOOST_CXX14_CONSTEXPR size_type find_last_of(basic_string_view s, size_type pos = npos) const BOOST_NOEXCEPT {
            if (s.len_ == 0u)
              return npos;
            if (pos >= len_)
              pos = 0;
            else
              pos = len_ - (pos+1);
            const_reverse_iterator iter = std::find_first_of
                ( this->crbegin () + pos, this->crend (), s.cbegin (), s.cend (), traits::eq );
            return iter == this->crend () ? npos : reverse_distance ( this->crbegin (), iter);
            }
        BOOST_CXX14_CONSTEXPR size_type find_last_of(charT c, size_type pos = npos) const BOOST_NOEXCEPT
            { return find_last_of(basic_string_view(&c, 1), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_last_of(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return find_last_of(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_last_of(const charT* s, size_type pos = npos) const BOOST_NOEXCEPT
            { return find_last_of(basic_string_view(s), pos); }

        //  find_first_not_of
        BOOST_CXX14_CONSTEXPR size_type find_first_not_of(basic_string_view s, size_type pos = 0) const BOOST_NOEXCEPT {
            if (pos >= len_)
              return npos;
            if (s.len_ == 0)
              return pos;
            const_iterator iter = find_not_of ( this->cbegin () + pos, this->cend (), s );
            return iter == this->cend () ? npos : std::distance ( this->cbegin (), iter );
            }
        BOOST_CXX14_CONSTEXPR size_type find_first_not_of(charT c, size_type pos = 0) const BOOST_NOEXCEPT
            { return find_first_not_of(basic_string_view(&c, 1), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_first_not_of(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return find_first_not_of(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_first_not_of(const charT* s, size_type pos = 0) const BOOST_NOEXCEPT
            { return find_first_not_of(basic_string_view(s), pos); }

        //  find_last_not_of
        BOOST_CXX14_CONSTEXPR size_type find_last_not_of(basic_string_view s, size_type pos = npos) const BOOST_NOEXCEPT {
            if (pos >= len_)
              pos = len_ - 1;
            if (s.len_ == 0u)
              return pos;
            pos = len_ - (pos+1);
            const_reverse_iterator iter = find_not_of ( this->crbegin () + pos, this->crend (), s );
            return iter == this->crend () ? npos : reverse_distance ( this->crbegin (), iter );
            }
        BOOST_CXX14_CONSTEXPR size_type find_last_not_of(charT c, size_type pos = npos) const BOOST_NOEXCEPT
            { return find_last_not_of(basic_string_view(&c, 1), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_last_not_of(const charT* s, size_type pos, size_type n) const BOOST_NOEXCEPT
            { return find_last_not_of(basic_string_view(s, n), pos); }
        BOOST_CXX14_CONSTEXPR size_type find_last_not_of(const charT* s, size_type pos = npos) const BOOST_NOEXCEPT
            { return find_last_not_of(basic_string_view(s), pos); }

    private:
        template <typename r_iter>
        size_type reverse_distance(r_iter first, r_iter last) const BOOST_NOEXCEPT {
        // Portability note here: std::distance is not NOEXCEPT, but calling it with a string_view::reverse_iterator will not throw.
            return len_ - 1 - std::distance ( first, last );
            }

        template <typename Iterator>
        Iterator find_not_of(Iterator first, Iterator last, basic_string_view s) const BOOST_NOEXCEPT {
            for (; first != last ; ++first)
                if ( 0 == traits::find(s.ptr_, s.len_, *first))
                    return first;
            return last;
            }

        const charT *ptr_;
        std::size_t len_;
        };


//  Comparison operators
//  Equality
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator==(basic_string_view<charT, traits> x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        if (x.size () != y.size ()) return false;
        return x.compare(y) == 0;
        }

//  Inequality
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator!=(basic_string_view<charT, traits> x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        if ( x.size () != y.size ()) return true;
        return x.compare(y) != 0;
        }

//  Less than
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<(basic_string_view<charT, traits> x,
                          basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return x.compare(y) < 0;
        }

//  Greater than
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>(basic_string_view<charT, traits> x,
                          basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return x.compare(y) > 0;
        }

//  Less than or equal to
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<=(basic_string_view<charT, traits> x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return x.compare(y) <= 0;
        }

//  Greater than or equal to
    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>=(basic_string_view<charT, traits> x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return x.compare(y) >= 0;
        }

// "sufficient additional overloads of comparison functions"
    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator==(basic_string_view<charT, traits> x,
                     const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x == basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator==(const std::basic_string<charT, traits, Allocator> & x,
                                 basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) == y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator==(basic_string_view<charT, traits> x,
                                              const charT * y) BOOST_NOEXCEPT {
        return x == basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator==(const charT * x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) == y;
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator!=(basic_string_view<charT, traits> x,
                     const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x != basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator!=(const std::basic_string<charT, traits, Allocator> & x,
                                 basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) != y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator!=(basic_string_view<charT, traits> x,
                           const charT * y) BOOST_NOEXCEPT {
        return x != basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator!=(const charT * x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) != y;
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator<(basic_string_view<charT, traits> x,
                    const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x < basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator<(const std::basic_string<charT, traits, Allocator> & x,
                                basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) < y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<(basic_string_view<charT, traits> x,
                          const charT * y) BOOST_NOEXCEPT {
        return x < basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<(const charT * x,
                          basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) < y;
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator>(basic_string_view<charT, traits> x,
                    const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x > basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator>(const std::basic_string<charT, traits, Allocator> & x,
                                basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) > y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>(basic_string_view<charT, traits> x,
                          const charT * y) BOOST_NOEXCEPT {
        return x > basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>(const charT * x,
                          basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) > y;
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator<=(basic_string_view<charT, traits> x,
                     const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x <= basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator<=(const std::basic_string<charT, traits, Allocator> & x,
                                 basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) <= y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<=(basic_string_view<charT, traits> x,
                           const charT * y) BOOST_NOEXCEPT {
        return x <= basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator<=(const charT * x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) <= y;
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator>=(basic_string_view<charT, traits> x,
                     const std::basic_string<charT, traits, Allocator> & y) BOOST_NOEXCEPT {
        return x >= basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits, typename Allocator>
    inline BOOST_CXX14_CONSTEXPR bool operator>=(const std::basic_string<charT, traits, Allocator> & x,
                                 basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) >= y;
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>=(basic_string_view<charT, traits> x,
                           const charT * y) BOOST_NOEXCEPT {
        return x >= basic_string_view<charT, traits>(y);
        }

    template<typename charT, typename traits>
    inline BOOST_CXX14_CONSTEXPR bool operator>=(const charT * x,
                           basic_string_view<charT, traits> y) BOOST_NOEXCEPT {
        return basic_string_view<charT, traits>(x) >= y;
        }

    // Inserter
    template<class charT, class traits>
    inline std::basic_ostream<charT, traits>&
    operator<<(std::basic_ostream<charT, traits>& os,
      const basic_string_view<charT,traits>& str) {
        return boost::io::ostream_put(os, str.data(), str.size());
        }

#if 0
    // numeric conversions
    //
    //  These are short-term implementations.
    //  In a production environment, I would rather avoid the copying.
    //
    inline int stoi (string_view str, size_t* idx=0, int base=10) {
        return std::stoi ( std::string(str), idx, base );
        }

    inline long stol (string_view str, size_t* idx=0, int base=10) {
        return std::stol ( std::string(str), idx, base );
        }

    inline unsigned long stoul (string_view str, size_t* idx=0, int base=10) {
        return std::stoul ( std::string(str), idx, base );
        }

    inline long long stoll (string_view str, size_t* idx=0, int base=10) {
        return std::stoll ( std::string(str), idx, base );
        }

    inline unsigned long long stoull (string_view str, size_t* idx=0, int base=10) {
        return std::stoull ( std::string(str), idx, base );
        }

    inline float stof (string_view str, size_t* idx=0) {
        return std::stof ( std::string(str), idx );
        }

    inline double stod (string_view str, size_t* idx=0) {
        return std::stod ( std::string(str), idx );
        }

    inline long double stold (string_view str, size_t* idx=0)  {
        return std::stold ( std::string(str), idx );
        }

    inline int  stoi (wstring_view str, size_t* idx=0, int base=10) {
        return std::stoi ( std::wstring(str), idx, base );
        }

    inline long stol (wstring_view str, size_t* idx=0, int base=10) {
        return std::stol ( std::wstring(str), idx, base );
        }

    inline unsigned long stoul (wstring_view str, size_t* idx=0, int base=10) {
        return std::stoul ( std::wstring(str), idx, base );
        }

    inline long long stoll (wstring_view str, size_t* idx=0, int base=10) {
        return std::stoll ( std::wstring(str), idx, base );
        }

    inline unsigned long long stoull (wstring_view str, size_t* idx=0, int base=10) {
        return std::stoull ( std::wstring(str), idx, base );
        }

    inline float  stof (wstring_view str, size_t* idx=0) {
        return std::stof ( std::wstring(str), idx );
        }

    inline double stod (wstring_view str, size_t* idx=0) {
        return std::stod ( std::wstring(str), idx );
        }

    inline long double stold (wstring_view str, size_t* idx=0) {
        return std::stold ( std::wstring(str), idx );
        }
#endif

    // Forward declaration of Boost.ContainerHash function
    template <class It> std::size_t hash_range(It, It);

    template <class charT, class traits>
    std::size_t hash_value(basic_string_view<charT, traits> s) {
        return boost::hash_range(s.begin(), s.end());
        }
}

#if 0
namespace std {
    // Hashing
    template<> struct hash<boost::string_view>;
    template<> struct hash<boost::u16string_view>;
    template<> struct hash<boost::u32string_view>;
    template<> struct hash<boost::wstring_view>;
}
#endif

#endif
