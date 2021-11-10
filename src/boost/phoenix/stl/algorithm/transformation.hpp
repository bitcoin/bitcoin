// Copyright 2005 Daniel Wallin.
// Copyright 2005 Joel de Guzman.
// Copyright 2005 Dan Marsden.
// Copyright 2015 John Fletcher.
//
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// Modeled after range_ex, Copyright 2004 Eric Niebler

#ifndef BOOST_PHOENIX_ALGORITHM_TRANSFORMATION_HPP
#define BOOST_PHOENIX_ALGORITHM_TRANSFORMATION_HPP

#include <algorithm>
#include <numeric>

#include <boost/phoenix/core/limits.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_sort.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_remove.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_remove_if.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_unique.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_reverse.hpp>
#include <boost/phoenix/stl/algorithm/detail/has_sort.hpp>

#include <boost/phoenix/stl/algorithm/detail/begin.hpp>
#include <boost/phoenix/stl/algorithm/detail/end.hpp>
#include <boost/phoenix/stl/algorithm/detail/decay_array.hpp>

#include <boost/phoenix/function/adapt_callable.hpp>

//#include <boost/range/result_iterator.hpp> is deprecated
#include <boost/range/iterator.hpp>
#include <boost/range/difference_type.hpp>

#include <boost/mpl/if.hpp>

#include <boost/type_traits/is_void.hpp>

namespace boost { namespace phoenix { namespace impl
{
    struct swap
    {
        typedef void result_type;

        template <class A, class B>
        void operator()(A& a, B& b) const
        {
            using std::swap;
            swap(a, b);
        }
    };

    struct copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class I>
        struct result<This(R&, I)>
            : detail::decay_array<I>
        {};

        template<class R, class I>
        typename detail::decay_array<I>::type
        operator()(R& r, I i) const
        {
            return std::copy(detail::begin_(r), detail::end_(r), i);
        }
    };

    struct copy_backward
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class I>
        struct result<This(R&, I)>
            : result<This(R&, I const &)>
        {};

        template<typename This, class R, class I>
        struct result<This(R&, I &)>
        {
            typedef I type;
        };

        template<class R, class I>
        I operator()(R& r, I & i) const
        {
            return std::copy_backward(detail::begin_(r), detail::end_(r), i);
        }

        template<class R, class I>
        I const operator()(R& r, I const & i) const
        {
            return std::copy_backward(detail::begin_(r), detail::end_(r), i);
        }
    };

    struct transform
    {
        template <typename Sig>
        struct result;
        
        template<typename This, class R, class OutorI1, class ForOut>
        struct result<This(R&, OutorI1, ForOut)>
            : detail::decay_array<OutorI1>
        {
        };

        template<typename This, class R, class OutorI1, class ForOut, class BinF>
        struct result<This(R&, OutorI1, ForOut, BinF)>
            : detail::decay_array<ForOut>
        {
        };

        template<class R, class O, class F>
        typename result<transform(R&,O,F)>::type
        operator()(R& r, O o, F f) const
        {
            return std::transform(detail::begin_(r), detail::end_(r), o, f);
        }

        template<class R, class I, class O, class F>
        typename result<transform(R&,I,O,F)>::type
        operator()(R& r, I i, O o, F f) const
        {
            return std::transform(detail::begin_(r), detail::end_(r), i, o, f);
        }
    };

    struct replace
    {
        typedef void result_type;

        template<class R, class T>
        void operator()(R& r, T const& what, T const& with) const
        {
            std::replace(detail::begin_(r), detail::end_(r), what, with);
        }
    };

    struct replace_if
    {
        typedef void result_type;

        template<class R, class P, class T>
        void operator()(R& r, P p, T const& with) const
        {
            std::replace_if(detail::begin_(r), detail::end_(r), p, with);
        }
    };

    struct replace_copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O, class T, class T2>
        struct result<This(R&, O, T&, T2&)>
            : detail::decay_array<O>
        {};

        template<class R, class O, class T>
        typename detail::decay_array<O>::type 
        operator()(R& r, O o, T const& what, T const& with) const
        {
            return std::replace_copy(detail::begin_(r), detail::end_(r), o, what, with);
        }
    };

    struct replace_copy_if
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O, class P, class T>
        struct result<This(R&, O, P, T&)>
            : detail::decay_array<O>
        {};

        template<class R, class O, class P, class T>
        typename detail::decay_array<O>::type
        operator()(R& r, O o, P p, T const& with) const
        {
            return std::replace_copy_if(detail::begin_(r), detail::end_(r), o, p, with);
        }
    };

    struct fill
    {
        typedef void result_type;

        template<class R, class T>
        void operator()(R& r, T const& x) const
        {
            std::fill(detail::begin_(r), detail::end_(r), x);
        }
    };

    struct fill_n
    {
        typedef void result_type;

        template<class R, class N, class T>
        void operator()(R& r, N n, T const& x) const
        {
            std::fill_n(detail::begin_(r), n, x);
        }
    };

    struct generate
    {
        typedef void result_type;

        template<class R, class G>
        void operator()(R& r, G const & g) const
        {
            std::generate(detail::begin_(r), detail::end_(r), g);
        }
    };

    struct generate_n
    {
        typedef void result_type;

        template<class R, class N, class G>
        void operator()(R& r, N n, G g) const
        {
            std::generate_n(detail::begin_(r), n, g);
        }
    };

    struct remove
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class T>
        struct result<This(R&, T&)>
            : range_iterator<R>
        {
        };

        template<class R, class T>
        typename range_iterator<R>::type
        execute(R& r, T const& x, mpl::true_) const
        {
            r.remove(x);
            return detail::end_(r);
        }

        template<class R, class T>
        typename range_iterator<R>::type
        execute(R& r, T const& x, mpl::false_) const
        {
            return std::remove(detail::begin_(r), detail::end_(r), x);
        }

        template<class R, class T>
        typename range_iterator<R>::type
        operator()(R& r, T const& x) const
        {
            return execute(r, x, has_remove<R>());
        }
    };

    struct remove_if
    {
        template <typename Sig>
        struct result;

        template <typename This, class R, class P>
        struct result<This(R&,P)>
            : range_iterator<R>
        {
        };

        template<class R, class P>
        typename range_iterator<R>::type
        execute(R& r, P p, mpl::true_) const
        {
            r.remove_if(p);
            return detail::end_(r);
        }

        template<class R, class P>
        typename range_iterator<R>::type
        execute(R& r, P p, mpl::false_) const
        {
            return std::remove_if(detail::begin_(r), detail::end_(r), p);
        }

        template<class R, class P>
        typename range_iterator<R>::type
        operator()(R& r, P p) const
        {
            return execute(r, p, has_remove_if<R>());
        }
    };

    struct remove_copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O, class T>
        struct result<This(R&, O, T)>
            : detail::decay_array<O>
        {};

        template<class R, class O, class T>
        typename detail::decay_array<O>::type
        operator()(R& r, O o, T const& x) const
        {
            return std::remove_copy(detail::begin_(r), detail::end_(r), o, x);
        }
    };

    struct remove_copy_if
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O, class P>
        struct result<This(R&, O, P)>
            : detail::decay_array<O>
        {};

        template<class R, class O, class P>
        typename detail::decay_array<O>::type
        operator()(R& r, O o, P p) const
        {
            return std::remove_copy_if(detail::begin_(r), detail::end_(r), o, p);
        }
    };

    struct unique
    {
        template <typename Sig>
        struct result;

        template<typename This, class R>
        struct result<This(R&)>
            : range_iterator<R>
        {};

        template<typename This, class R, class P>
        struct result<This(R&, P)>
            : range_iterator<R>
        {};

        template<class R>
        typename range_iterator<R>::type
        execute(R& r, mpl::true_) const
        {
            r.unique();
            return detail::end_(r);
        }

        template<class R>
        typename range_iterator<R>::type
        execute(R& r, mpl::false_) const
        {
            return std::unique(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        typename range_iterator<R>::type
        operator()(R& r) const
        {
            return execute(r, has_unique<R>());
        }


        template<class R, class P>
        typename range_iterator<R>::type
        execute(R& r, P p, mpl::true_) const
        {
            r.unique(p);
            return detail::end_(r);
        }

        template<class R, class P>
        typename range_iterator<R>::type
        execute(R& r, P p, mpl::false_) const
        {
            return std::unique(detail::begin_(r), detail::end_(r), p);
        }

        template<class R, class P>
        typename range_iterator<R>::type
        operator()(R& r, P p) const
        {
            return execute(r, p, has_unique<R>());
        }
    };

    struct unique_copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O>
        struct result<This(R&, O)>
            : detail::decay_array<O>
        {};

        template<typename This, class R, class O, class P>
        struct result<This(R&, O, P)>
            : detail::decay_array<O>
        {};

        template<class R, class O>
        typename detail::decay_array<O>::type operator()(R& r, O o) const
        {
            return std::unique_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                );
        }

        template<class R, class O, class P>
        typename detail::decay_array<O>::type operator()(R& r, O o, P p) const
        {
            return std::unique_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                , p
                );
        }
    };

    struct reverse
    {
        typedef void result_type;

        template<class R>
        void execute(R& r, mpl::true_) const
        {
            r.reverse();
        }

        template<class R>
        void execute(R& r, mpl::false_) const
        {
            std::reverse(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        void operator()(R& r) const
        {
            execute(r, has_reverse<R>());
        }
    };

    struct reverse_copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class O>
        struct result<This(R&, O)>
            : detail::decay_array<O>
        {};

        template<class R, class O>
        typename detail::decay_array<O>::type operator()(R& r, O o) const
        {
            return std::reverse_copy(
                detail::begin_(r)
                , detail::end_(r)
                , o
                );
        }
    };

    struct rotate
    {
        typedef void result_type;

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            std::rotate(
                detail::begin_(r)
                , m
                , detail::end_(r)
                );
        }
    };

    struct rotate_copy
    {
        template <typename Sig>
        struct result;

        template<typename This, class R, class M, class O>
        struct result<This(R&, M, O)>
            : detail::decay_array<O>
        {};

        template<class R, class M, class O>
        typename detail::decay_array<O>::type operator()(R& r, M m, O o) const
        {
            return std::rotate_copy(
                detail::begin_(r)
                , m
                , detail::end_(r)
                , o
                );
        }
    };

#ifndef BOOST_NO_CXX98_RANDOM_SHUFFLE
    struct random_shuffle
    {
        typedef void result_type;

        template<class R>
        void operator()(R& r) const
        {
            return std::random_shuffle(detail::begin_(r), detail::end_(r));
        }

        template<class R, class G>
        void operator()(R& r, G g) const
        {
            return std::random_shuffle(detail::begin_(r), detail::end_(r), g);
        }
    };
#endif

    struct partition
    {
        template <typename Sig>
        struct result;

        template <typename This, class R, class P>
        struct result<This(R&, P)>
            : range_iterator<R>
        {};

        template<class R, class P>
        typename range_iterator<R>::type
        operator()(R& r, P p) const
        {
            return std::partition(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct stable_partition
    {
        template <typename Sig>
        struct result;

        template <typename This, class R, class P>
        struct result<This(R&, P)>
            : range_iterator<R>
        {};

        template<class R, class P>
        typename range_iterator<R>::type
        operator()(R& r, P p) const
        {
            return std::stable_partition(detail::begin_(r), detail::end_(r), p);
        }
    };

    struct sort
    {
        typedef void result_type;

        template<class R>
        void execute(R& r, mpl::true_) const
        {
            r.sort();
        }

        template<class R>
        void execute(R& r, mpl::false_) const
        {
            std::sort(detail::begin_(r), detail::end_(r));
        }

        template<class R>
        void operator()(R& r) const
        {
            execute(r, has_sort<R>());
        }

        template<class R, class C>
        void execute(R& r, C c, mpl::true_) const
        {
            r.sort(c);
        }

        template<class R, class C>
        void execute(R& r, C c, mpl::false_) const
        {
            std::sort(detail::begin_(r), detail::end_(r), c);
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            execute(r, c, has_sort<R>());
        }
    };

    struct stable_sort
    {
        typedef void result_type;

        template<class R>
        void operator()(R& r) const
        {
            std::stable_sort(detail::begin_(r), detail::end_(r));
        }

        template<class R, class C>
        void operator()(R& r, C c) const
        {
            std::stable_sort(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct partial_sort
    {
        typedef void result_type;

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            std::partial_sort(detail::begin_(r), m, detail::end_(r));
        }

        template<class R, class M, class C>
        void operator()(R& r, M m, C c) const
        {
            std::partial_sort(detail::begin_(r), m, detail::end_(r), c);
        }
    };

    struct partial_sort_copy
    {
        template <typename Sig>
        struct result;
        
        template <typename This, class R1, class R2>
        struct result<This(R1&, R2&)>
            : range_iterator<R2>
        {};

        template <typename This, class R1, class R2, class C>
        struct result<This(R1&, R2&, C)>
            : range_iterator<R2>
        {};

        template <class R1, class R2>
        typename range_iterator<R2>::type
        operator()(R1& r1, R2& r2) const
        {
            return std::partial_sort_copy(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                );
        }

        template <class R1, class R2, class C>
        typename range_iterator<R2>::type
        operator()(R1& r1, R2& r2, C c) const
        {
            return std::partial_sort_copy(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , c
                );
        }
    };

    struct nth_element
    {
        typedef void result_type;

        template<class R, class N>
        void operator()(R& r, N n) const
        {
            return std::nth_element(detail::begin_(r), n, detail::end_(r));
        }

        template<class R, class N, class C>
        void operator()(R& r, N n, C c) const
        {
            return std::nth_element(detail::begin_(r), n, detail::end_(r), c);
        }
    };

    struct merge 
    {
        template <typename Sig>
        struct result;

        template<typename This, class R1, class R2, class O>
        struct result<This(R1&, R2&, O)>
            : detail::decay_array<O>
        {};

        template<typename This, class R1, class R2, class O, class C>
        struct result<This(R1&, R2&, O, C)>
            : detail::decay_array<O>
        {};

        template<class R1, class R2, class O>
        typename detail::decay_array<O>::type operator()(R1& r1, R2& r2, O o) const
        {
            return std::merge(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template<class R1, class R2, class O, class C>
        typename detail::decay_array<O>::type operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::merge(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct inplace_merge 
    {
        typedef void result_type;

        template<class R, class M>
        void operator()(R& r, M m) const
        {
            return std::inplace_merge(detail::begin_(r), m, detail::end_(r));
        }

        template<class R, class M, class C>
        void operator()(R& r, M m, C c) const
        {
            return std::inplace_merge(detail::begin_(r), m, detail::end_(r), c);
        }
    };

    struct next_permutation
    {
        typedef bool result_type;

        template<class R>
        bool operator()(R& r) const
        {
            return std::next_permutation(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class C>
        bool operator()(R& r, C c) const
        {
            return std::next_permutation(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct prev_permutation
    {
        typedef bool result_type;

        template<class R>
        bool operator()(R& r) const
        {
            return std::prev_permutation(detail::begin_(r), detail::end_(r));
        }
    
        template<class R, class C>
        bool operator()(R& r, C c) const
        {
            return std::prev_permutation(detail::begin_(r), detail::end_(r), c);
        }
    };


    struct inner_product
    {
        template <typename Sig>
        struct result;

        template <typename This, typename R, typename I, typename T>
        struct result<This(R&, I, T)>
            : result<This(R&, I const &, T)>
        {};

        template <typename This, typename R, typename I, typename T>
        struct result<This(R&, I, T &)>
        {
            typedef T type;
        };

        template <typename This, typename R, typename I, typename T, typename C1, typename C2>
        struct result<This(R&, I, T, C1, C2)>
            : result<This(R&, I, T const &, C1, C2)>
        {};

        template <typename This, typename R, typename I, typename T, typename C1, typename C2>
        struct result<This(R&, I, T &, C1, C2)>
        {
            typedef T type;
        };

        template <class R, class I, class T>
        T
        operator()(R& r, I i, T t) const
        {
            return std::inner_product(
                detail::begin_(r), detail::end_(r), i, t);
        }

        template <class R, class I, class T, class C1, class C2>
        T
        operator()(R& r, I i, T t, C1 c1, C2 c2) const
        {
            return std::inner_product(
                detail::begin_(r), detail::end_(r), i, 
                t, c1, c2);
        }
    };

    struct partial_sum
    {
        template <typename Sig>
        struct result;

        template <typename This, class R, class I>
        struct result<This(R&, I)>
            : detail::decay_array<I>
        {};

        template <typename This, class R, class I, class C>
        struct result<This(R&, I, C)>
            : detail::decay_array<I>
        {};

        template <class R, class I>
        typename detail::decay_array<I>::type
        operator()(R& r, I i) const
        {
            return std::partial_sum(
                detail::begin_(r), detail::end_(r), i);
        }

        template <class R, class I, class C>
        typename detail::decay_array<I>::type
        operator()(R& r, I i, C c) const
        {
            return std::partial_sum(
                detail::begin_(r), detail::end_(r), i, c);
        }
    };

    struct adjacent_difference
    {
        template <typename Sig>
        struct result;
        
        template <typename This, class R, class I>
        struct result<This(R&, I)>
            : detail::decay_array<I>
        {};

        template <typename This,class R, class I, class C>
        struct result<This(R&, I, C)>
            : detail::decay_array<I>
        {};

        template <class R, class I>
        typename detail::decay_array<I>::type
        operator()(R& r, I i) const
        {
            return std::adjacent_difference(
                detail::begin_(r), detail::end_(r), i);
        }

        template <class R, class I, class C>
        typename detail::decay_array<I>::type
        operator()(R& r, I i, C c) const
        {
            return std::adjacent_difference(
                detail::begin_(r), detail::end_(r), i, c);
        }    
    };

    struct push_heap
    {
        typedef void result_type;

        template <class R>
        void operator()(R& r) const
        {
            std::push_heap(detail::begin_(r), detail::end_(r));
        }

        template <class R, class C>
        void operator()(R& r, C c) const
        {
            std::push_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct pop_heap
    {
        typedef void result_type;

        template <class R>
        void operator()(R& r) const
        {
            std::pop_heap(detail::begin_(r), detail::end_(r));
        }

        template <class R, class C>
        void operator()(R& r, C c) const
        {
            std::pop_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct make_heap
    {
        typedef void result_type;

        template <class R>
        void operator()(R& r) const
        {
            std::make_heap(detail::begin_(r), detail::end_(r));
        }

        template <class R, class C>
        void operator()(R& r, C c) const
        {
            std::make_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct sort_heap
    {
        typedef void result_type;

        template <class R>
        void operator()(R& r) const
        {
            std::sort_heap(detail::begin_(r), detail::end_(r));
        }

        template <class R, class C>
        void operator()(R& r, C c) const
        {
            std::sort_heap(detail::begin_(r), detail::end_(r), c);
        }
    };

    struct set_union
    {
        template <typename Sig>
        struct result;

        template <typename This, class R1, class R2, class O>
        struct result<This(R1&, R2&, O)>
            : detail::decay_array<O>
        {};

        template <typename This, class R1, class R2, class O, typename C>
        struct result<This(R1&, R2&, O, C)>
            : detail::decay_array<O>
        {};

        template <class R1, class R2, class O>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_union(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template <class R1, class R2, class O, class C>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_union(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_intersection
    {
        template <typename Sig>
        struct result;

        template <typename This, class R1, class R2, class O>
        struct result<This(R1&, R2&, O)>
            : detail::decay_array<O>
        {};

        template <typename This, class R1, class R2, class O, typename C>
        struct result<This(R1&, R2&, O, C)>
            : detail::decay_array<O>
        {};

        template <class R1, class R2, class O>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_intersection(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template <class R1, class R2, class O, class C>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_intersection(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_difference
    {
        template <typename Sig>
        struct result;

        template <typename This, class R1, class R2, class O>
        struct result<This(R1&, R2&, O)>
            : detail::decay_array<O>
        {};

        template <typename This, class R1, class R2, class O, class C>
        struct result<This(R1&, R2&, O, C)>
            : detail::decay_array<O>
        {};

        template <class R1, class R2, class O>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template <class R1, class R2, class O, class C>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

    struct set_symmetric_difference
    {
        template <typename Sig>
        struct result;

        template <typename This, class R1, class R2, class O>
        struct result<This(R1&, R2, O)>
            : detail::decay_array<O>
        {};

        template <typename This, class R1, class R2, class O, class C>
        struct result<This(R1&, R2, O, C)>
            : detail::decay_array<O>
        {};

        template <class R1, class R2, class O>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o) const
        {
            return std::set_symmetric_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                );
        }

        template <class R1, class R2, class O, class C>
        typename detail::decay_array<O>::type
        operator()(R1& r1, R2& r2, O o, C c) const
        {
            return std::set_symmetric_difference(
                detail::begin_(r1), detail::end_(r1)
                , detail::begin_(r2), detail::end_(r2)
                , o
                , c
                );
        }
    };

}}} // boost::phoenix::impl

namespace boost { namespace phoenix
{
    BOOST_PHOENIX_ADAPT_CALLABLE(swap, impl::swap, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(copy, impl::copy, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(copy_backward, impl::copy_backward, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(transform, impl::transform, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(transform, impl::transform, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(replace, impl::replace, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(replace_if, impl::replace_if, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(replace_copy, impl::replace_copy, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(replace_copy_if, impl::replace_copy_if, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(fill, impl::fill, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(fill_n, impl::fill_n, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(generate, impl::generate, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(generate_n, impl::generate_n, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(remove, impl::remove, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(remove_if, impl::remove_if, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(remove_copy, impl::remove_copy, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(remove_copy_if, impl::remove_copy_if, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(unique, impl::unique, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(unique, impl::unique, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(unique_copy, impl::unique_copy, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(unique_copy, impl::unique_copy, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(reverse, impl::reverse, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(reverse_copy, impl::reverse_copy, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(rotate, impl::rotate, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(rotate_copy, impl::rotate_copy, 3)
#ifndef BOOST_NO_CXX98_RANDOM_SHUFFLE
    BOOST_PHOENIX_ADAPT_CALLABLE(random_shuffle, impl::random_shuffle, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(random_shuffle, impl::random_shuffle, 2)
#endif
    BOOST_PHOENIX_ADAPT_CALLABLE(partition, impl::partition, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(stable_partition, impl::stable_partition, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(sort, impl::sort, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(sort, impl::sort, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(stable_sort, impl::stable_sort, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(stable_sort, impl::stable_sort, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sort, impl::partial_sort, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sort, impl::partial_sort, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sort_copy, impl::partial_sort_copy, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sort_copy, impl::partial_sort_copy, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(nth_element, impl::nth_element, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(nth_element, impl::nth_element, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(merge, impl::merge, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(merge, impl::merge, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(inplace_merge, impl::inplace_merge, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(inplace_merge, impl::inplace_merge, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(next_permutation, impl::next_permutation, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(next_permutation, impl::next_permutation, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(prev_permutation, impl::prev_permutation, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(prev_permutation, impl::prev_permutation, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(inner_product, impl::inner_product, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(inner_product, impl::inner_product, 5)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sum, impl::partial_sum, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(partial_sum, impl::partial_sum, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(adjacent_difference, impl::adjacent_difference, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(adjacent_difference, impl::adjacent_difference, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(push_heap, impl::push_heap, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(push_heap, impl::push_heap, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(pop_heap, impl::pop_heap, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(pop_heap, impl::pop_heap, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(make_heap, impl::make_heap, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(make_heap, impl::make_heap, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(sort_heap, impl::sort_heap, 1)
    BOOST_PHOENIX_ADAPT_CALLABLE(sort_heap, impl::sort_heap, 2)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_union, impl::set_union, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_union, impl::set_union, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_intersection, impl::set_intersection, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_intersection, impl::set_intersection, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_difference, impl::set_difference, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_difference, impl::set_difference, 4)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_symmetric_difference, impl::set_symmetric_difference, 3)
    BOOST_PHOENIX_ADAPT_CALLABLE(set_symmetric_difference, impl::set_symmetric_difference, 4)
}}

#endif
