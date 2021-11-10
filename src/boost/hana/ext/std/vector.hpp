/*!
@file
Adapts `std::vector` for use with Hana.

@copyright Louis Dionne 2013-2017
@copyright Gonzalo Brito Gadeschi 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_HANA_EXT_STD_VECTOR_HPP
#define BOOST_HANA_EXT_STD_VECTOR_HPP

#include <boost/hana/config.hpp>
#include <boost/hana/equal.hpp>
#include <boost/hana/fwd/core/tag_of.hpp>
#include <boost/hana/less.hpp>

#include <algorithm>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>


BOOST_HANA_NAMESPACE_BEGIN
    namespace ext { namespace std { struct vector_tag; }}

    template <typename T, typename Allocator>
    struct tag_of<std::vector<T, Allocator>> {
        using type = ext::std::vector_tag;
    };

    //////////////////////////////////////////////////////////////////////////
    // Comparable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct equal_impl<ext::std::vector_tag, ext::std::vector_tag> {
        template <typename T1, typename A1, typename T2, typename A2>
        static bool apply(std::vector<T1, A1> const& v1,
                          std::vector<T2, A2> const& v2)
        {
            return std::equal(begin(v1), end(v1),
                              begin(v2), end(v2),
                              hana::equal);
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // Orderable
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct less_impl<ext::std::vector_tag, ext::std::vector_tag> {
        template <typename T1, typename A1, typename T2, typename A2>
        static bool apply(std::vector<T1, A1> const& v1,
                          std::vector<T2, A2> const& v2)
        {
            return std::lexicographical_compare(begin(v1), end(v1),
                                                begin(v2), end(v2),
                                                hana::less);
        }
    };

#if 0
    //////////////////////////////////////////////////////////////////////////
    // Functor
    //////////////////////////////////////////////////////////////////////////
    template <>
    struct transform_impl<ext::std::vector_tag> {
        template <typename V, typename F>
        static auto apply(V&& v, F&& f) {
            using U = std::remove_cv_t<std::remove_reference_t<
                decltype(f(*v.begin()))
            >>;
            using Alloc = typename std::remove_reference_t<V>::allocator_type;
            using NewAlloc = typename std::allocator_traits<Alloc>::
                             template rebind_alloc<U>;
            std::vector<U, NewAlloc> result; result.reserve(v.size());

            std::transform(begin(v), end(v),
                           std::back_inserter(result), std::forward<F>(f));
            return result;
        }

        template <typename T, typename Alloc, typename F>
        static auto apply(std::vector<T, Alloc>&& v, F&& f)
            -> std::enable_if_t<
                std::is_same<
                    T,
                    std::remove_cv_t<std::remove_reference_t<
                        decltype(f(*v.begin()))
                    >>
                >{}
                , std::vector<T, Alloc>
            >
        {
            // If we receive a rvalue and the function returns elements of
            // the same type, we modify the vector in-place instead of
            // returning a new one.
            std::transform(std::make_move_iterator(begin(v)),
                           std::make_move_iterator(end(v)),
                           begin(v), std::forward<F>(f));
            return std::move(v);
        }
    };
#endif
BOOST_HANA_NAMESPACE_END

#endif // !BOOST_HANA_EXT_STD_VECTOR_HPP
