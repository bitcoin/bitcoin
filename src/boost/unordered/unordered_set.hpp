
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/unordered for documentation

#ifndef BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED
#define BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED

#include <boost/config.hpp>
#if defined(BOOST_HAS_PRAGMA_ONCE)
#pragma once
#endif

#include <boost/core/explicit_operator_bool.hpp>
#include <boost/functional/hash.hpp>
#include <boost/move/move.hpp>
#include <boost/unordered/detail/set.hpp>

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
#include <initializer_list>
#endif

#if defined(BOOST_MSVC)
#pragma warning(push)
// conditional expression is constant
#pragma warning(disable : 4127)
#if BOOST_MSVC >= 1400
// the inline specifier cannot be used when a friend declaration refers to a
// specialization of a function template
#pragma warning(disable : 4396)
#endif
#endif

namespace boost {
  namespace unordered {
    template <class T, class H, class P, class A> class unordered_set
    {
#if defined(BOOST_UNORDERED_USE_MOVE)
      BOOST_COPYABLE_AND_MOVABLE(unordered_set)
#endif
      template <typename, typename, typename, typename>
      friend class unordered_multiset;

    public:
      typedef T key_type;
      typedef T value_type;
      typedef H hasher;
      typedef P key_equal;
      typedef A allocator_type;

    private:
      typedef boost::unordered::detail::set<A, T, H, P> types;
      typedef typename types::value_allocator_traits value_allocator_traits;
      typedef typename types::table table;
      typedef typename table::node_pointer node_pointer;
      typedef typename table::link_pointer link_pointer;

    public:
      typedef typename value_allocator_traits::pointer pointer;
      typedef typename value_allocator_traits::const_pointer const_pointer;

      typedef value_type& reference;
      typedef value_type const& const_reference;

      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;

      typedef typename table::iterator iterator;
      typedef typename table::c_iterator const_iterator;
      typedef typename table::l_iterator local_iterator;
      typedef typename table::cl_iterator const_local_iterator;
      typedef typename types::node_type node_type;
      typedef typename types::insert_return_type insert_return_type;

    private:
      table table_;

    public:
      // constructors

      unordered_set();

      explicit unordered_set(size_type, const hasher& = hasher(),
        const key_equal& = key_equal(),
        const allocator_type& = allocator_type());

      template <class InputIt>
      unordered_set(InputIt, InputIt,
        size_type = boost::unordered::detail::default_bucket_count,
        const hasher& = hasher(), const key_equal& = key_equal(),
        const allocator_type& = allocator_type());

      unordered_set(unordered_set const&);

#if defined(BOOST_UNORDERED_USE_MOVE) ||                                       \
  !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      unordered_set(BOOST_RV_REF(unordered_set) other)
        BOOST_NOEXCEPT_IF(table::nothrow_move_constructible)
          : table_(other.table_, boost::unordered::detail::move_tag())
      {
        // The move is done in table_
      }
#endif

      explicit unordered_set(allocator_type const&);

      unordered_set(unordered_set const&, allocator_type const&);

      unordered_set(BOOST_RV_REF(unordered_set), allocator_type const&);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_set(std::initializer_list<value_type>,
        size_type = boost::unordered::detail::default_bucket_count,
        const hasher& = hasher(), const key_equal& l = key_equal(),
        const allocator_type& = allocator_type());
#endif

      explicit unordered_set(size_type, const allocator_type&);

      explicit unordered_set(size_type, const hasher&, const allocator_type&);

      template <class InputIt>
      unordered_set(InputIt, InputIt, size_type, const allocator_type&);

      template <class InputIt>
      unordered_set(
        InputIt, InputIt, size_type, const hasher&, const allocator_type&);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_set(
        std::initializer_list<value_type>, size_type, const allocator_type&);

      unordered_set(std::initializer_list<value_type>, size_type, const hasher&,
        const allocator_type&);
#endif

      // Destructor

      ~unordered_set() BOOST_NOEXCEPT;

// Assign

#if defined(BOOST_UNORDERED_USE_MOVE)
      unordered_set& operator=(BOOST_COPY_ASSIGN_REF(unordered_set) x)
      {
        table_.assign(x.table_, boost::unordered::detail::true_type());
        return *this;
      }

      unordered_set& operator=(BOOST_RV_REF(unordered_set) x)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_move_assignable<H>::value&&
              boost::is_nothrow_move_assignable<P>::value)
      {
        table_.move_assign(x.table_, boost::unordered::detail::true_type());
        return *this;
      }
#else
      unordered_set& operator=(unordered_set const& x)
      {
        table_.assign(x.table_, boost::unordered::detail::true_type());
        return *this;
      }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      unordered_set& operator=(unordered_set&& x)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_move_assignable<H>::value&&
              boost::is_nothrow_move_assignable<P>::value)
      {
        table_.move_assign(x.table_, boost::unordered::detail::true_type());
        return *this;
      }
#endif
#endif

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_set& operator=(std::initializer_list<value_type>);
#endif

      allocator_type get_allocator() const BOOST_NOEXCEPT
      {
        return table_.node_alloc();
      }

      // iterators

      iterator begin() BOOST_NOEXCEPT { return iterator(table_.begin()); }

      const_iterator begin() const BOOST_NOEXCEPT
      {
        return const_iterator(table_.begin());
      }

      iterator end() BOOST_NOEXCEPT { return iterator(); }

      const_iterator end() const BOOST_NOEXCEPT { return const_iterator(); }

      const_iterator cbegin() const BOOST_NOEXCEPT
      {
        return const_iterator(table_.begin());
      }

      const_iterator cend() const BOOST_NOEXCEPT { return const_iterator(); }

      // size and capacity

      bool empty() const BOOST_NOEXCEPT { return table_.size_ == 0; }

      size_type size() const BOOST_NOEXCEPT { return table_.size_; }

      size_type max_size() const BOOST_NOEXCEPT;

// emplace

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <class... Args>
      std::pair<iterator, bool> emplace(BOOST_FWD_REF(Args)... args)
      {
        return table_.emplace_unique(
          table::extractor::extract(boost::forward<Args>(args)...),
          boost::forward<Args>(args)...);
      }

#else

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1

      // 0 argument emplace requires special treatment in case
      // the container is instantiated with a value type that
      // doesn't have a default constructor.

      std::pair<iterator, bool> emplace(
        boost::unordered::detail::empty_emplace =
          boost::unordered::detail::empty_emplace(),
        value_type v = value_type())
      {
        return this->emplace(boost::move(v));
      }

#endif

      template <typename A0>
      std::pair<iterator, bool> emplace(BOOST_FWD_REF(A0) a0)
      {
        return table_.emplace_unique(
          table::extractor::extract(boost::forward<A0>(a0)),
          boost::unordered::detail::create_emplace_args(
            boost::forward<A0>(a0)));
      }

      template <typename A0, typename A1>
      std::pair<iterator, bool> emplace(
        BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1)
      {
        return table_.emplace_unique(
          table::extractor::extract(
            boost::forward<A0>(a0), boost::forward<A1>(a1)),
          boost::unordered::detail::create_emplace_args(
            boost::forward<A0>(a0), boost::forward<A1>(a1)));
      }

      template <typename A0, typename A1, typename A2>
      std::pair<iterator, bool> emplace(
        BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
      {
        return table_.emplace_unique(
          table::extractor::extract(
            boost::forward<A0>(a0), boost::forward<A1>(a1)),
          boost::unordered::detail::create_emplace_args(boost::forward<A0>(a0),
            boost::forward<A1>(a1), boost::forward<A2>(a2)));
      }

#endif

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <class... Args>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(Args)... args)
      {
        return table_.emplace_hint_unique(hint,
          table::extractor::extract(boost::forward<Args>(args)...),
          boost::forward<Args>(args)...);
      }

#else

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1

      iterator emplace_hint(const_iterator hint,
        boost::unordered::detail::empty_emplace =
          boost::unordered::detail::empty_emplace(),
        value_type v = value_type())
      {
        return this->emplace_hint(hint, boost::move(v));
      }

#endif

      template <typename A0>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(A0) a0)
      {
        return table_.emplace_hint_unique(hint,
          table::extractor::extract(boost::forward<A0>(a0)),
          boost::unordered::detail::create_emplace_args(
            boost::forward<A0>(a0)));
      }

      template <typename A0, typename A1>
      iterator emplace_hint(
        const_iterator hint, BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1)
      {
        return table_.emplace_hint_unique(hint,
          table::extractor::extract(
            boost::forward<A0>(a0), boost::forward<A1>(a1)),
          boost::unordered::detail::create_emplace_args(
            boost::forward<A0>(a0), boost::forward<A1>(a1)));
      }

      template <typename A0, typename A1, typename A2>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(A0) a0,
        BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
      {
        return table_.emplace_hint_unique(hint,
          table::extractor::extract(
            boost::forward<A0>(a0), boost::forward<A1>(a1)),
          boost::unordered::detail::create_emplace_args(boost::forward<A0>(a0),
            boost::forward<A1>(a1), boost::forward<A2>(a2)));
      }

#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_EMPLACE(z, n, _)                                       \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  std::pair<iterator, bool> emplace(                                           \
    BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, a))                        \
  {                                                                            \
    return table_.emplace_unique(                                              \
      table::extractor::extract(                                               \
        boost::forward<A0>(a0), boost::forward<A1>(a1)),                       \
      boost::unordered::detail::create_emplace_args(                           \
        BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_CALL_FORWARD, a)));               \
  }                                                                            \
                                                                               \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  iterator emplace_hint(                                                       \
    const_iterator hint, BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, a))   \
  {                                                                            \
    return table_.emplace_hint_unique(hint,                                    \
      table::extractor::extract(                                               \
        boost::forward<A0>(a0), boost::forward<A1>(a1)),                       \
      boost::unordered::detail::create_emplace_args(                           \
        BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_CALL_FORWARD, a)));               \
  }

      BOOST_UNORDERED_EMPLACE(1, 4, _)
      BOOST_UNORDERED_EMPLACE(1, 5, _)
      BOOST_UNORDERED_EMPLACE(1, 6, _)
      BOOST_UNORDERED_EMPLACE(1, 7, _)
      BOOST_UNORDERED_EMPLACE(1, 8, _)
      BOOST_UNORDERED_EMPLACE(1, 9, _)
      BOOST_PP_REPEAT_FROM_TO(10, BOOST_PP_INC(BOOST_UNORDERED_EMPLACE_LIMIT),
        BOOST_UNORDERED_EMPLACE, _)

#undef BOOST_UNORDERED_EMPLACE

#endif

      std::pair<iterator, bool> insert(value_type const& x)
      {
        return this->emplace(x);
      }

      std::pair<iterator, bool> insert(BOOST_UNORDERED_RV_REF(value_type) x)
      {
        return this->emplace(boost::move(x));
      }

      iterator insert(const_iterator hint, value_type const& x)
      {
        return this->emplace_hint(hint, x);
      }

      iterator insert(const_iterator hint, BOOST_UNORDERED_RV_REF(value_type) x)
      {
        return this->emplace_hint(hint, boost::move(x));
      }

      template <class InputIt> void insert(InputIt, InputIt);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      void insert(std::initializer_list<value_type>);
#endif

      // extract

      node_type extract(const_iterator position)
      {
        return node_type(
          table_.extract_by_iterator_unique(position), table_.node_alloc());
      }

      node_type extract(const key_type& k)
      {
        return node_type(table_.extract_by_key(k), table_.node_alloc());
      }

      insert_return_type insert(BOOST_RV_REF(node_type) np)
      {
        insert_return_type result;
        table_.move_insert_node_type_unique(np, result);
        return boost::move(result);
      }

      iterator insert(const_iterator hint, BOOST_RV_REF(node_type) np)
      {
        return table_.move_insert_node_type_with_hint_unique(hint, np);
      }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) ||                               \
  (BOOST_COMP_GNUC && BOOST_COMP_GNUC < BOOST_VERSION_NUMBER(4, 6, 0))
    private:
      // Note: Use r-value node_type to insert.
      insert_return_type insert(node_type&);
      iterator insert(const_iterator, node_type& np);

    public:
#endif

      iterator erase(const_iterator);
      size_type erase(const key_type&);
      iterator erase(const_iterator, const_iterator);
      BOOST_UNORDERED_DEPRECATED("Use erase instead")
      void quick_erase(const_iterator it) { erase(it); }
      BOOST_UNORDERED_DEPRECATED("Use erase instead")
      void erase_return_void(const_iterator it) { erase(it); }

      void swap(unordered_set&)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_swappable<H>::value&&
              boost::is_nothrow_swappable<P>::value);
      void clear() BOOST_NOEXCEPT { table_.clear_impl(); }

      template <typename H2, typename P2>
      void merge(boost::unordered_set<T, H2, P2, A>& source);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename H2, typename P2>
      void merge(boost::unordered_set<T, H2, P2, A>&& source);
#endif

      template <typename H2, typename P2>
      void merge(boost::unordered_multiset<T, H2, P2, A>& source);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename H2, typename P2>
      void merge(boost::unordered_multiset<T, H2, P2, A>&& source);
#endif

      // observers

      hasher hash_function() const;
      key_equal key_eq() const;

      // lookup

      const_iterator find(const key_type&) const;

      template <class CompatibleKey, class CompatibleHash,
        class CompatiblePredicate>
      const_iterator find(CompatibleKey const&, CompatibleHash const&,
        CompatiblePredicate const&) const;

      size_type count(const key_type&) const;

      std::pair<const_iterator, const_iterator> equal_range(
        const key_type&) const;

      // bucket interface

      size_type bucket_count() const BOOST_NOEXCEPT
      {
        return table_.bucket_count_;
      }

      size_type max_bucket_count() const BOOST_NOEXCEPT
      {
        return table_.max_bucket_count();
      }

      size_type bucket_size(size_type) const;

      size_type bucket(const key_type& k) const
      {
        return table_.hash_to_bucket(table_.hash(k));
      }

      local_iterator begin(size_type n)
      {
        return local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      const_local_iterator begin(size_type n) const
      {
        return const_local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      local_iterator end(size_type) { return local_iterator(); }

      const_local_iterator end(size_type) const
      {
        return const_local_iterator();
      }

      const_local_iterator cbegin(size_type n) const
      {
        return const_local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      const_local_iterator cend(size_type) const
      {
        return const_local_iterator();
      }

      // hash policy

      float load_factor() const BOOST_NOEXCEPT;
      float max_load_factor() const BOOST_NOEXCEPT { return table_.mlf_; }
      void max_load_factor(float) BOOST_NOEXCEPT;
      void rehash(size_type);
      void reserve(size_type);

#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x0582)
      friend bool operator==
        <T, H, P, A>(unordered_set const&, unordered_set const&);
      friend bool operator!=
        <T, H, P, A>(unordered_set const&, unordered_set const&);
#endif
    }; // class template unordered_set

#if BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES

    template <class InputIterator,
      class Hash =
        boost::hash<typename std::iterator_traits<InputIterator>::value_type>,
      class Pred =
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
      class Allocator = std::allocator<
        typename std::iterator_traits<InputIterator>::value_type> >
    unordered_set(InputIterator, InputIterator,
      std::size_t = boost::unordered::detail::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      ->unordered_set<typename std::iterator_traits<InputIterator>::value_type,
        Hash, Pred, Allocator>;

    template <class T, class Hash = boost::hash<T>,
      class Pred = std::equal_to<T>, class Allocator = std::allocator<T> >
    unordered_set(std::initializer_list<T>,
      std::size_t = boost::unordered::detail::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      ->unordered_set<T, Hash, Pred, Allocator>;

    template <class InputIterator, class Allocator>
    unordered_set(InputIterator, InputIterator, std::size_t, Allocator)
      ->unordered_set<typename std::iterator_traits<InputIterator>::value_type,
        boost::hash<typename std::iterator_traits<InputIterator>::value_type>,
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
        Allocator>;

    template <class InputIterator, class Hash, class Allocator>
    unordered_set(InputIterator, InputIterator, std::size_t, Hash, Allocator)
      ->unordered_set<typename std::iterator_traits<InputIterator>::value_type,
        Hash,
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
        Allocator>;

    template <class T, class Allocator>
    unordered_set(std::initializer_list<T>, std::size_t, Allocator)
      ->unordered_set<T, boost::hash<T>, std::equal_to<T>, Allocator>;

    template <class T, class Hash, class Allocator>
    unordered_set(std::initializer_list<T>, std::size_t, Hash, Allocator)
      ->unordered_set<T, Hash, std::equal_to<T>, Allocator>;

#endif

    template <class T, class H, class P, class A> class unordered_multiset
    {
#if defined(BOOST_UNORDERED_USE_MOVE)
      BOOST_COPYABLE_AND_MOVABLE(unordered_multiset)
#endif
      template <typename, typename, typename, typename>
      friend class unordered_set;

    public:
      typedef T key_type;
      typedef T value_type;
      typedef H hasher;
      typedef P key_equal;
      typedef A allocator_type;

    private:
      typedef boost::unordered::detail::set<A, T, H, P> types;
      typedef typename types::value_allocator_traits value_allocator_traits;
      typedef typename types::table table;
      typedef typename table::node_pointer node_pointer;
      typedef typename table::link_pointer link_pointer;

    public:
      typedef typename value_allocator_traits::pointer pointer;
      typedef typename value_allocator_traits::const_pointer const_pointer;

      typedef value_type& reference;
      typedef value_type const& const_reference;

      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;

      typedef typename table::iterator iterator;
      typedef typename table::c_iterator const_iterator;
      typedef typename table::l_iterator local_iterator;
      typedef typename table::cl_iterator const_local_iterator;
      typedef typename types::node_type node_type;

    private:
      table table_;

    public:
      // constructors

      unordered_multiset();

      explicit unordered_multiset(size_type, const hasher& = hasher(),
        const key_equal& = key_equal(),
        const allocator_type& = allocator_type());

      template <class InputIt>
      unordered_multiset(InputIt, InputIt,
        size_type = boost::unordered::detail::default_bucket_count,
        const hasher& = hasher(), const key_equal& = key_equal(),
        const allocator_type& = allocator_type());

      unordered_multiset(unordered_multiset const&);

#if defined(BOOST_UNORDERED_USE_MOVE) ||                                       \
  !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      unordered_multiset(BOOST_RV_REF(unordered_multiset) other)
        BOOST_NOEXCEPT_IF(table::nothrow_move_constructible)
          : table_(other.table_, boost::unordered::detail::move_tag())
      {
        // The move is done in table_
      }
#endif

      explicit unordered_multiset(allocator_type const&);

      unordered_multiset(unordered_multiset const&, allocator_type const&);

      unordered_multiset(
        BOOST_RV_REF(unordered_multiset), allocator_type const&);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_multiset(std::initializer_list<value_type>,
        size_type = boost::unordered::detail::default_bucket_count,
        const hasher& = hasher(), const key_equal& l = key_equal(),
        const allocator_type& = allocator_type());
#endif

      explicit unordered_multiset(size_type, const allocator_type&);

      explicit unordered_multiset(
        size_type, const hasher&, const allocator_type&);

      template <class InputIt>
      unordered_multiset(InputIt, InputIt, size_type, const allocator_type&);

      template <class InputIt>
      unordered_multiset(
        InputIt, InputIt, size_type, const hasher&, const allocator_type&);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_multiset(
        std::initializer_list<value_type>, size_type, const allocator_type&);

      unordered_multiset(std::initializer_list<value_type>, size_type,
        const hasher&, const allocator_type&);
#endif

      // Destructor

      ~unordered_multiset() BOOST_NOEXCEPT;

// Assign

#if defined(BOOST_UNORDERED_USE_MOVE)
      unordered_multiset& operator=(BOOST_COPY_ASSIGN_REF(unordered_multiset) x)
      {
        table_.assign(x.table_, boost::unordered::detail::false_type());
        return *this;
      }

      unordered_multiset& operator=(BOOST_RV_REF(unordered_multiset) x)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_move_assignable<H>::value&&
              boost::is_nothrow_move_assignable<P>::value)
      {
        table_.move_assign(x.table_, boost::unordered::detail::false_type());
        return *this;
      }
#else
      unordered_multiset& operator=(unordered_multiset const& x)
      {
        table_.assign(x.table_, boost::unordered::detail::false_type());
        return *this;
      }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      unordered_multiset& operator=(unordered_multiset&& x)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_move_assignable<H>::value&&
              boost::is_nothrow_move_assignable<P>::value)
      {
        table_.move_assign(x.table_, boost::unordered::detail::false_type());
        return *this;
      }
#endif
#endif

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      unordered_multiset& operator=(std::initializer_list<value_type>);
#endif

      allocator_type get_allocator() const BOOST_NOEXCEPT
      {
        return table_.node_alloc();
      }

      // iterators

      iterator begin() BOOST_NOEXCEPT { return iterator(table_.begin()); }

      const_iterator begin() const BOOST_NOEXCEPT
      {
        return const_iterator(table_.begin());
      }

      iterator end() BOOST_NOEXCEPT { return iterator(); }

      const_iterator end() const BOOST_NOEXCEPT { return const_iterator(); }

      const_iterator cbegin() const BOOST_NOEXCEPT
      {
        return const_iterator(table_.begin());
      }

      const_iterator cend() const BOOST_NOEXCEPT { return const_iterator(); }

      // size and capacity

      bool empty() const BOOST_NOEXCEPT { return table_.size_ == 0; }

      size_type size() const BOOST_NOEXCEPT { return table_.size_; }

      size_type max_size() const BOOST_NOEXCEPT;

// emplace

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <class... Args> iterator emplace(BOOST_FWD_REF(Args)... args)
      {
        return iterator(table_.emplace_equiv(
          boost::unordered::detail::func::construct_node_from_args(
            table_.node_alloc(), boost::forward<Args>(args)...)));
      }

#else

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1

      // 0 argument emplace requires special treatment in case
      // the container is instantiated with a value type that
      // doesn't have a default constructor.

      iterator emplace(boost::unordered::detail::empty_emplace =
                         boost::unordered::detail::empty_emplace(),
        value_type v = value_type())
      {
        return this->emplace(boost::move(v));
      }

#endif

      template <typename A0> iterator emplace(BOOST_FWD_REF(A0) a0)
      {
        return iterator(table_.emplace_equiv(
          boost::unordered::detail::func::construct_node_from_args(
            table_.node_alloc(), boost::unordered::detail::create_emplace_args(
                                   boost::forward<A0>(a0)))));
      }

      template <typename A0, typename A1>
      iterator emplace(BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1)
      {
        return iterator(table_.emplace_equiv(
          boost::unordered::detail::func::construct_node_from_args(
            table_.node_alloc(),
            boost::unordered::detail::create_emplace_args(
              boost::forward<A0>(a0), boost::forward<A1>(a1)))));
      }

      template <typename A0, typename A1, typename A2>
      iterator emplace(
        BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
      {
        return iterator(table_.emplace_equiv(
          boost::unordered::detail::func::construct_node_from_args(
            table_.node_alloc(),
            boost::unordered::detail::create_emplace_args(
              boost::forward<A0>(a0), boost::forward<A1>(a1),
              boost::forward<A2>(a2)))));
      }

#endif

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

      template <class... Args>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(Args)... args)
      {
        return iterator(table_.emplace_hint_equiv(
          hint, boost::unordered::detail::func::construct_node_from_args(
                  table_.node_alloc(), boost::forward<Args>(args)...)));
      }

#else

#if !BOOST_UNORDERED_SUN_WORKAROUNDS1

      iterator emplace_hint(const_iterator hint,
        boost::unordered::detail::empty_emplace =
          boost::unordered::detail::empty_emplace(),
        value_type v = value_type())
      {
        return this->emplace_hint(hint, boost::move(v));
      }

#endif

      template <typename A0>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(A0) a0)
      {
        return iterator(table_.emplace_hint_equiv(hint,
          boost::unordered::detail::func::construct_node_from_args(
            table_.node_alloc(), boost::unordered::detail::create_emplace_args(
                                   boost::forward<A0>(a0)))));
      }

      template <typename A0, typename A1>
      iterator emplace_hint(
        const_iterator hint, BOOST_FWD_REF(A0) a0, BOOST_FWD_REF(A1) a1)
      {
        return iterator(table_.emplace_hint_equiv(
          hint, boost::unordered::detail::func::construct_node_from_args(
                  table_.node_alloc(),
                  boost::unordered::detail::create_emplace_args(
                    boost::forward<A0>(a0), boost::forward<A1>(a1)))));
      }

      template <typename A0, typename A1, typename A2>
      iterator emplace_hint(const_iterator hint, BOOST_FWD_REF(A0) a0,
        BOOST_FWD_REF(A1) a1, BOOST_FWD_REF(A2) a2)
      {
        return iterator(table_.emplace_hint_equiv(
          hint, boost::unordered::detail::func::construct_node_from_args(
                  table_.node_alloc(),
                  boost::unordered::detail::create_emplace_args(
                    boost::forward<A0>(a0), boost::forward<A1>(a1),
                    boost::forward<A2>(a2)))));
      }

#endif

#if defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#define BOOST_UNORDERED_EMPLACE(z, n, _)                                       \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  iterator emplace(BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, a))         \
  {                                                                            \
    return iterator(table_.emplace_equiv(                                      \
      boost::unordered::detail::func::construct_node_from_args(                \
        table_.node_alloc(),                                                   \
        boost::unordered::detail::create_emplace_args(                         \
          BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_CALL_FORWARD, a)))));           \
  }                                                                            \
                                                                               \
  template <BOOST_PP_ENUM_PARAMS_Z(z, n, typename A)>                          \
  iterator emplace_hint(                                                       \
    const_iterator hint, BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_FWD_PARAM, a))   \
  {                                                                            \
    return iterator(table_.emplace_hint_equiv(                                 \
      hint, boost::unordered::detail::func::construct_node_from_args(          \
              table_.node_alloc(),                                             \
              boost::unordered::detail::create_emplace_args(                   \
                BOOST_PP_ENUM_##z(n, BOOST_UNORDERED_CALL_FORWARD, a)))));     \
  }

      BOOST_UNORDERED_EMPLACE(1, 4, _)
      BOOST_UNORDERED_EMPLACE(1, 5, _)
      BOOST_UNORDERED_EMPLACE(1, 6, _)
      BOOST_UNORDERED_EMPLACE(1, 7, _)
      BOOST_UNORDERED_EMPLACE(1, 8, _)
      BOOST_UNORDERED_EMPLACE(1, 9, _)
      BOOST_PP_REPEAT_FROM_TO(10, BOOST_PP_INC(BOOST_UNORDERED_EMPLACE_LIMIT),
        BOOST_UNORDERED_EMPLACE, _)

#undef BOOST_UNORDERED_EMPLACE

#endif

      iterator insert(value_type const& x) { return this->emplace(x); }

      iterator insert(BOOST_UNORDERED_RV_REF(value_type) x)
      {
        return this->emplace(boost::move(x));
      }

      iterator insert(const_iterator hint, value_type const& x)
      {
        return this->emplace_hint(hint, x);
      }

      iterator insert(const_iterator hint, BOOST_UNORDERED_RV_REF(value_type) x)
      {
        return this->emplace_hint(hint, boost::move(x));
      }

      template <class InputIt> void insert(InputIt, InputIt);

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
      void insert(std::initializer_list<value_type>);
#endif

      // extract

      node_type extract(const_iterator position)
      {
        return node_type(
          table_.extract_by_iterator_equiv(position), table_.node_alloc());
      }

      node_type extract(const key_type& k)
      {
        return node_type(table_.extract_by_key(k), table_.node_alloc());
      }

      iterator insert(BOOST_RV_REF(node_type) np)
      {
        return table_.move_insert_node_type_equiv(np);
      }

      iterator insert(const_iterator hint, BOOST_RV_REF(node_type) np)
      {
        return table_.move_insert_node_type_with_hint_equiv(hint, np);
      }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES) ||                               \
  (BOOST_COMP_GNUC && BOOST_COMP_GNUC < BOOST_VERSION_NUMBER(4, 6, 0))
    private:
      // Note: Use r-value node_type to insert.
      iterator insert(node_type&);
      iterator insert(const_iterator, node_type& np);

    public:
#endif

      iterator erase(const_iterator);
      size_type erase(const key_type&);
      iterator erase(const_iterator, const_iterator);
      BOOST_UNORDERED_DEPRECATED("Use erase instead")
      void quick_erase(const_iterator it) { erase(it); }
      BOOST_UNORDERED_DEPRECATED("Use erase instead")
      void erase_return_void(const_iterator it) { erase(it); }

      void swap(unordered_multiset&)
        BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
            boost::is_nothrow_swappable<H>::value&&
              boost::is_nothrow_swappable<P>::value);
      void clear() BOOST_NOEXCEPT { table_.clear_impl(); }

      template <typename H2, typename P2>
      void merge(boost::unordered_multiset<T, H2, P2, A>& source);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename H2, typename P2>
      void merge(boost::unordered_multiset<T, H2, P2, A>&& source);
#endif

      template <typename H2, typename P2>
      void merge(boost::unordered_set<T, H2, P2, A>& source);

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename H2, typename P2>
      void merge(boost::unordered_set<T, H2, P2, A>&& source);
#endif

      // observers

      hasher hash_function() const;
      key_equal key_eq() const;

      // lookup

      const_iterator find(const key_type&) const;

      template <class CompatibleKey, class CompatibleHash,
        class CompatiblePredicate>
      const_iterator find(CompatibleKey const&, CompatibleHash const&,
        CompatiblePredicate const&) const;

      size_type count(const key_type&) const;

      std::pair<const_iterator, const_iterator> equal_range(
        const key_type&) const;

      // bucket interface

      size_type bucket_count() const BOOST_NOEXCEPT
      {
        return table_.bucket_count_;
      }

      size_type max_bucket_count() const BOOST_NOEXCEPT
      {
        return table_.max_bucket_count();
      }

      size_type bucket_size(size_type) const;

      size_type bucket(const key_type& k) const
      {
        return table_.hash_to_bucket(table_.hash(k));
      }

      local_iterator begin(size_type n)
      {
        return local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      const_local_iterator begin(size_type n) const
      {
        return const_local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      local_iterator end(size_type) { return local_iterator(); }

      const_local_iterator end(size_type) const
      {
        return const_local_iterator();
      }

      const_local_iterator cbegin(size_type n) const
      {
        return const_local_iterator(table_.begin(n), n, table_.bucket_count_);
      }

      const_local_iterator cend(size_type) const
      {
        return const_local_iterator();
      }

      // hash policy

      float load_factor() const BOOST_NOEXCEPT;
      float max_load_factor() const BOOST_NOEXCEPT { return table_.mlf_; }
      void max_load_factor(float) BOOST_NOEXCEPT;
      void rehash(size_type);
      void reserve(size_type);

#if !BOOST_WORKAROUND(BOOST_BORLANDC, < 0x0582)
      friend bool operator==
        <T, H, P, A>(unordered_multiset const&, unordered_multiset const&);
      friend bool operator!=
        <T, H, P, A>(unordered_multiset const&, unordered_multiset const&);
#endif
    }; // class template unordered_multiset

#if BOOST_UNORDERED_TEMPLATE_DEDUCTION_GUIDES

    template <class InputIterator,
      class Hash =
        boost::hash<typename std::iterator_traits<InputIterator>::value_type>,
      class Pred =
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
      class Allocator = std::allocator<
        typename std::iterator_traits<InputIterator>::value_type> >
    unordered_multiset(InputIterator, InputIterator,
      std::size_t = boost::unordered::detail::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      ->unordered_multiset<
        typename std::iterator_traits<InputIterator>::value_type, Hash, Pred,
        Allocator>;

    template <class T, class Hash = boost::hash<T>,
      class Pred = std::equal_to<T>, class Allocator = std::allocator<T> >
    unordered_multiset(std::initializer_list<T>,
      std::size_t = boost::unordered::detail::default_bucket_count,
      Hash = Hash(), Pred = Pred(), Allocator = Allocator())
      ->unordered_multiset<T, Hash, Pred, Allocator>;

    template <class InputIterator, class Allocator>
    unordered_multiset(InputIterator, InputIterator, std::size_t, Allocator)
      ->unordered_multiset<
        typename std::iterator_traits<InputIterator>::value_type,
        boost::hash<typename std::iterator_traits<InputIterator>::value_type>,
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
        Allocator>;

    template <class InputIterator, class Hash, class Allocator>
    unordered_multiset(
      InputIterator, InputIterator, std::size_t, Hash, Allocator)
      ->unordered_multiset<
        typename std::iterator_traits<InputIterator>::value_type, Hash,
        std::equal_to<typename std::iterator_traits<InputIterator>::value_type>,
        Allocator>;

    template <class T, class Allocator>
    unordered_multiset(std::initializer_list<T>, std::size_t, Allocator)
      ->unordered_multiset<T, boost::hash<T>, std::equal_to<T>, Allocator>;

    template <class T, class Hash, class Allocator>
    unordered_multiset(std::initializer_list<T>, std::size_t, Hash, Allocator)
      ->unordered_multiset<T, Hash, std::equal_to<T>, Allocator>;

#endif

    ////////////////////////////////////////////////////////////////////////////
    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set()
        : table_(boost::unordered::detail::default_bucket_count, hasher(),
            key_equal(), allocator_type())
    {
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(size_type n, const hasher& hf,
      const key_equal& eql, const allocator_type& a)
        : table_(n, hf, eql, a)
    {
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_set<T, H, P, A>::unordered_set(InputIt f, InputIt l, size_type n,
      const hasher& hf, const key_equal& eql, const allocator_type& a)
        : table_(boost::unordered::detail::initial_size(f, l, n), hf, eql, a)
    {
      this->insert(f, l);
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(unordered_set const& other)
        : table_(other.table_,
            unordered_set::value_allocator_traits::
              select_on_container_copy_construction(other.get_allocator()))
    {
      if (other.table_.size_) {
        table_.copy_buckets(
          other.table_, boost::unordered::detail::true_type());
      }
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(allocator_type const& a)
        : table_(boost::unordered::detail::default_bucket_count, hasher(),
            key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      unordered_set const& other, allocator_type const& a)
        : table_(other.table_, a)
    {
      if (other.table_.size_) {
        table_.copy_buckets(
          other.table_, boost::unordered::detail::true_type());
      }
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      BOOST_RV_REF(unordered_set) other, allocator_type const& a)
        : table_(other.table_, a, boost::unordered::detail::move_tag())
    {
      table_.move_construct_buckets(other.table_);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      std::initializer_list<value_type> list, size_type n, const hasher& hf,
      const key_equal& eql, const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hf, eql, a)
    {
      this->insert(list.begin(), list.end());
    }

#endif

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      size_type n, const allocator_type& a)
        : table_(n, hasher(), key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      size_type n, const hasher& hf, const allocator_type& a)
        : table_(n, hf, key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_set<T, H, P, A>::unordered_set(
      InputIt f, InputIt l, size_type n, const allocator_type& a)
        : table_(boost::unordered::detail::initial_size(f, l, n), hasher(),
            key_equal(), a)
    {
      this->insert(f, l);
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_set<T, H, P, A>::unordered_set(InputIt f, InputIt l, size_type n,
      const hasher& hf, const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(f, l, n), hf, key_equal(), a)
    {
      this->insert(f, l);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      std::initializer_list<value_type> list, size_type n,
      const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hasher(), key_equal(), a)
    {
      this->insert(list.begin(), list.end());
    }

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::unordered_set(
      std::initializer_list<value_type> list, size_type n, const hasher& hf,
      const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hf, key_equal(), a)
    {
      this->insert(list.begin(), list.end());
    }

#endif

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>::~unordered_set() BOOST_NOEXCEPT
    {
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_set<T, H, P, A>& unordered_set<T, H, P, A>::operator=(
      std::initializer_list<value_type> list)
    {
      this->clear();
      this->insert(list.begin(), list.end());
      return *this;
    }

#endif

    // size and capacity

    template <class T, class H, class P, class A>
    std::size_t unordered_set<T, H, P, A>::max_size() const BOOST_NOEXCEPT
    {
      using namespace std;

      // size < mlf_ * count
      return boost::unordered::detail::double_to_size(
               ceil(static_cast<double>(table_.mlf_) *
                    static_cast<double>(table_.max_bucket_count()))) -
             1;
    }

    // modifiers

    template <class T, class H, class P, class A>
    template <class InputIt>
    void unordered_set<T, H, P, A>::insert(InputIt first, InputIt last)
    {
      if (first != last) {
        table_.insert_range_unique(
          table::extractor::extract(*first), first, last);
      }
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <class T, class H, class P, class A>
    void unordered_set<T, H, P, A>::insert(
      std::initializer_list<value_type> list)
    {
      this->insert(list.begin(), list.end());
    }
#endif

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::iterator
    unordered_set<T, H, P, A>::erase(const_iterator position)
    {
      node_pointer node = table::get_node(position);
      BOOST_ASSERT(node);
      node_pointer next = table::next_node(node);
      table_.erase_nodes_unique(node, next);
      return iterator(next);
    }

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::size_type
    unordered_set<T, H, P, A>::erase(const key_type& k)
    {
      return table_.erase_key_unique(k);
    }

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::iterator
    unordered_set<T, H, P, A>::erase(const_iterator first, const_iterator last)
    {
      node_pointer last_node = table::get_node(last);
      if (first == last)
        return iterator(last_node);
      table_.erase_nodes_unique(table::get_node(first), last_node);
      return iterator(last_node);
    }

    template <class T, class H, class P, class A>
    void unordered_set<T, H, P, A>::swap(unordered_set& other)
      BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
          boost::is_nothrow_swappable<H>::value&&
            boost::is_nothrow_swappable<P>::value)
    {
      table_.swap(other.table_);
    }

    // observers

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::hasher
    unordered_set<T, H, P, A>::hash_function() const
    {
      return table_.hash_function();
    }

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::key_equal
    unordered_set<T, H, P, A>::key_eq() const
    {
      return table_.key_eq();
    }

    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_set<T, H, P, A>::merge(
      boost::unordered_set<T, H2, P2, A>& source)
    {
      table_.merge_unique(source.table_);
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_set<T, H, P, A>::merge(
      boost::unordered_set<T, H2, P2, A>&& source)
    {
      table_.merge_unique(source.table_);
    }
#endif

    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_set<T, H, P, A>::merge(
      boost::unordered_multiset<T, H2, P2, A>& source)
    {
      table_.merge_unique(source.table_);
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_set<T, H, P, A>::merge(
      boost::unordered_multiset<T, H2, P2, A>&& source)
    {
      table_.merge_unique(source.table_);
    }
#endif

    // lookup

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::const_iterator
    unordered_set<T, H, P, A>::find(const key_type& k) const
    {
      return const_iterator(table_.find_node(k));
    }

    template <class T, class H, class P, class A>
    template <class CompatibleKey, class CompatibleHash,
      class CompatiblePredicate>
    typename unordered_set<T, H, P, A>::const_iterator
    unordered_set<T, H, P, A>::find(CompatibleKey const& k,
      CompatibleHash const& hash, CompatiblePredicate const& eq) const
    {
      return const_iterator(
        table_.find_node_impl(table::policy::apply_hash(hash, k), k, eq));
    }

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::size_type
    unordered_set<T, H, P, A>::count(const key_type& k) const
    {
      return table_.find_node(k) ? 1 : 0;
    }

    template <class T, class H, class P, class A>
    std::pair<typename unordered_set<T, H, P, A>::const_iterator,
      typename unordered_set<T, H, P, A>::const_iterator>
    unordered_set<T, H, P, A>::equal_range(const key_type& k) const
    {
      node_pointer n = table_.find_node(k);
      return std::make_pair(
        const_iterator(n), const_iterator(n ? table::next_node(n) : n));
    }

    template <class T, class H, class P, class A>
    typename unordered_set<T, H, P, A>::size_type
    unordered_set<T, H, P, A>::bucket_size(size_type n) const
    {
      return table_.bucket_size(n);
    }

    // hash policy

    template <class T, class H, class P, class A>
    float unordered_set<T, H, P, A>::load_factor() const BOOST_NOEXCEPT
    {
      BOOST_ASSERT(table_.bucket_count_ != 0);
      return static_cast<float>(table_.size_) /
             static_cast<float>(table_.bucket_count_);
    }

    template <class T, class H, class P, class A>
    void unordered_set<T, H, P, A>::max_load_factor(float m) BOOST_NOEXCEPT
    {
      table_.max_load_factor(m);
    }

    template <class T, class H, class P, class A>
    void unordered_set<T, H, P, A>::rehash(size_type n)
    {
      table_.rehash(n);
    }

    template <class T, class H, class P, class A>
    void unordered_set<T, H, P, A>::reserve(size_type n)
    {
      table_.rehash(static_cast<std::size_t>(
        std::ceil(static_cast<double>(n) / table_.mlf_)));
    }

    template <class T, class H, class P, class A>
    inline bool operator==(
      unordered_set<T, H, P, A> const& m1, unordered_set<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_set<T, H, P, A> x;
      };
#endif
      return m1.table_.equals_unique(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline bool operator!=(
      unordered_set<T, H, P, A> const& m1, unordered_set<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_set<T, H, P, A> x;
      };
#endif
      return !m1.table_.equals_unique(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline void swap(
      unordered_set<T, H, P, A>& m1, unordered_set<T, H, P, A>& m2)
      BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(m1.swap(m2)))
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_set<T, H, P, A> x;
      };
#endif
      m1.swap(m2);
    }

    ////////////////////////////////////////////////////////////////////////////

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset()
        : table_(boost::unordered::detail::default_bucket_count, hasher(),
            key_equal(), allocator_type())
    {
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(size_type n,
      const hasher& hf, const key_equal& eql, const allocator_type& a)
        : table_(n, hf, eql, a)
    {
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_multiset<T, H, P, A>::unordered_multiset(InputIt f, InputIt l,
      size_type n, const hasher& hf, const key_equal& eql,
      const allocator_type& a)
        : table_(boost::unordered::detail::initial_size(f, l, n), hf, eql, a)
    {
      this->insert(f, l);
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      unordered_multiset const& other)
        : table_(other.table_,
            unordered_multiset::value_allocator_traits::
              select_on_container_copy_construction(other.get_allocator()))
    {
      if (other.table_.size_) {
        table_.copy_buckets(
          other.table_, boost::unordered::detail::false_type());
      }
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(allocator_type const& a)
        : table_(boost::unordered::detail::default_bucket_count, hasher(),
            key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      unordered_multiset const& other, allocator_type const& a)
        : table_(other.table_, a)
    {
      if (other.table_.size_) {
        table_.copy_buckets(
          other.table_, boost::unordered::detail::false_type());
      }
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      BOOST_RV_REF(unordered_multiset) other, allocator_type const& a)
        : table_(other.table_, a, boost::unordered::detail::move_tag())
    {
      table_.move_construct_buckets(other.table_);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      std::initializer_list<value_type> list, size_type n, const hasher& hf,
      const key_equal& eql, const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hf, eql, a)
    {
      this->insert(list.begin(), list.end());
    }

#endif

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      size_type n, const allocator_type& a)
        : table_(n, hasher(), key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      size_type n, const hasher& hf, const allocator_type& a)
        : table_(n, hf, key_equal(), a)
    {
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      InputIt f, InputIt l, size_type n, const allocator_type& a)
        : table_(boost::unordered::detail::initial_size(f, l, n), hasher(),
            key_equal(), a)
    {
      this->insert(f, l);
    }

    template <class T, class H, class P, class A>
    template <class InputIt>
    unordered_multiset<T, H, P, A>::unordered_multiset(InputIt f, InputIt l,
      size_type n, const hasher& hf, const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(f, l, n), hf, key_equal(), a)
    {
      this->insert(f, l);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      std::initializer_list<value_type> list, size_type n,
      const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hasher(), key_equal(), a)
    {
      this->insert(list.begin(), list.end());
    }

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::unordered_multiset(
      std::initializer_list<value_type> list, size_type n, const hasher& hf,
      const allocator_type& a)
        : table_(
            boost::unordered::detail::initial_size(list.begin(), list.end(), n),
            hf, key_equal(), a)
    {
      this->insert(list.begin(), list.end());
    }

#endif

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>::~unordered_multiset() BOOST_NOEXCEPT
    {
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)

    template <class T, class H, class P, class A>
    unordered_multiset<T, H, P, A>& unordered_multiset<T, H, P, A>::operator=(
      std::initializer_list<value_type> list)
    {
      this->clear();
      this->insert(list.begin(), list.end());
      return *this;
    }

#endif

    // size and capacity

    template <class T, class H, class P, class A>
    std::size_t unordered_multiset<T, H, P, A>::max_size() const BOOST_NOEXCEPT
    {
      using namespace std;

      // size < mlf_ * count
      return boost::unordered::detail::double_to_size(
               ceil(static_cast<double>(table_.mlf_) *
                    static_cast<double>(table_.max_bucket_count()))) -
             1;
    }

    // modifiers

    template <class T, class H, class P, class A>
    template <class InputIt>
    void unordered_multiset<T, H, P, A>::insert(InputIt first, InputIt last)
    {
      table_.insert_range_equiv(first, last);
    }

#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST)
    template <class T, class H, class P, class A>
    void unordered_multiset<T, H, P, A>::insert(
      std::initializer_list<value_type> list)
    {
      this->insert(list.begin(), list.end());
    }
#endif

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::iterator
    unordered_multiset<T, H, P, A>::erase(const_iterator position)
    {
      node_pointer node = table::get_node(position);
      BOOST_ASSERT(node);
      node_pointer next = table::next_node(node);
      table_.erase_nodes_equiv(node, next);
      return iterator(next);
    }

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::size_type
    unordered_multiset<T, H, P, A>::erase(const key_type& k)
    {
      return table_.erase_key_equiv(k);
    }

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::iterator
    unordered_multiset<T, H, P, A>::erase(
      const_iterator first, const_iterator last)
    {
      node_pointer last_node = table::get_node(last);
      if (first == last)
        return iterator(last_node);
      table_.erase_nodes_equiv(table::get_node(first), last_node);
      return iterator(last_node);
    }

    template <class T, class H, class P, class A>
    void unordered_multiset<T, H, P, A>::swap(unordered_multiset& other)
      BOOST_NOEXCEPT_IF(value_allocator_traits::is_always_equal::value&&
          boost::is_nothrow_swappable<H>::value&&
            boost::is_nothrow_swappable<P>::value)
    {
      table_.swap(other.table_);
    }

    // observers

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::hasher
    unordered_multiset<T, H, P, A>::hash_function() const
    {
      return table_.hash_function();
    }

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::key_equal
    unordered_multiset<T, H, P, A>::key_eq() const
    {
      return table_.key_eq();
    }

    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_multiset<T, H, P, A>::merge(
      boost::unordered_multiset<T, H2, P2, A>& source)
    {
      while (!source.empty()) {
        insert(source.extract(source.begin()));
      }
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_multiset<T, H, P, A>::merge(
      boost::unordered_multiset<T, H2, P2, A>&& source)
    {
      while (!source.empty()) {
        insert(source.extract(source.begin()));
      }
    }
#endif

    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_multiset<T, H, P, A>::merge(
      boost::unordered_set<T, H2, P2, A>& source)
    {
      while (!source.empty()) {
        insert(source.extract(source.begin()));
      }
    }

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <class T, class H, class P, class A>
    template <typename H2, typename P2>
    void unordered_multiset<T, H, P, A>::merge(
      boost::unordered_set<T, H2, P2, A>&& source)
    {
      while (!source.empty()) {
        insert(source.extract(source.begin()));
      }
    }
#endif

    // lookup

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::const_iterator
    unordered_multiset<T, H, P, A>::find(const key_type& k) const
    {
      return const_iterator(table_.find_node(k));
    }

    template <class T, class H, class P, class A>
    template <class CompatibleKey, class CompatibleHash,
      class CompatiblePredicate>
    typename unordered_multiset<T, H, P, A>::const_iterator
    unordered_multiset<T, H, P, A>::find(CompatibleKey const& k,
      CompatibleHash const& hash, CompatiblePredicate const& eq) const
    {
      return const_iterator(
        table_.find_node_impl(table::policy::apply_hash(hash, k), k, eq));
    }

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::size_type
    unordered_multiset<T, H, P, A>::count(const key_type& k) const
    {
      node_pointer n = table_.find_node(k);
      return n ? table_.group_count(n) : 0;
    }

    template <class T, class H, class P, class A>
    std::pair<typename unordered_multiset<T, H, P, A>::const_iterator,
      typename unordered_multiset<T, H, P, A>::const_iterator>
    unordered_multiset<T, H, P, A>::equal_range(const key_type& k) const
    {
      node_pointer n = table_.find_node(k);
      return std::make_pair(
        const_iterator(n), const_iterator(n ? table_.next_group(n) : n));
    }

    template <class T, class H, class P, class A>
    typename unordered_multiset<T, H, P, A>::size_type
    unordered_multiset<T, H, P, A>::bucket_size(size_type n) const
    {
      return table_.bucket_size(n);
    }

    // hash policy

    template <class T, class H, class P, class A>
    float unordered_multiset<T, H, P, A>::load_factor() const BOOST_NOEXCEPT
    {
      BOOST_ASSERT(table_.bucket_count_ != 0);
      return static_cast<float>(table_.size_) /
             static_cast<float>(table_.bucket_count_);
    }

    template <class T, class H, class P, class A>
    void unordered_multiset<T, H, P, A>::max_load_factor(float m) BOOST_NOEXCEPT
    {
      table_.max_load_factor(m);
    }

    template <class T, class H, class P, class A>
    void unordered_multiset<T, H, P, A>::rehash(size_type n)
    {
      table_.rehash(n);
    }

    template <class T, class H, class P, class A>
    void unordered_multiset<T, H, P, A>::reserve(size_type n)
    {
      table_.rehash(static_cast<std::size_t>(
        std::ceil(static_cast<double>(n) / table_.mlf_)));
    }

    template <class T, class H, class P, class A>
    inline bool operator==(unordered_multiset<T, H, P, A> const& m1,
      unordered_multiset<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_multiset<T, H, P, A> x;
      };
#endif
      return m1.table_.equals_equiv(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline bool operator!=(unordered_multiset<T, H, P, A> const& m1,
      unordered_multiset<T, H, P, A> const& m2)
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_multiset<T, H, P, A> x;
      };
#endif
      return !m1.table_.equals_equiv(m2.table_);
    }

    template <class T, class H, class P, class A>
    inline void swap(
      unordered_multiset<T, H, P, A>& m1, unordered_multiset<T, H, P, A>& m2)
      BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(m1.swap(m2)))
    {
#if BOOST_WORKAROUND(BOOST_CODEGEARC, BOOST_TESTED_AT(0x0613))
      struct dummy
      {
        unordered_multiset<T, H, P, A> x;
      };
#endif
      m1.swap(m2);
    }

    template <typename N, typename T, typename A> class node_handle_set
    {
      BOOST_MOVABLE_BUT_NOT_COPYABLE(node_handle_set)

      template <typename Types> friend struct ::boost::unordered::detail::table;
      template <class T2, class H2, class P2, class A2>
      friend class unordered_set;
      template <class T2, class H2, class P2, class A2>
      friend class unordered_multiset;

      typedef typename boost::unordered::detail::rebind_wrap<A, T>::type
        value_allocator;
      typedef boost::unordered::detail::allocator_traits<value_allocator>
        value_allocator_traits;
      typedef N node;
      typedef typename boost::unordered::detail::rebind_wrap<A, node>::type
        node_allocator;
      typedef boost::unordered::detail::allocator_traits<node_allocator>
        node_allocator_traits;
      typedef typename node_allocator_traits::pointer node_pointer;

    public:
      typedef T value_type;
      typedef A allocator_type;

    private:
      node_pointer ptr_;
      bool has_alloc_;
      boost::unordered::detail::optional<value_allocator> alloc_;

      node_handle_set(node_pointer ptr, allocator_type const& a)
          : ptr_(ptr), alloc_(a)
      {
      }

    public:
      BOOST_CONSTEXPR node_handle_set() BOOST_NOEXCEPT : ptr_(),
                                                         has_alloc_(false)
      {
      }

      ~node_handle_set()
      {
        if (ptr_) {
          node_allocator node_alloc(*alloc_);
          boost::unordered::detail::node_tmp<node_allocator> tmp(
            ptr_, node_alloc);
        }
      }

      node_handle_set(BOOST_RV_REF(node_handle_set) n) BOOST_NOEXCEPT
        : ptr_(n.ptr_),
          alloc_(boost::move(n.alloc_))
      {
        n.ptr_ = node_pointer();
      }

      node_handle_set& operator=(BOOST_RV_REF(node_handle_set) n)
      {
        BOOST_ASSERT(!alloc_.has_value() ||
                     value_allocator_traits::
                       propagate_on_container_move_assignment::value ||
                     (n.alloc_.has_value() && alloc_ == n.alloc_));

        if (ptr_) {
          node_allocator node_alloc(*alloc_);
          boost::unordered::detail::node_tmp<node_allocator> tmp(
            ptr_, node_alloc);
          ptr_ = node_pointer();
        }

        if (!alloc_.has_value() ||
            value_allocator_traits::propagate_on_container_move_assignment::
              value) {
          alloc_ = boost::move(n.alloc_);
        }
        ptr_ = n.ptr_;
        n.ptr_ = node_pointer();

        return *this;
      }

      value_type& value() const { return ptr_->value(); }

      allocator_type get_allocator() const { return *alloc_; }

      BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()

      bool operator!() const BOOST_NOEXCEPT { return ptr_ ? 0 : 1; }

      bool empty() const BOOST_NOEXCEPT { return ptr_ ? 0 : 1; }

      void swap(node_handle_set& n) BOOST_NOEXCEPT_IF(
        value_allocator_traits::propagate_on_container_swap::value ||
        value_allocator_traits::is_always_equal::value)
      {
        BOOST_ASSERT(
          !alloc_.has_value() || !n.alloc_.has_value() ||
          value_allocator_traits::propagate_on_container_swap::value ||
          alloc_ == n.alloc_);
        if (value_allocator_traits::propagate_on_container_swap::value ||
            !alloc_.has_value() || !n.alloc_.has_value()) {
          boost::swap(alloc_, n.alloc_);
        }
        boost::swap(ptr_, n.ptr_);
      }
    };

    template <typename N, typename T, typename A>
    void swap(node_handle_set<N, T, A>& x, node_handle_set<N, T, A>& y)
      BOOST_NOEXCEPT_IF(BOOST_NOEXCEPT_EXPR(x.swap(y)))
    {
      x.swap(y);
    }

    template <typename N, typename T, typename A> struct insert_return_type_set
    {
    private:
      BOOST_MOVABLE_BUT_NOT_COPYABLE(insert_return_type_set)

      typedef typename boost::unordered::detail::rebind_wrap<A, T>::type
        value_allocator;
      typedef N node_;

    public:
      bool inserted;
      boost::unordered::iterator_detail::c_iterator<node_> position;
      boost::unordered::node_handle_set<N, T, A> node;

      insert_return_type_set() : inserted(false), position(), node() {}

      insert_return_type_set(BOOST_RV_REF(insert_return_type_set)
          x) BOOST_NOEXCEPT : inserted(x.inserted),
                              position(x.position),
                              node(boost::move(x.node))
      {
      }

      insert_return_type_set& operator=(BOOST_RV_REF(insert_return_type_set) x)
      {
        inserted = x.inserted;
        position = x.position;
        node = boost::move(x.node);
        return *this;
      }
    };

    template <typename N, typename T, typename A>
    void swap(
      insert_return_type_set<N, T, A>& x, insert_return_type_set<N, T, A>& y)
    {
      boost::swap(x.node, y.node);
      boost::swap(x.inserted, y.inserted);
      boost::swap(x.position, y.position);
    }
  } // namespace unordered
} // namespace boost

#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif // BOOST_UNORDERED_UNORDERED_SET_HPP_INCLUDED
