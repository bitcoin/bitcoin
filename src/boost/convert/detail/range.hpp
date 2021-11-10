// Copyright (c) 2009-2020 Vladimir Batov.
// Use, modification and distribution are subject to the Boost Software License,
// Version 1.0. See http://www.boost.org/LICENSE_1_0.txt.

#ifndef BOOST_CONVERT_DETAIL_RANGE_HPP
#define BOOST_CONVERT_DETAIL_RANGE_HPP

#include <boost/convert/detail/has_member.hpp>
#include <boost/convert/detail/char.hpp>
#include <boost/range/iterator.hpp>

namespace boost { namespace cnv
{
    namespace detail
    {
        template<typename T, bool is_class> struct is_range : std::false_type {};

        template<typename T> struct is_range<T, /*is_class=*/true>
        {
            BOOST_DECLARE_HAS_MEMBER(has_begin, begin);
            BOOST_DECLARE_HAS_MEMBER(  has_end, end);

            static bool BOOST_CONSTEXPR_OR_CONST value = has_begin<T>::value && has_end<T>::value;
        };
    }
    template<typename T> struct is_range : detail::is_range<typename boost::remove_const<T>::type, boost::is_class<T>::value> {};
    template<typename T, typename enable =void> struct range;
    template<typename T, typename enable =void> struct iterator;

    template<typename T>
    struct iterator<T, typename std::enable_if<is_range<T>::value>::type>
    {
        using       type = typename boost::range_iterator<T>::type;
        using const_type = typename boost::range_iterator<T const>::type;
        using value_type = typename boost::iterator_value<type>::type;
    };
    template<typename T>
    struct iterator<T*, void>
    {
        using value_type = typename boost::remove_const<T>::type;
        using       type = T*;
        using const_type = value_type const*;
    };
    template<typename T>
    struct range_base
    {
        using     value_type = typename cnv::iterator<T>::value_type;
        using       iterator = typename cnv::iterator<T>::type;
        using const_iterator = typename cnv::iterator<T>::const_type;
        using    sentry_type = const_iterator;

        iterator       begin () { return begin_; }
        const_iterator begin () const { return begin_; }
        void      operator++ () { ++begin_; }
//      void      operator-- () { --end_; }

        protected:

        range_base (iterator b, iterator e) : begin_(b), end_(e) {}

        iterator       begin_;
        iterator mutable end_;
    };

    template<typename T>
    struct range<T, typename std::enable_if<is_range<T>::value>::type> : public range_base<T>
    {
        using      this_type = range;
        using      base_type = range_base<T>;
        using       iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;
        using    sentry_type = const_iterator;

        range (T& r) : base_type(r.begin(), r.end()) {}

        iterator       end ()       { return base_type::end_; }
        const_iterator end () const { return base_type::end_; }
        sentry_type sentry () const { return base_type::end_; }
        std::size_t   size () const { return base_type::end_ - base_type::begin_; }
        bool         empty () const { return base_type::begin_ == base_type::end_; }
    };

    template<typename T>
    struct range<T*, typename std::enable_if<cnv::is_char<T>::value>::type> : public range_base<T*>
    {
        using      this_type = range;
        using      base_type = range_base<T*>;
        using     value_type = typename boost::remove_const<T>::type;
        using       iterator = T*;
        using const_iterator = value_type const*;

        struct sentry_type
        {
            friend bool operator!=(iterator it, sentry_type) { return !!*it; }
        };

        range (iterator b, iterator e =0) : base_type(b, e) {}

        iterator       end ()       { return base_type::end_ ? base_type::end_ : (base_type::end_ = base_type::begin_ + size()); }
        const_iterator end () const { return base_type::end_ ? base_type::end_ : (base_type::end_ = base_type::begin_ + size()); }
        sentry_type sentry () const { return sentry_type(); }
        std::size_t   size () const { return std::char_traits<value_type>::length(base_type::begin_); }
        bool         empty () const { return !*base_type::begin_; }
    };
    template<typename T>
    struct range<T* const, void> : public range<T*>
    {
        range (T* b, T* e =0) : range<T*>(b, e) {}
    };
    template <typename T, std::size_t N>
    struct range<T [N], void> : public range<T*>
    {
        range (T* b, T* e =0) : range<T*>(b, e) {}
    };
}}

#endif // BOOST_CONVERT_DETAIL_RANGE_HPP
