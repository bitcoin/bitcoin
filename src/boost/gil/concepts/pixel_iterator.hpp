//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_CONCEPTS_PIXEL_ITERATOR_HPP
#define BOOST_GIL_CONCEPTS_PIXEL_ITERATOR_HPP

#include <boost/gil/concepts/channel.hpp>
#include <boost/gil/concepts/color.hpp>
#include <boost/gil/concepts/concept_check.hpp>
#include <boost/gil/concepts/fwd.hpp>
#include <boost/gil/concepts/pixel.hpp>
#include <boost/gil/concepts/pixel_based.hpp>

#include <boost/iterator/iterator_concepts.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

#if defined(BOOST_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedefs"
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

namespace boost { namespace gil {

// Forward declarations
template <typename It> struct const_iterator_type;
template <typename It> struct iterator_is_mutable;
template <typename It> struct is_iterator_adaptor;
template <typename It, typename NewBaseIt> struct iterator_adaptor_rebind;
template <typename It> struct iterator_adaptor_get_base;

// These iterator mutability concepts are taken from Boost concept_check.hpp.
// Isolating mutability to result in faster compile time
namespace detail {

// Preconditions: TT Models boost_concepts::ForwardTraversalConcept
template <class TT>
struct ForwardIteratorIsMutableConcept
{
    void constraints()
    {
        auto const tmp = *i;
        *i++ = tmp; // require postincrement and assignment
    }
    TT i;
};

// Preconditions: TT Models boost::BidirectionalIteratorConcept
template <class TT>
struct BidirectionalIteratorIsMutableConcept
{
    void constraints()
    {
        gil_function_requires< ForwardIteratorIsMutableConcept<TT>>();
        auto const tmp = *i;
        *i-- = tmp; // require postdecrement and assignment
    }
    TT i;
};

// Preconditions: TT Models boost_concepts::RandomAccessTraversalConcept
template <class TT>
struct RandomAccessIteratorIsMutableConcept
{
    void constraints()
    {
        gil_function_requires<BidirectionalIteratorIsMutableConcept<TT>>();

        typename std::iterator_traits<TT>::difference_type n = 0;
        ignore_unused_variable_warning(n);
        i[n] = *i; // require element access and assignment
    }
    TT i;
};

// Iterators that can be used as the base of memory_based_step_iterator require some additional functions
// \tparam Iterator Models boost_concepts::RandomAccessTraversalConcept
template <typename Iterator>
struct RandomAccessIteratorIsMemoryBasedConcept
{
    void constraints()
    {
        std::ptrdiff_t bs = memunit_step(it);
        ignore_unused_variable_warning(bs);

        it = memunit_advanced(it, 3);
        std::ptrdiff_t bd = memunit_distance(it, it);
        ignore_unused_variable_warning(bd);

        memunit_advance(it,3);
        // for performace you may also provide a customized implementation of memunit_advanced_ref
    }
    Iterator it;
};

/// \tparam Iterator Models PixelIteratorConcept
template <typename Iterator>
struct PixelIteratorIsMutableConcept
{
    void constraints()
    {
        gil_function_requires<detail::RandomAccessIteratorIsMutableConcept<Iterator>>();

        using ref_t = typename std::remove_reference
            <
                typename std::iterator_traits<Iterator>::reference
            >::type;
        using channel_t = typename element_type<ref_t>::type;
        gil_function_requires<detail::ChannelIsMutableConcept<channel_t>>();
    }
};

} // namespace detail

/// \ingroup PixelLocatorConcept
/// \brief Concept for locators and views that can define a type just like the given locator or view, except X and Y is swapped
/// \code
/// concept HasTransposedTypeConcept<typename T>
/// {
///     typename transposed_type<T>;
///         where Metafunction<transposed_type<T> >;
/// };
/// \endcode
template <typename T>
struct HasTransposedTypeConcept
{
    void constraints()
    {
        using type = typename transposed_type<T>::type;
        ignore_unused_variable_warning(type{});
    }
};

/// \defgroup PixelIteratorConceptPixelIterator PixelIteratorConcept
/// \ingroup PixelIteratorConcept
/// \brief STL iterator over pixels

/// \ingroup PixelIteratorConceptPixelIterator
/// \brief An STL random access traversal iterator over a model of PixelConcept.
///
/// GIL's iterators must also provide the following metafunctions:
///  - \p const_iterator_type<Iterator>:   Returns a read-only equivalent of \p Iterator
///  - \p iterator_is_mutable<Iterator>:   Returns whether the given iterator is read-only or mutable
///  - \p is_iterator_adaptor<Iterator>:   Returns whether the given iterator is an adaptor over another iterator. See IteratorAdaptorConcept for additional requirements of adaptors.
///
/// \code
/// concept PixelIteratorConcept<typename Iterator>
///     : boost_concepts::RandomAccessTraversalConcept<Iterator>, PixelBasedConcept<Iterator>
/// {
///     where PixelValueConcept<value_type>;
///     typename const_iterator_type<It>::type;
///         where PixelIteratorConcept<const_iterator_type<It>::type>;
///     static const bool  iterator_is_mutable<It>::value;
///     static const bool  is_iterator_adaptor<It>::value;   // is it an iterator adaptor
/// };
/// \endcode
template <typename Iterator>
struct PixelIteratorConcept
{
    void constraints()
    {
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<Iterator>>();
        gil_function_requires<PixelBasedConcept<Iterator>>();

        using value_type = typename std::iterator_traits<Iterator>::value_type;
        gil_function_requires<PixelValueConcept<value_type>>();

        using const_t = typename const_iterator_type<Iterator>::type;
        static bool const is_mutable = iterator_is_mutable<Iterator>::value;
        ignore_unused_variable_warning(is_mutable);

        // immutable iterator must be constructible from (possibly mutable) iterator
        const_t const_it(it);
        ignore_unused_variable_warning(const_it);

        check_base(typename is_iterator_adaptor<Iterator>::type());
    }

    void check_base(std::false_type) {}

    void check_base(std::true_type)
    {
        using base_t = typename iterator_adaptor_get_base<Iterator>::type;
        gil_function_requires<PixelIteratorConcept<base_t>>();
    }

    Iterator it;
};

/// \brief Pixel iterator that allows for changing its pixel
/// \ingroup PixelIteratorConceptPixelIterator
/// \code
/// concept MutablePixelIteratorConcept<PixelIteratorConcept Iterator>
///     : MutableRandomAccessIteratorConcept<Iterator> {};
/// \endcode
template <typename Iterator>
struct MutablePixelIteratorConcept
{
    void constraints()
    {
        gil_function_requires<PixelIteratorConcept<Iterator>>();
        gil_function_requires<detail::PixelIteratorIsMutableConcept<Iterator>>();
    }
};

/// \defgroup PixelIteratorConceptStepIterator StepIteratorConcept
/// \ingroup PixelIteratorConcept
/// \brief Iterator that advances by a specified step

/// \ingroup PixelIteratorConceptStepIterator
/// \brief Concept of a random-access iterator that can be advanced in memory units (bytes or bits)
/// \code
/// concept MemoryBasedIteratorConcept<boost_concepts::RandomAccessTraversalConcept Iterator>
/// {
///     typename byte_to_memunit<Iterator>; where metafunction<byte_to_memunit<Iterator> >;
///     std::ptrdiff_t      memunit_step(const Iterator&);
///     std::ptrdiff_t      memunit_distance(const Iterator& , const Iterator&);
///     void                memunit_advance(Iterator&, std::ptrdiff_t diff);
///     Iterator            memunit_advanced(const Iterator& p, std::ptrdiff_t diff) { Iterator tmp; memunit_advance(tmp,diff); return tmp; }
///     Iterator::reference memunit_advanced_ref(const Iterator& p, std::ptrdiff_t diff) { return *memunit_advanced(p,diff); }
/// };
/// \endcode
template <typename Iterator>
struct MemoryBasedIteratorConcept
{
    void constraints()
    {
        gil_function_requires<boost_concepts::RandomAccessTraversalConcept<Iterator>>();
        gil_function_requires<detail::RandomAccessIteratorIsMemoryBasedConcept<Iterator>>();
    }
};

/// \ingroup PixelIteratorConceptStepIterator
/// \brief Step iterator concept
///
/// Step iterators are iterators that have a set_step method
/// \code
/// concept StepIteratorConcept<boost_concepts::ForwardTraversalConcept Iterator>
/// {
///     template <Integral D>
///     void Iterator::set_step(D step);
/// };
/// \endcode
template <typename Iterator>
struct StepIteratorConcept
{
    void constraints()
    {
        gil_function_requires<boost_concepts::ForwardTraversalConcept<Iterator>>();
        it.set_step(0);
    }
    Iterator it;
};


/// \ingroup PixelIteratorConceptStepIterator
/// \brief Step iterator that allows for modifying its current value
/// \code
/// concept MutableStepIteratorConcept<Mutable_ForwardIteratorConcept Iterator>
///     : StepIteratorConcept<Iterator> {};
/// \endcode
template <typename Iterator>
struct MutableStepIteratorConcept
{
    void constraints()
    {
        gil_function_requires<StepIteratorConcept<Iterator>>();
        gil_function_requires<detail::ForwardIteratorIsMutableConcept<Iterator>>();
    }
};

/// \defgroup PixelIteratorConceptIteratorAdaptor IteratorAdaptorConcept
/// \ingroup PixelIteratorConcept
/// \brief Adaptor over another iterator

/// \ingroup PixelIteratorConceptIteratorAdaptor
/// \brief Iterator adaptor is a forward iterator adapting another forward iterator.
///
/// In addition to GIL iterator requirements,
/// GIL iterator adaptors must provide the following metafunctions:
///  - \p is_iterator_adaptor<Iterator>:             Returns \p std::true_type
///  - \p iterator_adaptor_get_base<Iterator>:       Returns the base iterator type
///  - \p iterator_adaptor_rebind<Iterator,NewBase>: Replaces the base iterator with the new one
///
/// The adaptee can be obtained from the iterator via the "base()" method.
///
/// \code
/// concept IteratorAdaptorConcept<boost_concepts::ForwardTraversalConcept Iterator>
/// {
///     where SameType<is_iterator_adaptor<Iterator>::type, std::true_type>;
///
///     typename iterator_adaptor_get_base<Iterator>;
///         where Metafunction<iterator_adaptor_get_base<Iterator> >;
///         where boost_concepts::ForwardTraversalConcept<iterator_adaptor_get_base<Iterator>::type>;
///
///     typename another_iterator;
///     typename iterator_adaptor_rebind<Iterator,another_iterator>::type;
///         where boost_concepts::ForwardTraversalConcept<another_iterator>;
///         where IteratorAdaptorConcept<iterator_adaptor_rebind<Iterator,another_iterator>::type>;
///
///     const iterator_adaptor_get_base<Iterator>::type& Iterator::base() const;
/// };
/// \endcode
template <typename Iterator>
struct IteratorAdaptorConcept
{
    void constraints()
    {
        gil_function_requires<boost_concepts::ForwardTraversalConcept<Iterator>>();

        using base_t = typename iterator_adaptor_get_base<Iterator>::type;
        gil_function_requires<boost_concepts::ForwardTraversalConcept<base_t>>();

        static_assert(is_iterator_adaptor<Iterator>::value, "");
        using rebind_t = typename iterator_adaptor_rebind<Iterator, void*>::type;

        base_t base = it.base();
        ignore_unused_variable_warning(base);
    }
    Iterator it;
};

/// \brief Iterator adaptor that is mutable
/// \ingroup PixelIteratorConceptIteratorAdaptor
/// \code
/// concept MutableIteratorAdaptorConcept<Mutable_ForwardIteratorConcept Iterator>
///     : IteratorAdaptorConcept<Iterator> {};
/// \endcode
template <typename Iterator>
struct MutableIteratorAdaptorConcept
{
    void constraints()
    {
        gil_function_requires<IteratorAdaptorConcept<Iterator>>();
        gil_function_requires<detail::ForwardIteratorIsMutableConcept<Iterator>>();
    }
};

}} // namespace boost::gil

#if defined(BOOST_CLANG)
#pragma clang diagnostic pop
#endif

#if defined(BOOST_GCC) && (BOOST_GCC >= 40900)
#pragma GCC diagnostic pop
#endif

#endif
