// Copyright (C) 2019 T. Zachary Laine
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef BOOST_STL_INTERFACES_CONTAINER_INTERFACE_HPP
#define BOOST_STL_INTERFACES_CONTAINER_INTERFACE_HPP

#include <boost/stl_interfaces/reverse_iterator.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <algorithm>
#include <stdexcept>
#include <cstddef>


namespace boost { namespace stl_interfaces { namespace detail {

    template<typename T, typename SizeType>
    struct n_iter : iterator_interface<
                        n_iter<T, SizeType>,
                        std::random_access_iterator_tag,
                        T>
    {
        n_iter() : x_(nullptr), n_(0) {}
        n_iter(T const & x, SizeType n) : x_(&x), n_(n) {}

        T const & operator*() const { return *x_; }
        constexpr std::ptrdiff_t operator-(n_iter other) const noexcept
        {
            return std::ptrdiff_t(n_) - std::ptrdiff_t(other.n_);
        }
        n_iter & operator+=(std::ptrdiff_t offset)
        {
            n_ += offset;
            return *this;
        }

    private:
        T const * x_;
        SizeType n_;
    };

    template<typename T, typename SizeType>
    constexpr auto make_n_iter(T const & x, SizeType n) noexcept(
        noexcept(n_iter<T, SizeType>(x, n)))
    {
        using result_type = n_iter<T, SizeType>;
        return result_type(x, SizeType(0));
    }
    template<typename T, typename SizeType>
    constexpr auto make_n_iter_end(T const & x, SizeType n) noexcept(
        noexcept(n_iter<T, SizeType>(x, n)))
    {
        return n_iter<T, SizeType>(x, n);
    }

    template<typename Container>
    std::size_t fake_capacity(Container const & c)
    {
        return SIZE_MAX;
    }
    template<
        typename Container,
        typename Enable = decltype(
            std::size_t() = std::declval<Container const &>().capacity())>
    std::size_t fake_capacity(Container const & c)
    {
        return c.capacity();
    }

}}}

namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V1 {

    /** A CRTP template that one may derive from to make it easier to define
        container types.

        The template parameter `D` for `sequence_container_interface` may be
        an incomplete type. Before any member of the resulting specialization
        of `sequence_container_interface` other than special member functions
        is referenced, `D` shall be complete; shall model
        `std::derived_from<sequence_container_interface<D>>`,
        `std::semiregular`, and `std::forward_range`; and shall contain all
        the nested types required in Table 72: Container requirements and, for
        those whose iterator nested type models `std::bidirectinal_iterator`,
        those in Table 73: Reversible container requirements.

        For an object `d` of type `D`, a call to `std::ranges::begin(d)` sxhall
        not mutate any data members of `d`, and `d`'s destructor shall end the
        lifetimes of the objects in `[std::ranges::begin(d),
        std::ranges::end(d))`. */
    template<
        typename Derived,
        element_layout Contiguity = element_layout::discontiguous
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E = std::enable_if_t<
            std::is_class<Derived>::value &&
            std::is_same<Derived, std::remove_cv_t<Derived>>::value>
#endif
        >
    struct sequence_container_interface;

    namespace v1_dtl {
        template<typename Iter>
        using in_iter = std::is_convertible<
            typename std::iterator_traits<Iter>::iterator_category,
            std::input_iterator_tag>;

        template<typename D, typename = void>
        struct clear_impl
        {
            static constexpr void call(D & d) noexcept {}
        };
        template<typename D>
        struct clear_impl<D, void_t<decltype(std::declval<D>().clear())>>
        {
            static constexpr void call(D & d) noexcept { d.clear(); }
        };

        template<typename D, element_layout Contiguity>
        void derived_container(sequence_container_interface<D, Contiguity> const &);
    }

    template<
        typename Derived,
        element_layout Contiguity
#ifndef BOOST_STL_INTERFACES_DOXYGEN
        ,
        typename E
#endif
        >
    struct sequence_container_interface
    {
#ifndef BOOST_STL_INTERFACES_DOXYGEN
    private:
        constexpr Derived & derived() noexcept
        {
            return static_cast<Derived &>(*this);
        }
        constexpr const Derived & derived() const noexcept
        {
            return static_cast<Derived const &>(*this);
        }
        constexpr Derived & mutable_derived() const noexcept
        {
            return const_cast<Derived &>(static_cast<Derived const &>(*this));
        }
#endif

    public:
        template<typename D = Derived>
        constexpr auto empty() noexcept(
            noexcept(std::declval<D &>().begin() == std::declval<D &>().end()))
            -> decltype(
                std::declval<D &>().begin() == std::declval<D &>().end())
        {
            return derived().begin() == derived().end();
        }
        template<typename D = Derived>
        constexpr auto empty() const noexcept(noexcept(
            std::declval<D const &>().begin() ==
            std::declval<D const &>().end()))
            -> decltype(
                std::declval<D const &>().begin() ==
                std::declval<D const &>().end())
        {
            return derived().begin() == derived().end();
        }

        template<
            typename D = Derived,
            element_layout C = Contiguity,
            typename Enable = std::enable_if_t<C == element_layout::contiguous>>
        constexpr auto data() noexcept(noexcept(std::declval<D &>().begin()))
            -> decltype(std::addressof(*std::declval<D &>().begin()))
        {
            return std::addressof(*derived().begin());
        }
        template<
            typename D = Derived,
            element_layout C = Contiguity,
            typename Enable = std::enable_if_t<C == element_layout::contiguous>>
        constexpr auto data() const
            noexcept(noexcept(std::declval<D const &>().begin()))
                -> decltype(std::addressof(*std::declval<D const &>().begin()))
        {
            return std::addressof(*derived().begin());
        }

        template<typename D = Derived>
        constexpr auto size()
#if !BOOST_CLANG
            noexcept(noexcept(
                std::declval<D &>().end() - std::declval<D &>().begin()))
#endif
            -> decltype(typename D::size_type(
                std::declval<D &>().end() - std::declval<D &>().begin()))
        {
            return derived().end() - derived().begin();
        }
        template<typename D = Derived>
        constexpr auto size() const noexcept(noexcept(
            std::declval<D const &>().end() -
            std::declval<D const &>().begin()))
            -> decltype(typename D::size_type(
#if !BOOST_CLANG
                std::declval<D const &>().end() -
                std::declval<D const &>().begin()
#endif
                ))
        {
            return derived().end() - derived().begin();
        }

        template<typename D = Derived>
        constexpr auto front() noexcept(noexcept(*std::declval<D &>().begin()))
            -> decltype(*std::declval<D &>().begin())
        {
            return *derived().begin();
        }
        template<typename D = Derived>
        constexpr auto front() const
            noexcept(noexcept(*std::declval<D const &>().begin()))
                -> decltype(*std::declval<D const &>().begin())
        {
            return *derived().begin();
        }

        template<typename D = Derived>
        constexpr auto push_front(typename D::value_type const & x) noexcept(
            noexcept(std::declval<D &>().emplace_front(x)))
            -> decltype((void)std::declval<D &>().emplace_front(x))
        {
            derived().emplace_front(x);
        }

        template<typename D = Derived>
        constexpr auto push_front(typename D::value_type && x) noexcept(
            noexcept(std::declval<D &>().emplace_front(std::move(x))))
            -> decltype((void)std::declval<D &>().emplace_front(std::move(x)))
        {
            derived().emplace_front(std::move(x));
        }

        template<typename D = Derived>
        constexpr auto pop_front() noexcept -> decltype(
            std::declval<D &>().emplace_front(
                std::declval<typename D::value_type &>()),
            (void)std::declval<D &>().erase(std::declval<D &>().begin()))
        {
            derived().erase(derived().begin());
        }

        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<
                v1_dtl::decrementable_sentinel<D>::value &&
                v1_dtl::common_range<D>::value>>
        constexpr auto
        back() noexcept(noexcept(*std::prev(std::declval<D &>().end())))
            -> decltype(*std::prev(std::declval<D &>().end()))
        {
            return *std::prev(derived().end());
        }
        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<
                v1_dtl::decrementable_sentinel<D>::value &&
                v1_dtl::common_range<D>::value>>
        constexpr auto back() const
            noexcept(noexcept(*std::prev(std::declval<D const &>().end())))
                -> decltype(*std::prev(std::declval<D const &>().end()))
        {
            return *std::prev(derived().end());
        }

        template<typename D = Derived>
        constexpr auto push_back(typename D::value_type const & x) noexcept(
            noexcept(std::declval<D &>().emplace_back(x)))
            -> decltype((void)std::declval<D &>().emplace_back(x))
        {
            derived().emplace_back(x);
        }

        template<typename D = Derived>
        constexpr auto push_back(typename D::value_type && x) noexcept(
            noexcept(std::declval<D &>().emplace_back(std::move(x))))
            -> decltype((void)std::declval<D &>().emplace_back(std::move(x)))
        {
            derived().emplace_back(std::move(x));
        }

        template<typename D = Derived>
        constexpr auto pop_back() noexcept -> decltype(
            std::declval<D &>().emplace_back(
                std::declval<typename D::value_type>()),
            (void)std::declval<D &>().erase(
                std::prev(std::declval<D &>().end())))
        {
            derived().erase(std::prev(derived().end()));
        }

        template<typename D = Derived>
        constexpr auto operator[](typename D::size_type n) noexcept(
            noexcept(std::declval<D &>().begin()[n]))
            -> decltype(std::declval<D &>().begin()[n])
        {
            return derived().begin()[n];
        }
        template<typename D = Derived>
        constexpr auto operator[](typename D::size_type n) const
            noexcept(noexcept(std::declval<D const &>().begin()[n]))
                -> decltype(std::declval<D const &>().begin()[n])
        {
            return derived().begin()[n];
        }

        template<typename D = Derived>
        constexpr auto at(typename D::size_type i)
            -> decltype(std::declval<D &>().size(), std::declval<D &>()[i])
        {
            if (derived().size() <= i) {
                throw std::out_of_range(
                    "Bounds check failed in sequence_container_interface::at()");
            }
            return derived()[i];
        }

        template<typename D = Derived>
        constexpr auto at(typename D::size_type i) const -> decltype(
            std::declval<D const &>().size(), std::declval<D const &>()[i])
        {
            if (derived().size() <= i) {
                throw std::out_of_range(
                    "Bounds check failed in sequence_container_interface::at()");
            }
            return derived()[i];
        }

        template<typename D = Derived, typename Iter = typename D::const_iterator>
        constexpr Iter begin() const
            noexcept(noexcept(std::declval<D &>().begin()))
        {
            return Iter(mutable_derived().begin());
        }
        template<typename D = Derived, typename Iter = typename D::const_iterator>
        constexpr Iter end() const noexcept(noexcept(std::declval<D &>().end()))
        {
            return Iter(mutable_derived().end());
        }

        template<typename D = Derived>
        constexpr auto cbegin() const
            noexcept(noexcept(std::declval<D const &>().begin()))
                -> decltype(std::declval<D const &>().begin())
        {
            return derived().begin();
        }
        template<typename D = Derived>
        constexpr auto cend() const
            noexcept(noexcept(std::declval<D const &>().end()))
                -> decltype(std::declval<D const &>().end())
        {
            return derived().end();
        }

        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<v1_dtl::common_range<D>::value>>
        constexpr auto rbegin() noexcept(noexcept(
            stl_interfaces::make_reverse_iterator(std::declval<D &>().end())))
        {
            return stl_interfaces::make_reverse_iterator(derived().end());
        }
        template<
            typename D = Derived,
            typename Enable = std::enable_if_t<v1_dtl::common_range<D>::value>>
        constexpr auto rend() noexcept(noexcept(
            stl_interfaces::make_reverse_iterator(std::declval<D &>().begin())))
        {
            return stl_interfaces::make_reverse_iterator(derived().begin());
        }

        template<typename D = Derived>
        constexpr auto rbegin() const
            noexcept(noexcept(std::declval<D &>().rbegin()))
        {
            return
                typename D::const_reverse_iterator(mutable_derived().rbegin());
        }
        template<typename D = Derived>
        constexpr auto rend() const
            noexcept(noexcept(std::declval<D &>().rend()))
        {
            return typename D::const_reverse_iterator(mutable_derived().rend());
        }

        template<typename D = Derived>
        constexpr auto crbegin() const
            noexcept(noexcept(std::declval<D const &>().rbegin()))
                -> decltype(std::declval<D const &>().rbegin())
        {
            return derived().rbegin();
        }
        template<typename D = Derived>
        constexpr auto crend() const
            noexcept(noexcept(std::declval<D const &>().rend()))
                -> decltype(std::declval<D const &>().rend())
        {
            return derived().rend();
        }

        template<typename D = Derived>
        constexpr auto insert(
            typename D::const_iterator pos,
            typename D::value_type const &
                x) noexcept(noexcept(std::declval<D &>().emplace(pos, x)))
            -> decltype(std::declval<D &>().emplace(pos, x))
        {
            return derived().emplace(pos, x);
        }

        template<typename D = Derived>
        constexpr auto insert(
            typename D::const_iterator pos,
            typename D::value_type &&
                x) noexcept(noexcept(std::declval<D &>()
                                         .emplace(pos, std::move(x))))
            -> decltype(std::declval<D &>().emplace(pos, std::move(x)))
        {
            return derived().emplace(pos, std::move(x));
        }

        template<typename D = Derived>
        constexpr auto insert(
            typename D::const_iterator pos,
            typename D::size_type n,
            typename D::value_type const & x)
            // If you see an error in this noexcept() expression, that's
            // because this function is not properly constrained.  In other
            // words, Derived does not have a "range" insert like
            // insert(position, first, last).  If that is the case, this
            // function should be removed via SFINAE from overload resolution.
            // However, both the trailing decltype code below and a
            // std::enable_if in the template parameters do not work.  Sorry
            // about that.  See below for details.
            noexcept(noexcept(std::declval<D &>().insert(
                pos, detail::make_n_iter(x, n), detail::make_n_iter_end(x, n))))
        // This causes the compiler to infinitely recurse into this function's
        // declaration, even though the call below does not match the
        // signature of this function.
#if 0
            -> decltype(std::declval<D &>().insert(
                pos, detail::make_n_iter(x, n), detail::make_n_iter_end(x, n)))
#endif
        {
            return derived().insert(
                pos, detail::make_n_iter(x, n), detail::make_n_iter_end(x, n));
        }

        template<typename D = Derived>
        constexpr auto insert(
            typename D::const_iterator pos,
            std::initializer_list<typename D::value_type>
                il) noexcept(noexcept(std::declval<D &>()
                                          .insert(pos, il.begin(), il.end())))
            -> decltype(std::declval<D &>().insert(pos, il.begin(), il.end()))
        {
            return derived().insert(pos, il.begin(), il.end());
        }

        template<typename D = Derived>
        constexpr auto erase(typename D::const_iterator pos) noexcept
            -> decltype(std::declval<D &>().erase(pos, std::next(pos)))
        {
            return derived().erase(pos, std::next(pos));
        }

        template<
            typename InputIterator,
            typename D = Derived,
            typename Enable =
                std::enable_if_t<v1_dtl::in_iter<InputIterator>::value>>
        constexpr auto assign(InputIterator first, InputIterator last) noexcept(
            noexcept(std::declval<D &>().insert(
                std::declval<D &>().begin(), first, last)))
            -> decltype(
                std::declval<D &>().erase(
                    std::declval<D &>().begin(), std::declval<D &>().end()),
                (void)std::declval<D &>().insert(
                    std::declval<D &>().begin(), first, last))
        {
            auto out = derived().begin();
            auto const out_last = derived().end();
            for (; out != out_last && first != last; ++first, ++out) {
                *out = *first;
            }
            if (out != out_last)
                derived().erase(out, out_last);
            if (first != last)
                derived().insert(derived().end(), first, last);
        }

        template<typename D = Derived>
        constexpr auto assign(
            typename D::size_type n,
            typename D::value_type const &
                x) noexcept(noexcept(std::declval<D &>()
                                         .insert(
                                             std::declval<D &>().begin(),
                                             detail::make_n_iter(x, n),
                                             detail::make_n_iter_end(x, n))))
            -> decltype(
                std::declval<D &>().size(),
                std::declval<D &>().erase(
                    std::declval<D &>().begin(), std::declval<D &>().end()),
                (void)std::declval<D &>().insert(
                    std::declval<D &>().begin(),
                    detail::make_n_iter(x, n),
                    detail::make_n_iter_end(x, n)))
        {
            if (detail::fake_capacity(derived()) < n) {
                Derived temp(n, x);
                derived().swap(temp);
            } else {
                auto const min_size =
                    std::min<std::ptrdiff_t>(n, derived().size());
                auto const fill_end =
                    std::fill_n(derived().begin(), min_size, x);
                if (min_size < (std::ptrdiff_t)derived().size()) {
                    derived().erase(fill_end, derived().end());
                } else {
                    n -= min_size;
                    derived().insert(
                        derived().begin(),
                        detail::make_n_iter(x, n),
                        detail::make_n_iter_end(x, n));
                }
            }
        }

        template<typename D = Derived>
        constexpr auto
        assign(std::initializer_list<typename D::value_type> il) noexcept(
            noexcept(std::declval<D &>().assign(il.begin(), il.end())))
            -> decltype((void)std::declval<D &>().assign(il.begin(), il.end()))
        {
            derived().assign(il.begin(), il.end());
        }

        template<typename D = Derived>
        constexpr auto
        operator=(std::initializer_list<typename D::value_type> il) noexcept(
            noexcept(std::declval<D &>().assign(il.begin(), il.end())))
            -> decltype(
                std::declval<D &>().assign(il.begin(), il.end()),
                std::declval<D &>())
        {
            derived().assign(il.begin(), il.end());
            return *this;
        }

        template<typename D = Derived>
        constexpr auto clear() noexcept
            -> decltype((void)std::declval<D &>().erase(
                std::declval<D &>().begin(), std::declval<D &>().end()))
        {
            derived().erase(derived().begin(), derived().end());
        }
    };

    /** Implementation of free function `swap()` for all containers derived
        from `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto swap(
        ContainerInterface & lhs,
        ContainerInterface & rhs) noexcept(noexcept(lhs.swap(rhs)))
        -> decltype(v1_dtl::derived_container(lhs), lhs.swap(rhs))
    {
        return lhs.swap(rhs);
    }

    /** Implementation of `operator==()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto
    operator==(ContainerInterface const & lhs, ContainerInterface const & rhs) noexcept(
        noexcept(lhs.size() == rhs.size()) &&
        noexcept(*lhs.begin() == *rhs.begin()))
        -> decltype(
            v1_dtl::derived_container(lhs),
            lhs.size() == rhs.size(),
            *lhs.begin() == *rhs.begin(),
            true)
    {
        return lhs.size() == rhs.size() &&
               std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    /** Implementation of `operator!=()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto operator!=(
        ContainerInterface const & lhs,
        ContainerInterface const & rhs) noexcept(noexcept(lhs == rhs))
        -> decltype(v1_dtl::derived_container(lhs), lhs == rhs)
    {
        return !(lhs == rhs);
    }

    /** Implementation of `operator<()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto operator<(
        ContainerInterface const & lhs,
        ContainerInterface const &
            rhs) noexcept(noexcept(*lhs.begin() < *rhs.begin()))
        -> decltype(
            v1_dtl::derived_container(lhs), *lhs.begin() < *rhs.begin(), true)
    {
        auto it1 = lhs.begin();
        auto const last1 = lhs.end();
        auto it2 = rhs.begin();
        auto const last2 = rhs.end();
        for (; it1 != last1 && it2 != last2; ++it1, ++it2) {
            if (*it1 < *it2)
                return true;
            if (*it2 < *it1)
                return false;
        }
        return it1 == last1 && it2 != last2;
    }

    /** Implementation of `operator<=()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto operator<=(
        ContainerInterface const & lhs,
        ContainerInterface const & rhs) noexcept(noexcept(lhs < rhs))
        -> decltype(v1_dtl::derived_container(lhs), lhs < rhs)
    {
        return !(rhs < lhs);
    }

    /** Implementation of `operator>()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto operator>(
        ContainerInterface const & lhs,
        ContainerInterface const & rhs) noexcept(noexcept(lhs < rhs))
        -> decltype(v1_dtl::derived_container(lhs), lhs < rhs)
    {
        return rhs < lhs;
    }

    /** Implementation of `operator>=()` for all containers derived from
        `sequence_container_interface`.  */
    template<typename ContainerInterface>
    constexpr auto operator>=(
        ContainerInterface const & lhs,
        ContainerInterface const & rhs) noexcept(noexcept(lhs < rhs))
        -> decltype(v1_dtl::derived_container(lhs), lhs < rhs)
    {
        return !(lhs < rhs);
    }

}}}

#if defined(BOOST_STL_INTERFACES_DOXYGEN) || BOOST_STL_INTERFACES_USE_CONCEPTS

namespace boost { namespace stl_interfaces { BOOST_STL_INTERFACES_NAMESPACE_V2 {

    namespace v2_dtl {

        // This needs to become an exposition-only snake-case template alias
        // when standardized.
        template<typename T>
        using container_size_t = typename T::size_type;

        template<typename T, typename I>
        // clang-format off
        concept range_insert =
            requires (T t, std::ranges::iterator_t<T> t_it, I it) {
            t.template insert<I>(t_it, it, it);
            // clang-format on
        };

        template<typename T>
        using n_iter_t =
            detail::n_iter<std::ranges::range_value_t<T>, container_size_t<T>>;
    }

    // clang-format off

    /** A CRTP template that one may derive from to make it easier to define
        container types.

        The template parameter `D` for `sequence_container_interface` may be
        an incomplete type. Before any member of the resulting specialization
        of `sequence_container_interface` other than special member functions
        is referenced, `D` shall be complete; shall model
        `std::derived_from<sequence_container_interface<D>>`,
        `std::semiregular`, and `std::forward_range`; and shall contain all
        the nested types required in Table 72: Container requirements and, for
        those whose iterator nested type models `std::bidirectinal_iterator`,
        those in Table 73: Reversible container requirements.

        For an object `d` of type `D`, a call to `std::ranges::begin(d)` shall
        not mutate any data members of `d`, and `d`'s destructor shall end the
        lifetimes of the objects in `[std::ranges::begin(d),
        std::ranges::end(d))`.

        The `Contiguity` template parameter is not needed, and is unused.  It
        only exists to make the transition from `namespace v1` to `namespace
        v2` seamless. */
    template<typename D,
             element_layout Contiguity = element_layout::discontiguous>
      requires std::is_class_v<D> && std::same_as<D, std::remove_cv_t<D>>
    struct sequence_container_interface
    {
    private:
      constexpr D& derived() noexcept {
        return static_cast<D&>(*this);
      }
      constexpr const D& derived() const noexcept {
        return static_cast<const D&>(*this);
      }
      constexpr D & mutable_derived() const noexcept {
        return const_cast<D&>(static_cast<const D&>(*this));
      }
      static constexpr void clear_impl(D& d) noexcept {}
      static constexpr void clear_impl(D& d) noexcept
        requires requires { d.clear(); }
      { d.clear(); }

    public:
      constexpr bool empty() const {
        return std::ranges::begin(derived()) == std::ranges::end(derived());
      }

      constexpr auto data() requires std::contiguous_iterator<std::ranges::iterator_t<D>> {
        return std::to_address(std::ranges::begin(derived()));
      }
      constexpr auto data() const requires std::contiguous_iterator<std::ranges::iterator_t<const D>> {
          return std::to_address(std::ranges::begin(derived()));
        }

      template<typename C = D>
      constexpr v2_dtl::container_size_t<C> size() const
        requires std::sized_sentinel_for<std::ranges::sentinel_t<const C>, std::ranges::iterator_t<const C>> {
          return v2_dtl::container_size_t<C>(
            std::ranges::end(derived()) - std::ranges::begin(derived()));
        }

      constexpr decltype(auto) front() {
        BOOST_ASSERT(!empty());
        return *std::ranges::begin(derived());
      }
      constexpr decltype(auto) front() const {
        BOOST_ASSERT(!empty());
        return *std::ranges::begin(derived());
      }

      template<typename C = D>
        constexpr void push_front(const std::ranges::range_value_t<C>& x)
          requires requires { derived().emplace_front(x); } {
            derived().emplace_front(x);
          }
      template<typename C = D>
        constexpr void push_front(std::ranges::range_value_t<C>&& x)
          requires requires { derived().emplace_front(std::move(x)); } {
            derived().emplace_front(std::move(x));
          }
      constexpr void pop_front() noexcept
        requires requires (const std::ranges::range_value_t<D>& x, std::ranges::iterator_t<D> position) {
          derived().emplace_front(x);
          derived().erase(position);
        } {
          return derived().erase(std::ranges::begin(derived()));
        }

      constexpr decltype(auto) back()
        requires std::ranges::bidirectional_range<D> && std::ranges::common_range<D> {
          BOOST_ASSERT(!empty());
          return *std::ranges::prev(std::ranges::end(derived()));
        }
      constexpr decltype(auto) back() const
        requires std::ranges::bidirectional_range<const D> && std::ranges::common_range<const D> {
          BOOST_ASSERT(!empty());
          return *std::ranges::prev(std::ranges::end(derived()));
        }

      template<std::ranges::bidirectional_range C = D>
        constexpr void push_back(const std::ranges::range_value_t<C>& x)
          requires std::ranges::common_range<C> && requires { derived().emplace_back(x); } {
            derived().emplace_back(x);
          }
      template<std::ranges::bidirectional_range C = D>
        constexpr void push_back(std::ranges::range_value_t<C>&& x)
          requires std::ranges::common_range<C> && requires { derived().emplace_back(std::move(x)); } {
            derived().emplace_back(std::move(x));
          }
      constexpr void pop_back() noexcept
        requires std::ranges::bidirectional_range<D> && std::ranges::common_range<D> &&
          requires (std::ranges::range_value_t<D> x, std::ranges::iterator_t<D> position) {
          derived().emplace_back(std::move(x));
            derived().erase(position);
          } {
            return derived().erase(std::ranges::prev(std::ranges::end(derived())));
          }

      template<std::ranges::random_access_range C = D>
        constexpr decltype(auto) operator[](v2_dtl::container_size_t<C> n) {
          return std::ranges::begin(derived())[n];
        }
      template<std::ranges::random_access_range C = const D>
        constexpr decltype(auto) operator[](v2_dtl::container_size_t<C> n) const {
          return std::ranges::begin(derived())[n];
        }

      template<std::ranges::random_access_range C = D>
        constexpr decltype(auto) at(v2_dtl::container_size_t<C> n) {
          if (derived().size() <= n)
            throw std::out_of_range("Bounds check failed in sequence_container_interface::at()");
          return std::ranges::begin(derived())[n];
        }
      template<std::ranges::random_access_range C = const D>
        constexpr decltype(auto) at(v2_dtl::container_size_t<C> n) const {
          if (derived().size() <= n)
            throw std::out_of_range("Bounds check failed in sequence_container_interface::at()");
          return std::ranges::begin(derived())[n];
        }

      constexpr auto begin() const {
        return typename D::const_iterator(mutable_derived().begin());
      }
      constexpr auto end() const {
        return typename D::const_iterator(mutable_derived().end());
      }

      constexpr auto cbegin() const { return derived().begin(); }
      constexpr auto cend() const { return derived().end(); }

      constexpr auto rbegin()
        requires std::ranges::bidirectional_range<D> && std::ranges::common_range<D> {
          return std::reverse_iterator(std::ranges::end(derived()));
        }
      constexpr auto rend()
        requires std::ranges::bidirectional_range<D> && std::ranges::common_range<D> {
          return std::reverse_iterator(std::ranges::begin(derived()));
        }

      constexpr auto rbegin() const
        requires std::ranges::bidirectional_range<const D> &&
          std::ranges::common_range<const D> {
            return std::reverse_iterator(std::ranges::iterator_t<const D>(
              mutable_derived().end()));
          }
      constexpr auto rend() const
        requires std::ranges::bidirectional_range<const D> &&
          std::ranges::common_range<const D> {
            return std::reverse_iterator(std::ranges::iterator_t<const D>(
              mutable_derived().begin()));
          }

      constexpr auto crbegin() const
        requires std::ranges::bidirectional_range<const D> &&
          std::ranges::common_range<const D> {
            return std::reverse_iterator(std::ranges::iterator_t<const D>(
              mutable_derived().end()));
            }
      constexpr auto crend() const
        requires std::ranges::bidirectional_range<const D> &&
          std::ranges::common_range<const D> {
            return std::reverse_iterator(std::ranges::iterator_t<const D>(
              mutable_derived().begin()));
            }

      template<typename C = D>
        constexpr auto insert(std::ranges::iterator_t<const C> position,
                              const std::ranges::range_value_t<C>& x)
          requires requires { derived().emplace(position, x); } {
            return derived().emplace(position, x);
          }
      template<typename C = D>
        constexpr auto insert(std::ranges::iterator_t<const C> position,
                              std::ranges::range_value_t<C>&& x)
          requires requires { derived().emplace(position, std::move(x)); } {
            return derived().emplace(position, std::move(x));
          }
      template<typename C = D>
        constexpr auto insert(std::ranges::iterator_t<const C> position,
                              v2_dtl::container_size_t<C> n,
                              const std::ranges::range_value_t<C>& x)
          requires v2_dtl::range_insert<C, v2_dtl::n_iter_t<C>> {
            auto first = detail::make_n_iter(x, n);
            auto last = detail::make_n_iter_end(x, n);
            return derived().insert(
                position, detail::make_n_iter(x, n), detail::make_n_iter_end(x, n));
            }
      template<typename C = D>
        constexpr auto insert(std::ranges::iterator_t<const C> position,
                              std::initializer_list<std::ranges::range_value_t<C>> il)
          requires requires {
            derived().template insert<decltype(position), decltype(il)>(
                position, il.begin(), il.end()); } {
              return derived().insert(position, il.begin(), il.end());
            }

      template<typename C = D>
        constexpr void erase(C::const_iterator position)
          requires requires { derived().erase(position, std::ranges::next(position)); } {
            derived().erase(position, std::ranges::next(position));
          }

      template<std::input_iterator Iter, typename C = D>
        constexpr void assign(Iter first, Iter last)
          requires requires {
            derived().erase(std::ranges::begin(derived()), std::ranges::end(derived()));
            derived().insert(std::ranges::begin(derived()), first, last); } {
              auto out = derived().begin();
              auto const out_last = derived().end();
              for (; out != out_last && first != last; ++first, ++out) {
                *out = *first;
              }
              if (out != out_last)
                derived().erase(out, out_last);
              if (first != last)
                derived().insert(derived().end(), first, last);
            }
      template<typename C = D>
        constexpr void assign(v2_dtl::container_size_t<C> n,
                              const std::ranges::range_value_t<C>& x)
          requires requires {
            { derived().size() } -> std::convertible_to<std::size_t>;
            derived().erase(std::ranges::begin(derived()), std::ranges::end(derived()));
            derived().insert(std::ranges::begin(derived()),
                             detail::make_n_iter(x, n),
                             detail::make_n_iter_end(x, n)); } {
              if (detail::fake_capacity(derived()) < n) {
                C temp(n, x);
                derived().swap(temp);
              } else {
                auto const min_size = std::min<std::ptrdiff_t>(n, derived().size());
                auto const fill_end = std::fill_n(derived().begin(), min_size, x);
                if (min_size < (std::ptrdiff_t)derived().size()) {
                  derived().erase(fill_end, derived().end());
                } else {
                  n -= min_size;
                  derived().insert(
                    derived().begin(),
                    detail::make_n_iter(x, n),
                    detail::make_n_iter_end(x, n));
                }
              }
            }
      template<typename C = D>
        constexpr void assign(std::initializer_list<std::ranges::range_value_t<C>> il)
          requires requires { derived().assign(il.begin(), il.end()); } {
            derived().assign(il.begin(), il.end());
          }

      constexpr void clear() noexcept
        requires requires {
          derived().erase(std::ranges::begin(derived()), std::ranges::end(derived())); } {
            derived().erase(std::ranges::begin(derived()), std::ranges::end(derived()));
          }

      template<typename C = D>
        constexpr decltype(auto) operator=(
            std::initializer_list<std::ranges::range_value_t<C>> il)
          requires requires { derived().assign(il.begin(), il.end()); } {
            derived().assign(il.begin(), il.end());
            return *this;
          }

      friend constexpr void swap(D& lhs, D& rhs) requires requires { lhs.swap(rhs); } {
        return lhs.swap(rhs);
      }

      friend constexpr bool operator==(const D& lhs, const D& rhs)
        requires std::ranges::sized_range<const D> &&
          requires { std::ranges::equal(lhs, rhs); } {
            return lhs.size() == rhs.size() && std::ranges::equal(lhs, rhs);
          }
#if 0 // TODO: This appears to work, but as of this writing (and using GCC
      // 10), op<=> is not yet being used to evaluate op==, op<, etc.
      friend constexpr std::compare_three_way_result_t<std::ranges::range_reference_t<const D>>
        operator<=>(const D& lhs, const D& rhs)
        requires std::three_way_comparable<std::ranges::range_reference_t<const D>> {
          return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(),
                                                        rhs.begin(), rhs.end());
        }
#else
      friend constexpr bool operator!=(const D& lhs, const D& rhs)
        requires requires { lhs == rhs; } {
          return !(lhs == rhs);
        }
      friend constexpr bool operator<(D lhs, D rhs)
        requires std::totally_ordered<std::ranges::range_reference_t<D>> {
          return std::ranges::lexicographical_compare(lhs, rhs);
        }
      friend constexpr bool operator<=(D lhs, D rhs)
        requires std::totally_ordered<std::ranges::range_reference_t<D>> {
            return lhs == rhs || lhs < rhs;
          }
      friend constexpr bool operator>(D lhs, D rhs)
        requires std::totally_ordered<std::ranges::range_reference_t<D>> {
          return !(lhs <= rhs);
        }
      friend constexpr bool operator>=(D lhs, D rhs)
        requires std::totally_ordered<std::ranges::range_reference_t<D>> {
            return rhs <= lhs;
          }
#endif
    };

    // clang-format on

}}}

#endif

#endif
