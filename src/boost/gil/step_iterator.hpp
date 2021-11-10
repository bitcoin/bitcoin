//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_STEP_ITERATOR_HPP
#define BOOST_GIL_STEP_ITERATOR_HPP

#include <boost/gil/dynamic_step.hpp>
#include <boost/gil/pixel_iterator.hpp>
#include <boost/gil/pixel_iterator_adaptor.hpp>
#include <boost/gil/utilities.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace boost { namespace gil {

/// \defgroup PixelIteratorModelStepPtr step iterators
/// \ingroup PixelIteratorModel
/// \brief Iterators that allow for specifying the step between two adjacent values

namespace detail {

/// \ingroup PixelIteratorModelStepPtr
/// \brief An adaptor over an existing iterator that changes the step unit
///
/// (i.e. distance(it,it+1)) by a given predicate. Instead of calling base's
/// operators ++, --, +=, -=, etc. the adaptor is using the passed policy object SFn
/// for advancing and for computing the distance between iterators.

template <typename Derived,  // type of the derived class
          typename Iterator, // Models Iterator
          typename SFn>      // A policy object that can compute the distance between two iterators of type Iterator
                             // and can advance an iterator of type Iterator a given number of Iterator's units
class step_iterator_adaptor : public iterator_adaptor<Derived, Iterator, use_default, use_default, use_default, typename SFn::difference_type>
{
public:
    using parent_t = iterator_adaptor<Derived, Iterator, use_default, use_default, use_default, typename SFn::difference_type>;
    using base_difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using difference_type = typename SFn::difference_type;
    using reference = typename std::iterator_traits<Iterator>::reference;

    step_iterator_adaptor() {}
    step_iterator_adaptor(const Iterator& it, SFn step_fn=SFn()) : parent_t(it), _step_fn(step_fn) {}

    difference_type step() const { return _step_fn.step(); }

protected:
    SFn _step_fn;
private:
    friend class boost::iterator_core_access;

    void increment() { _step_fn.advance(this->base_reference(),1); }
    void decrement() { _step_fn.advance(this->base_reference(),-1); }
    void advance(base_difference_type d) { _step_fn.advance(this->base_reference(),d); }
    difference_type distance_to(const step_iterator_adaptor& it) const { return _step_fn.difference(this->base_reference(),it.base_reference()); }
};

// although iterator_adaptor defines these, the default implementation computes distance and compares for zero.
// it is often faster to just apply the relation operator to the base
template <typename D,typename Iterator,typename SFn> inline
bool operator>(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.step()>0 ? p1.base()> p2.base() : p1.base()< p2.base();
}

template <typename D,typename Iterator,typename SFn> inline
bool operator<(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.step()>0 ? p1.base()< p2.base() : p1.base()> p2.base();
}

template <typename D,typename Iterator,typename SFn> inline
bool operator>=(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.step()>0 ? p1.base()>=p2.base() : p1.base()<=p2.base();
}

template <typename D,typename Iterator,typename SFn> inline
bool operator<=(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.step()>0 ? p1.base()<=p2.base() : p1.base()>=p2.base();
}

template <typename D,typename Iterator,typename SFn> inline
bool operator==(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.base()==p2.base();
}

template <typename D,typename Iterator,typename SFn> inline
bool operator!=(const step_iterator_adaptor<D,Iterator,SFn>& p1, const step_iterator_adaptor<D,Iterator,SFn>& p2) {
    return p1.base()!=p2.base();
}

} // namespace detail

////////////////////////////////////////////////////////////////////////////////////////
///                 MEMORY-BASED STEP ITERATOR
////////////////////////////////////////////////////////////////////////////////////////

/// \class memory_based_step_iterator
/// \ingroup PixelIteratorModelStepPtr PixelBasedModel
/// \brief Iterator with dynamically specified step in memory units (bytes or bits). Models StepIteratorConcept, IteratorAdaptorConcept, MemoryBasedIteratorConcept, PixelIteratorConcept, HasDynamicXStepTypeConcept
///
/// A refinement of step_iterator_adaptor that uses a dynamic parameter for the step
/// which is specified in memory units, such as bytes or bits
///
/// Pixel step iterators are used to provide iteration over non-adjacent pixels.
/// Common use is a vertical traversal, where the step is the row stride.
///
/// Another application is as a sub-channel view. For example, a red intensity image over
/// interleaved RGB data would use a step iterator adaptor with step sizeof(channel_t)*3
/// In the latter example the step size could be fixed at compile time for efficiency.
/// Compile-time fixed step can be implemented by providing a step function object that takes the step as a template
////////////////////////////////////////////////////////////////////////////////////////

/// \ingroup PixelIteratorModelStepPtr
/// \brief function object that returns the memory unit distance between two iterators and advances a given iterator a given number of mem units (bytes or bits)
template <typename Iterator>
struct memunit_step_fn {
    using difference_type = std::ptrdiff_t;

    memunit_step_fn(difference_type step=memunit_step(Iterator())) : _step(step) {}

    difference_type difference(const Iterator& it1, const Iterator& it2) const { return memunit_distance(it1,it2)/_step; }
    void            advance(Iterator& it, difference_type d)             const { memunit_advance(it,d*_step); }
    difference_type step()                                               const { return _step; }

    void            set_step(std::ptrdiff_t step) { _step=step; }
private:
    BOOST_GIL_CLASS_REQUIRE(Iterator, boost::gil, MemoryBasedIteratorConcept)
    difference_type _step;
};

template <typename Iterator>
class memory_based_step_iterator : public detail::step_iterator_adaptor<memory_based_step_iterator<Iterator>,
                                                                            Iterator,
                                                                            memunit_step_fn<Iterator>>
{
    BOOST_GIL_CLASS_REQUIRE(Iterator, boost::gil, MemoryBasedIteratorConcept)
public:
    using parent_t = detail::step_iterator_adaptor<memory_based_step_iterator<Iterator>,
                                          Iterator,
                                          memunit_step_fn<Iterator>>;
    using reference = typename parent_t::reference;
    using difference_type = typename parent_t::difference_type;
    using x_iterator = Iterator;

    memory_based_step_iterator() : parent_t(Iterator()) {}
    memory_based_step_iterator(Iterator it, std::ptrdiff_t memunit_step) : parent_t(it, memunit_step_fn<Iterator>(memunit_step)) {}
    template <typename I2>
    memory_based_step_iterator(const memory_based_step_iterator<I2>& it)
        : parent_t(it.base(), memunit_step_fn<Iterator>(it.step())) {}

    /// For some reason operator[] provided by iterator_adaptor returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d) const { return *(*this+d); }

    void set_step(std::ptrdiff_t memunit_step) { this->_step_fn.set_step(memunit_step); }

    x_iterator& base()              { return parent_t::base_reference(); }
    x_iterator const& base() const  { return parent_t::base_reference(); }
};

template <typename Iterator>
struct const_iterator_type<memory_based_step_iterator<Iterator>> {
    using type = memory_based_step_iterator<typename const_iterator_type<Iterator>::type>;
};

template <typename Iterator>
struct iterator_is_mutable<memory_based_step_iterator<Iterator>> : public iterator_is_mutable<Iterator> {};


/////////////////////////////
//  IteratorAdaptorConcept
/////////////////////////////

template <typename Iterator>
struct is_iterator_adaptor<memory_based_step_iterator<Iterator>> : std::true_type {};

template <typename Iterator>
struct iterator_adaptor_get_base<memory_based_step_iterator<Iterator>>
{
    using type = Iterator;
};

template <typename Iterator, typename NewBaseIterator>
struct iterator_adaptor_rebind<memory_based_step_iterator<Iterator>, NewBaseIterator>
{
    using type = memory_based_step_iterator<NewBaseIterator>;
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename Iterator>
struct color_space_type<memory_based_step_iterator<Iterator>> : public color_space_type<Iterator> {};

template <typename Iterator>
struct channel_mapping_type<memory_based_step_iterator<Iterator>> : public channel_mapping_type<Iterator> {};

template <typename Iterator>
struct is_planar<memory_based_step_iterator<Iterator>> : public is_planar<Iterator> {};

template <typename Iterator>
struct channel_type<memory_based_step_iterator<Iterator>> : public channel_type<Iterator> {};

/////////////////////////////
//  MemoryBasedIteratorConcept
/////////////////////////////
template <typename Iterator>
struct byte_to_memunit<memory_based_step_iterator<Iterator>> : public byte_to_memunit<Iterator> {};

template <typename Iterator>
inline std::ptrdiff_t memunit_step(const memory_based_step_iterator<Iterator>& p) { return p.step(); }

template <typename Iterator>
inline std::ptrdiff_t memunit_distance(const memory_based_step_iterator<Iterator>& p1,
                                    const memory_based_step_iterator<Iterator>& p2) {
    return memunit_distance(p1.base(),p2.base());
}

template <typename Iterator>
inline void memunit_advance(memory_based_step_iterator<Iterator>& p,
                         std::ptrdiff_t diff) {
    memunit_advance(p.base(), diff);
}

template <typename Iterator>
inline memory_based_step_iterator<Iterator>
memunit_advanced(const memory_based_step_iterator<Iterator>& p,
              std::ptrdiff_t diff) {
    return memory_based_step_iterator<Iterator>(memunit_advanced(p.base(), diff),p.step());
}

template <typename Iterator>
inline typename std::iterator_traits<Iterator>::reference
memunit_advanced_ref(const memory_based_step_iterator<Iterator>& p,
                  std::ptrdiff_t diff) {
    return memunit_advanced_ref(p.base(), diff);
}

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename Iterator>
struct dynamic_x_step_type<memory_based_step_iterator<Iterator>> {
    using type = memory_based_step_iterator<Iterator>;
};

// For step iterators, pass the function object to the base
template <typename Iterator, typename Deref>
struct iterator_add_deref<memory_based_step_iterator<Iterator>,Deref> {
    BOOST_GIL_CLASS_REQUIRE(Deref, boost::gil, PixelDereferenceAdaptorConcept)

    using type = memory_based_step_iterator<typename iterator_add_deref<Iterator, Deref>::type>;

    static type make(const memory_based_step_iterator<Iterator>& it, const Deref& d) { return type(iterator_add_deref<Iterator, Deref>::make(it.base(),d),it.step()); }
};

////////////////////////////////////////////////////////////////////////////////////////
/// make_step_iterator
////////////////////////////////////////////////////////////////////////////////////////

template <typename I> typename dynamic_x_step_type<I>::type make_step_iterator(const I& it, std::ptrdiff_t step);

namespace detail {

// if the iterator is a plain base iterator (non-adaptor), wraps it in memory_based_step_iterator
template <typename I>
auto make_step_iterator_impl(I const& it, std::ptrdiff_t step, std::false_type)
    -> typename dynamic_x_step_type<I>::type
{
    return memory_based_step_iterator<I>(it, step);
}

// If the iterator is compound, put the step in its base
template <typename I>
auto make_step_iterator_impl(I const& it, std::ptrdiff_t step, std::true_type)
    -> typename dynamic_x_step_type<I>::type
{
    return make_step_iterator(it.base(), step);
}

// If the iterator is memory_based_step_iterator, change the step
template <typename BaseIt>
auto make_step_iterator_impl(
    memory_based_step_iterator<BaseIt> const& it,
    std::ptrdiff_t step,
    std::true_type)
    -> memory_based_step_iterator<BaseIt>
{
    return memory_based_step_iterator<BaseIt>(it.base(), step);
}

} // namespace detail

/// \brief Constructs a step iterator from a base iterator and a step.
///
/// To construct a step iterator from a given iterator Iterator and a given step, if Iterator does not
/// already have a dynamic step, we wrap it in a memory_based_step_iterator. Otherwise we
/// do a compile-time traversal of the chain of iterator adaptors to locate the step iterator
/// and then set it step to the new one.
///
/// The step iterator of Iterator is not always memory_based_step_iterator<Iterator>. For example, Iterator may
/// already be a memory_based_step_iterator, in which case it will be inefficient to stack them;
/// we can obtain the same result by multiplying their steps. Note that for Iterator to be a
/// step iterator it does not necessarily have to have the form memory_based_step_iterator<J>.
/// The step iterator can be wrapped inside another iterator. Also, it may not have the
/// type memory_based_step_iterator, but it could be a user-provided type.
template <typename I>  // Models MemoryBasedIteratorConcept, HasDynamicXStepTypeConcept
typename dynamic_x_step_type<I>::type make_step_iterator(const I& it, std::ptrdiff_t step) {
    return detail::make_step_iterator_impl(it, step, typename is_iterator_adaptor<I>::type());
}

}}  // namespace boost::gil

#endif
