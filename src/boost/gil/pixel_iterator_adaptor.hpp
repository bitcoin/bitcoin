//
// Copyright 2005-2007 Adobe Systems Incorporated
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
#ifndef BOOST_GIL_PIXEL_ITERATOR_ADAPTOR_HPP
#define BOOST_GIL_PIXEL_ITERATOR_ADAPTOR_HPP

#include <boost/gil/concepts.hpp>
#include <boost/gil/pixel_iterator.hpp>

#include <boost/iterator/iterator_facade.hpp>

#include <iterator>

namespace boost { namespace gil {

/// \defgroup PixelIteratorModelDerefPtr dereference_iterator_adaptor
/// \ingroup PixelIteratorModel
/// \brief An iterator that invokes a provided function object upon dereference. Models: IteratorAdaptorConcept, PixelIteratorConcept

/// \ingroup PixelIteratorModelDerefPtr PixelBasedModel
/// \brief An adaptor over an existing iterator that provides for custom filter on dereferencing the object. Models: IteratorAdaptorConcept, PixelIteratorConcept

template <typename Iterator,    // Models Iterator
          typename DFn>  // Models Returns the result of dereferencing a given iterator of type Iterator
class dereference_iterator_adaptor : public iterator_adaptor<dereference_iterator_adaptor<Iterator,DFn>,
                                                             Iterator,
                                                             typename DFn::value_type,
                                                             typename std::iterator_traits<Iterator>::iterator_category,
                                                             typename DFn::reference,
                                                             use_default> {
    DFn _deref_fn;
public:
    using parent_t = iterator_adaptor<dereference_iterator_adaptor<Iterator,DFn>,
                                    Iterator,
                                    typename DFn::value_type,
                                    typename std::iterator_traits<Iterator>::iterator_category,
                                    typename DFn::reference,
                                    use_default>;
    using reference = typename DFn::result_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using dereference_fn = DFn;

    dereference_iterator_adaptor() {}
    template <typename Iterator1>
    dereference_iterator_adaptor(const dereference_iterator_adaptor<Iterator1,DFn>& dit) : parent_t(dit.base()), _deref_fn(dit._deref_fn) {}
    dereference_iterator_adaptor(Iterator it, DFn deref_fn=DFn()) : parent_t(it), _deref_fn(deref_fn) {}
    template <typename Iterator1, typename DFn1>
    dereference_iterator_adaptor(const dereference_iterator_adaptor<Iterator1,DFn1>& it) : parent_t(it.base()), _deref_fn(it._deref_fn) {}
    /// For some reason operator[] provided by iterator_facade returns a custom class that is convertible to reference
    /// We require our own reference because it is registered in iterator_traits
    reference operator[](difference_type d) const { return *(*this+d);}

    // although iterator_adaptor defines these, the default implementation computes distance and compares for zero.
    // it is often faster to just apply the relation operator to the base
    bool    operator> (const dereference_iterator_adaptor& p) const { return this->base_reference()> p.base_reference(); }
    bool    operator< (const dereference_iterator_adaptor& p) const { return this->base_reference()< p.base_reference(); }
    bool    operator>=(const dereference_iterator_adaptor& p) const { return this->base_reference()>=p.base_reference(); }
    bool    operator<=(const dereference_iterator_adaptor& p) const { return this->base_reference()<=p.base_reference(); }
    bool    operator==(const dereference_iterator_adaptor& p) const { return this->base_reference()==p.base_reference(); }
    bool    operator!=(const dereference_iterator_adaptor& p) const { return this->base_reference()!=p.base_reference(); }

    Iterator& base()              { return this->base_reference(); }
    const Iterator& base() const  { return this->base_reference(); }
    const DFn& deref_fn() const { return _deref_fn; }
private:
    template <typename Iterator1, typename DFn1>
    friend class dereference_iterator_adaptor;
    friend class boost::iterator_core_access;

    reference dereference() const { return _deref_fn(*(this->base_reference())); }
};

template <typename I, typename DFn>
struct const_iterator_type<dereference_iterator_adaptor<I,DFn> > {
    using type = dereference_iterator_adaptor<typename const_iterator_type<I>::type,typename DFn::const_t>;
};

template <typename I, typename DFn>
struct iterator_is_mutable<dereference_iterator_adaptor<I, DFn>>
    : std::integral_constant<bool, DFn::is_mutable>
{};


template <typename I, typename DFn>
struct is_iterator_adaptor<dereference_iterator_adaptor<I, DFn>> : std::true_type {};

template <typename I, typename DFn>
struct iterator_adaptor_get_base<dereference_iterator_adaptor<I, DFn>>
{
    using type = I;
};

template <typename I, typename DFn, typename NewBaseIterator>
struct iterator_adaptor_rebind<dereference_iterator_adaptor<I,DFn>,NewBaseIterator> {
    using type = dereference_iterator_adaptor<NewBaseIterator,DFn>;
};

/////////////////////////////
//  PixelBasedConcept
/////////////////////////////

template <typename I, typename DFn>
struct color_space_type<dereference_iterator_adaptor<I,DFn> > : public color_space_type<typename DFn::value_type> {};

template <typename I, typename DFn>
struct channel_mapping_type<dereference_iterator_adaptor<I,DFn> > : public channel_mapping_type<typename DFn::value_type> {};

template <typename I, typename DFn>
struct is_planar<dereference_iterator_adaptor<I,DFn> > : public is_planar<typename DFn::value_type> {};

template <typename I, typename DFn>
struct channel_type<dereference_iterator_adaptor<I,DFn> > : public channel_type<typename DFn::value_type> {};


/////////////////////////////
//  MemoryBasedIteratorConcept
/////////////////////////////

template <typename Iterator, typename DFn>
struct byte_to_memunit<dereference_iterator_adaptor<Iterator,DFn> > : public byte_to_memunit<Iterator> {};

template <typename Iterator, typename DFn>
inline typename std::iterator_traits<Iterator>::difference_type
memunit_step(const dereference_iterator_adaptor<Iterator,DFn>& p) {
    return memunit_step(p.base());
}

template <typename Iterator, typename DFn>
inline typename std::iterator_traits<Iterator>::difference_type
memunit_distance(const dereference_iterator_adaptor<Iterator,DFn>& p1,
              const dereference_iterator_adaptor<Iterator,DFn>& p2) {
    return memunit_distance(p1.base(),p2.base());
}

template <typename Iterator, typename DFn>
inline void memunit_advance(dereference_iterator_adaptor<Iterator,DFn>& p,
                         typename std::iterator_traits<Iterator>::difference_type diff) {
    memunit_advance(p.base(), diff);
}

template <typename Iterator, typename DFn>
inline dereference_iterator_adaptor<Iterator,DFn>
memunit_advanced(const dereference_iterator_adaptor<Iterator,DFn>& p,
              typename std::iterator_traits<Iterator>::difference_type diff) {
    return dereference_iterator_adaptor<Iterator,DFn>(memunit_advanced(p.base(), diff), p.deref_fn());
}


template <typename Iterator, typename DFn>
inline
typename std::iterator_traits<dereference_iterator_adaptor<Iterator,DFn> >::reference
memunit_advanced_ref(const dereference_iterator_adaptor<Iterator,DFn>& p,
                  typename std::iterator_traits<Iterator>::difference_type diff) {
    return *memunit_advanced(p, diff);
}

/////////////////////////////
//  HasDynamicXStepTypeConcept
/////////////////////////////

template <typename Iterator, typename DFn>
struct dynamic_x_step_type<dereference_iterator_adaptor<Iterator,DFn> > {
    using type = dereference_iterator_adaptor<typename dynamic_x_step_type<Iterator>::type,DFn>;
};

/// \brief Returns the type (and creates an instance) of an iterator that invokes the given dereference adaptor upon dereferencing
/// \ingroup PixelIteratorModelDerefPtr
template <typename Iterator, typename Deref>
struct iterator_add_deref {
    BOOST_GIL_CLASS_REQUIRE(Deref, boost::gil, PixelDereferenceAdaptorConcept)

    using type = dereference_iterator_adaptor<Iterator, Deref>;

    static type make(const Iterator& it, const Deref& d) { return type(it,d); }
};

/// \ingroup PixelIteratorModelDerefPtr
/// \brief For dereference iterator adaptors, compose the new function object after the old one
template <typename Iterator, typename PREV_DEREF, typename Deref>
struct iterator_add_deref<dereference_iterator_adaptor<Iterator, PREV_DEREF>,Deref> {
//    BOOST_GIL_CLASS_REQUIRE(Deref, boost::gil, PixelDereferenceAdaptorConcept)

    using type = dereference_iterator_adaptor<Iterator, deref_compose<Deref,PREV_DEREF>>;

    static type make(const dereference_iterator_adaptor<Iterator, PREV_DEREF>& it, const Deref& d) {
        return type(it.base(),deref_compose<Deref,PREV_DEREF>(d,it.deref_fn()));
    }
};

}}  // namespace boost::gil

#endif
