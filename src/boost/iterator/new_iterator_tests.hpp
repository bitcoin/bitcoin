#ifndef BOOST_NEW_ITERATOR_TESTS_HPP
# define BOOST_NEW_ITERATOR_TESTS_HPP

//
// Copyright (c) David Abrahams 2001.
// Copyright (c) Jeremy Siek 2001-2003.
// Copyright (c) Thomas Witt 2002.
//
// Use, modification and distribution is subject to the
// Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

// This is meant to be the beginnings of a comprehensive, generic
// test suite for STL concepts such as iterators and containers.
//
// Revision History:
// 28 Oct 2002  Started update for new iterator categories
//              (Jeremy Siek)
// 28 Apr 2002  Fixed input iterator requirements.
//              For a == b a++ == b++ is no longer required.
//              See 24.1.1/3 for details.
//              (Thomas Witt)
// 08 Feb 2001  Fixed bidirectional iterator test so that
//              --i is no longer a precondition.
//              (Jeremy Siek)
// 04 Feb 2001  Added lvalue test, corrected preconditions
//              (David Abrahams)

# include <iterator>
# include <boost/static_assert.hpp>
# include <boost/concept_archetype.hpp> // for detail::dummy_constructor
# include <boost/pending/iterator_tests.hpp>
# include <boost/iterator/is_readable_iterator.hpp>
# include <boost/iterator/is_lvalue_iterator.hpp>
# include <boost/type_traits/is_same.hpp>
# include <boost/mpl/bool.hpp>
# include <boost/mpl/and.hpp>

# include <boost/iterator/detail/config_def.hpp>
# include <boost/detail/is_incrementable.hpp>
# include <boost/core/lightweight_test.hpp>

namespace boost {


// Do separate tests for *i++ so we can treat, e.g., smart pointers,
// as readable and/or writable iterators.
template <class Iterator, class T>
void readable_iterator_traversal_test(Iterator i1, T v, mpl::true_)
{
    T v2(*i1++);
    BOOST_TEST(v == v2);
}

template <class Iterator, class T>
void readable_iterator_traversal_test(const Iterator i1, T v, mpl::false_)
{}

template <class Iterator, class T>
void writable_iterator_traversal_test(Iterator i1, T v, mpl::true_)
{
    ++i1;  // we just wrote into that position
    *i1++ = v;
    Iterator x(i1++);
    (void)x;
}

template <class Iterator, class T>
void writable_iterator_traversal_test(const Iterator i1, T v, mpl::false_)
{}


// Preconditions: *i == v
template <class Iterator, class T>
void readable_iterator_test(const Iterator i1, T v)
{
  Iterator i2(i1); // Copy Constructible
  typedef typename std::iterator_traits<Iterator>::reference ref_t;
  ref_t r1 = *i1;
  ref_t r2 = *i2;
  T v1 = r1;
  T v2 = r2;
  BOOST_TEST(v1 == v);
  BOOST_TEST(v2 == v);

# if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
  readable_iterator_traversal_test(i1, v, detail::is_postfix_incrementable<Iterator>());
      
  // I think we don't really need this as it checks the same things as
  // the above code.
  BOOST_STATIC_ASSERT(is_readable_iterator<Iterator>::value);
# endif 
}

template <class Iterator, class T>
void writable_iterator_test(Iterator i, T v, T v2)
{
  Iterator i2(i); // Copy Constructible
  *i2 = v;

# if !BOOST_WORKAROUND(__MWERKS__, <= 0x2407)
  writable_iterator_traversal_test(
      i, v2, mpl::and_<
          detail::is_incrementable<Iterator>
        , detail::is_postfix_incrementable<Iterator>
      >());
# endif 
}

template <class Iterator>
void swappable_iterator_test(Iterator i, Iterator j)
{
  Iterator i2(i), j2(j);
  typename std::iterator_traits<Iterator>::value_type bi = *i, bj = *j;
  iter_swap(i2, j2);
  typename std::iterator_traits<Iterator>::value_type ai = *i, aj = *j;
  BOOST_TEST(bi == aj && bj == ai);
}

template <class Iterator, class T>
void constant_lvalue_iterator_test(Iterator i, T v1)
{
  Iterator i2(i);
  typedef typename std::iterator_traits<Iterator>::value_type value_type;
  typedef typename std::iterator_traits<Iterator>::reference reference;
  BOOST_STATIC_ASSERT((is_same<const value_type&, reference>::value));
  const T& v2 = *i2;
  BOOST_TEST(v1 == v2);
# ifndef BOOST_NO_LVALUE_RETURN_DETECTION
  BOOST_STATIC_ASSERT(is_lvalue_iterator<Iterator>::value);
  BOOST_STATIC_ASSERT(!is_non_const_lvalue_iterator<Iterator>::value);
# endif 
}

template <class Iterator, class T>
void non_const_lvalue_iterator_test(Iterator i, T v1, T v2)
{
  Iterator i2(i);
  typedef typename std::iterator_traits<Iterator>::value_type value_type;
  typedef typename std::iterator_traits<Iterator>::reference reference;
  BOOST_STATIC_ASSERT((is_same<value_type&, reference>::value));
  T& v3 = *i2;
  BOOST_TEST(v1 == v3);
  
  // A non-const lvalue iterator is not neccessarily writable, but we
  // are assuming the value_type is assignable here
  *i = v2;
  
  T& v4 = *i2;
  BOOST_TEST(v2 == v4);
# ifndef BOOST_NO_LVALUE_RETURN_DETECTION
  BOOST_STATIC_ASSERT(is_lvalue_iterator<Iterator>::value);
  BOOST_STATIC_ASSERT(is_non_const_lvalue_iterator<Iterator>::value);
# endif 
}

template <class Iterator, class T>
void forward_readable_iterator_test(Iterator i, Iterator j, T val1, T val2)
{
  Iterator i2;
  Iterator i3(i);
  i2 = i;
  BOOST_TEST(i2 == i3);
  BOOST_TEST(i != j);
  BOOST_TEST(i2 != j);
  readable_iterator_test(i, val1);
  readable_iterator_test(i2, val1);
  readable_iterator_test(i3, val1);

  BOOST_TEST(i == i2++);
  BOOST_TEST(i != ++i3);

  readable_iterator_test(i2, val2);
  readable_iterator_test(i3, val2);

  readable_iterator_test(i, val1);
}

template <class Iterator, class T>
void forward_swappable_iterator_test(Iterator i, Iterator j, T val1, T val2)
{
  forward_readable_iterator_test(i, j, val1, val2);
  Iterator i2 = i;
  ++i2;
  swappable_iterator_test(i, i2);
}

// bidirectional
// Preconditions: *i == v1, *++i == v2
template <class Iterator, class T>
void bidirectional_readable_iterator_test(Iterator i, T v1, T v2)
{
  Iterator j(i);
  ++j;
  forward_readable_iterator_test(i, j, v1, v2);
  ++i;

  Iterator i1 = i, i2 = i;

  BOOST_TEST(i == i1--);
  BOOST_TEST(i != --i2);

  readable_iterator_test(i, v2);
  readable_iterator_test(i1, v1);
  readable_iterator_test(i2, v1);

  --i;
  BOOST_TEST(i == i1);
  BOOST_TEST(i == i2);
  ++i1;
  ++i2;

  readable_iterator_test(i, v1);
  readable_iterator_test(i1, v2);
  readable_iterator_test(i2, v2);
}

// random access
// Preconditions: [i,i+N) is a valid range
template <class Iterator, class TrueVals>
void random_access_readable_iterator_test(Iterator i, int N, TrueVals vals)
{
  bidirectional_readable_iterator_test(i, vals[0], vals[1]);
  const Iterator j = i;
  int c;

  for (c = 0; c < N-1; ++c)
  {
    BOOST_TEST(i == j + c);
    BOOST_TEST(*i == vals[c]);
    typename std::iterator_traits<Iterator>::value_type x = j[c];
    BOOST_TEST(*i == x);
    BOOST_TEST(*i == *(j + c));
    BOOST_TEST(*i == *(c + j));
    ++i;
    BOOST_TEST(i > j);
    BOOST_TEST(i >= j);
    BOOST_TEST(j <= i);
    BOOST_TEST(j < i);
  }

  Iterator k = j + N - 1;
  for (c = 0; c < N-1; ++c)
  {
    BOOST_TEST(i == k - c);
    BOOST_TEST(*i == vals[N - 1 - c]);
    typename std::iterator_traits<Iterator>::value_type x = j[N - 1 - c];
    BOOST_TEST(*i == x);
    Iterator q = k - c; 
    BOOST_TEST(*i == *q);
    BOOST_TEST(i > j);
    BOOST_TEST(i >= j);
    BOOST_TEST(j <= i);
    BOOST_TEST(j < i);
    --i;
  }
}

} // namespace boost

# include <boost/iterator/detail/config_undef.hpp>

#endif // BOOST_NEW_ITERATOR_TESTS_HPP
