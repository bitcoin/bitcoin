// (C) Copyright Jeremy Siek 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_ITERATOR_CONCEPTS_HPP
#define BOOST_ITERATOR_CONCEPTS_HPP

#include <boost/concept_check.hpp>
#include <boost/iterator/iterator_categories.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_integral.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/or.hpp>

#include <boost/static_assert.hpp>

// Use boost/limits to work around missing limits headers on some compilers
#include <boost/limits.hpp>
#include <boost/config.hpp>

#include <algorithm>
#include <iterator>

#include <boost/concept/detail/concept_def.hpp>

namespace boost_concepts
{
  // Used a different namespace here (instead of "boost") so that the
  // concept descriptions do not take for granted the names in
  // namespace boost.

  //===========================================================================
  // Iterator Access Concepts

  BOOST_concept(ReadableIterator,(Iterator))
    : boost::Assignable<Iterator>
    , boost::CopyConstructible<Iterator>

  {
      typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::value_type value_type;
      typedef BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::reference reference;

      BOOST_CONCEPT_USAGE(ReadableIterator)
      {

          value_type v = *i;
          boost::ignore_unused_variable_warning(v);
      }
  private:
      Iterator i;
  };

  template <
      typename Iterator
    , typename ValueType = BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::value_type
  >
  struct WritableIterator
    : boost::CopyConstructible<Iterator>
  {
      BOOST_CONCEPT_USAGE(WritableIterator)
      {
          *i = v;
      }
  private:
      ValueType v;
      Iterator i;
  };

  template <
      typename Iterator
    , typename ValueType = BOOST_DEDUCED_TYPENAME std::iterator_traits<Iterator>::value_type
  >
  struct WritableIteratorConcept : WritableIterator<Iterator,ValueType> {};

  BOOST_concept(SwappableIterator,(Iterator))
  {
      BOOST_CONCEPT_USAGE(SwappableIterator)
      {
          std::iter_swap(i1, i2);
      }
  private:
      Iterator i1;
      Iterator i2;
  };

  BOOST_concept(LvalueIterator,(Iterator))
  {
      typedef typename std::iterator_traits<Iterator>::value_type value_type;

      BOOST_CONCEPT_USAGE(LvalueIterator)
      {
        value_type& r = const_cast<value_type&>(*i);
        boost::ignore_unused_variable_warning(r);
      }
  private:
      Iterator i;
  };


  //===========================================================================
  // Iterator Traversal Concepts

  BOOST_concept(IncrementableIterator,(Iterator))
    : boost::Assignable<Iterator>
    , boost::CopyConstructible<Iterator>
  {
      typedef typename boost::iterator_traversal<Iterator>::type traversal_category;

      BOOST_CONCEPT_ASSERT((
        boost::Convertible<
            traversal_category
          , boost::incrementable_traversal_tag
        >));

      BOOST_CONCEPT_USAGE(IncrementableIterator)
      {
          ++i;
          (void)i++;
      }
  private:
      Iterator i;
  };

  BOOST_concept(SinglePassIterator,(Iterator))
    : IncrementableIterator<Iterator>
    , boost::EqualityComparable<Iterator>

  {
      BOOST_CONCEPT_ASSERT((
          boost::Convertible<
             BOOST_DEDUCED_TYPENAME SinglePassIterator::traversal_category
           , boost::single_pass_traversal_tag
          > ));
  };

  BOOST_concept(ForwardTraversal,(Iterator))
    : SinglePassIterator<Iterator>
    , boost::DefaultConstructible<Iterator>
  {
      typedef typename std::iterator_traits<Iterator>::difference_type difference_type;

      BOOST_MPL_ASSERT((boost::is_integral<difference_type>));
      BOOST_MPL_ASSERT_RELATION(std::numeric_limits<difference_type>::is_signed, ==, true);

      BOOST_CONCEPT_ASSERT((
          boost::Convertible<
             BOOST_DEDUCED_TYPENAME ForwardTraversal::traversal_category
           , boost::forward_traversal_tag
          > ));
  };

  BOOST_concept(BidirectionalTraversal,(Iterator))
    : ForwardTraversal<Iterator>
  {
      BOOST_CONCEPT_ASSERT((
          boost::Convertible<
             BOOST_DEDUCED_TYPENAME BidirectionalTraversal::traversal_category
           , boost::bidirectional_traversal_tag
          > ));

      BOOST_CONCEPT_USAGE(BidirectionalTraversal)
      {
          --i;
          (void)i--;
      }
   private:
      Iterator i;
  };

  BOOST_concept(RandomAccessTraversal,(Iterator))
    : BidirectionalTraversal<Iterator>
  {
      BOOST_CONCEPT_ASSERT((
          boost::Convertible<
             BOOST_DEDUCED_TYPENAME RandomAccessTraversal::traversal_category
           , boost::random_access_traversal_tag
          > ));

      BOOST_CONCEPT_USAGE(RandomAccessTraversal)
      {
          i += n;
          i = i + n;
          i = n + i;
          i -= n;
          i = i - n;
          n = i - j;
      }

   private:
      typename BidirectionalTraversal<Iterator>::difference_type n;
      Iterator i, j;
  };

  //===========================================================================
  // Iterator Interoperability

  namespace detail
  {
    template <typename Iterator1, typename Iterator2>
    void interop_single_pass_constraints(Iterator1 const& i1, Iterator2 const& i2)
    {
        bool b;
        b = i1 == i2;
        b = i1 != i2;

        b = i2 == i1;
        b = i2 != i1;
        boost::ignore_unused_variable_warning(b);
    }

    template <typename Iterator1, typename Iterator2>
    void interop_rand_access_constraints(
        Iterator1 const& i1, Iterator2 const& i2,
        boost::random_access_traversal_tag, boost::random_access_traversal_tag)
    {
        bool b;
        typename std::iterator_traits<Iterator2>::difference_type n;
        b = i1 <  i2;
        b = i1 <= i2;
        b = i1 >  i2;
        b = i1 >= i2;
        n = i1 -  i2;

        b = i2 <  i1;
        b = i2 <= i1;
        b = i2 >  i1;
        b = i2 >= i1;
        n = i2 -  i1;
        boost::ignore_unused_variable_warning(b);
        boost::ignore_unused_variable_warning(n);
    }

    template <typename Iterator1, typename Iterator2>
    void interop_rand_access_constraints(
        Iterator1 const&, Iterator2 const&,
        boost::single_pass_traversal_tag, boost::single_pass_traversal_tag)
    { }

  } // namespace detail

  BOOST_concept(InteroperableIterator,(Iterator)(ConstIterator))
  {
   private:
      typedef typename boost::iterators::pure_iterator_traversal<Iterator>::type traversal_category;
      typedef typename boost::iterators::pure_iterator_traversal<ConstIterator>::type const_traversal_category;

   public:
      BOOST_CONCEPT_ASSERT((SinglePassIterator<Iterator>));
      BOOST_CONCEPT_ASSERT((SinglePassIterator<ConstIterator>));

      BOOST_CONCEPT_USAGE(InteroperableIterator)
      {
          detail::interop_single_pass_constraints(i, ci);
          detail::interop_rand_access_constraints(i, ci, traversal_category(), const_traversal_category());

          ci = i;
      }

   private:
      Iterator      i;
      ConstIterator ci;
  };

} // namespace boost_concepts

#include <boost/concept/detail/concept_undef.hpp>

#endif // BOOST_ITERATOR_CONCEPTS_HPP
