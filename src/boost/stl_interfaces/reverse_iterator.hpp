// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_REVERSE_ITERATOR_HPP
#define BOOST_STL_INTERFACES_REVERSE_ITERATOR_HPP

#include <boost/stl_interfaces/iterator_interface.hpp>


namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V1 {

    namespace v1_dtl {
        template<typename Iter>
        constexpr auto ce_dist(Iter f, Iter l, std::random_access_iterator_tag)
            -> decltype(l - f)
        {
            return l - f;
        }
        template<typename Iter, typename Tag>
        constexpr auto ce_dist(Iter f, Iter l, Tag)
            -> decltype(std::distance(f, l))
        {
            decltype(std::distance(f, l)) retval = 0;
            for (; f != l; ++f) {
                ++retval;
            }
            return retval;
        }

        template<typename Iter>
        constexpr Iter ce_prev(Iter it)
        {
            return --it;
        }

        template<typename Iter, typename Offset>
        constexpr void
        ce_adv(Iter & f, Offset n, std::random_access_iterator_tag)
        {
            f += n;
        }
        template<typename Iter, typename Offset, typename Tag>
        constexpr void ce_adv(Iter & f, Offset n, Tag)
        {
            if (0 < n) {
                for (Offset i = 0; i < n; ++i) {
                    ++f;
                }
            } else {
                for (Offset i = 0; i < -n; ++i) {
                    --f;
                }
            }
        }
    }

    /** This type is very similar to the C++20 version of
        `std::reverse_iterator`; it is `constexpr`-, `noexcept`-, and
        proxy-friendly. */
    template<typename BidiIter>
    struct reverse_iterator
        : iterator_interface<
              reverse_iterator<BidiIter>,
#if BOOST_STL_INTERFACES_USE_CONCEPTS
              typename boost::stl_interfaces::v2::v2_dtl::iter_concept_t<
                  BidiIter>,
#else
              typename std::iterator_traits<BidiIter>::iterator_category,
#endif
              typename std::iterator_traits<BidiIter>::value_type,
              typename std::iterator_traits<BidiIter>::reference,
              typename std::iterator_traits<BidiIter>::pointer,
              typename std::iterator_traits<BidiIter>::difference_type>
    {
        constexpr reverse_iterator() noexcept(noexcept(BidiIter())) : it_() {}
        constexpr reverse_iterator(BidiIter it) noexcept(
            noexcept(BidiIter(it))) :
            it_(it)
        {}
        template<
            typename BidiIter2,
            typename E = std::enable_if_t<
                std::is_convertible<BidiIter2, BidiIter>::value>>
        reverse_iterator(reverse_iterator<BidiIter2> const & it) : it_(it.it_)
        {}

        friend BOOST_STL_INTERFACES_HIDDEN_FRIEND_CONSTEXPR auto
        operator-(reverse_iterator lhs, reverse_iterator rhs) noexcept(
            noexcept(v1_dtl::ce_dist(
                lhs.it_,
                rhs.it_,
                typename std::iterator_traits<BidiIter>::iterator_category{})))
        {
            return -v1_dtl::ce_dist(
                rhs.it_,
                lhs.it_,
                typename std::iterator_traits<BidiIter>::iterator_category{});
        }

        constexpr typename std::iterator_traits<BidiIter>::reference
        operator*() const noexcept(
            noexcept(std::prev(v1_dtl::ce_prev(std::declval<BidiIter &>()))))
        {
            return *v1_dtl::ce_prev(it_);
        }

        constexpr reverse_iterator & operator+=(
            typename std::iterator_traits<BidiIter>::difference_type
                n) noexcept(noexcept(v1_dtl::
                                         ce_adv(
                                             std::declval<BidiIter &>(),
                                             -n,
                                             typename std::iterator_traits<
                                                 BidiIter>::
                                                 iterator_category{})))
        {
            v1_dtl::ce_adv(
                it_,
                -n,
                typename std::iterator_traits<BidiIter>::iterator_category{});
            return *this;
        }

        constexpr BidiIter base() const noexcept { return it_; }

    private:
        friend access;
        constexpr BidiIter & base_reference() noexcept { return it_; }
        constexpr BidiIter const & base_reference() const noexcept
        {
            return it_;
        }

        template<typename BidiIter2>
        friend struct reverse_iterator;

        BidiIter it_;
    };

    template<typename BidiIter>
    constexpr auto operator==(
        reverse_iterator<BidiIter> lhs,
        reverse_iterator<BidiIter>
            rhs) noexcept(noexcept(lhs.base() == rhs.base()))
        -> decltype(rhs.base() == lhs.base())
    {
        return lhs.base() == rhs.base();
    }

    template<typename BidiIter1, typename BidiIter2>
    constexpr auto operator==(
        reverse_iterator<BidiIter1> lhs,
        reverse_iterator<BidiIter2>
            rhs) noexcept(noexcept(lhs.base() == rhs.base()))
        -> decltype(rhs.base() == lhs.base())
    {
        return lhs.base() == rhs.base();
    }

    /** Makes a `reverse_iterator<BidiIter>` from an iterator of type
        `Bidiiter`. */
    template<typename BidiIter>
    auto make_reverse_iterator(BidiIter it)
    {
        return reverse_iterator<BidiIter>(it);
    }

}}}


#if defined(BOOST_STL_INTERFACES_DOXYGEN) || BOOST_STL_INTERFACES_USE_CONCEPTS

namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V2 {

    /** A template alias for `std::reverse_iterator`.  This only exists to
        make migration from Boost.STLInterfaces to C++20 easier; switch to the
        one in `std` as soon as you can. */
    template<typename BidiIter>
    using reverse_iterator = std::reverse_iterator<BidiIter>;


    /** Makes a `reverse_iterator<BidiIter>` from an iterator of type
        `Bidiiter`.  This only exists to make migration from
        Boost.STLInterfaces to C++20 easier; switch to the one in `std` as
        soon as you can. */
    template<typename BidiIter>
    auto make_reverse_iterator(BidiIter it)
    {
        return reverse_iterator<BidiIter>(it);
    }

}}}

#endif

#endif
