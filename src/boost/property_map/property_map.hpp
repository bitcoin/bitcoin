//  (C) Copyright Jeremy Siek 1999-2001.
//  Copyright (C) 2006 Trustees of Indiana University
//  Authors: Douglas Gregor and Jeremy Siek

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/property_map for documentation.

#ifndef BOOST_PROPERTY_MAP_HPP
#define BOOST_PROPERTY_MAP_HPP

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <cstddef>
#include <iterator>
#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <boost/concept_archetype.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/has_xxx.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost {

  //=========================================================================
  // property_traits class

  BOOST_MPL_HAS_XXX_TRAIT_DEF(key_type)
  BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)
  BOOST_MPL_HAS_XXX_TRAIT_DEF(reference)
  BOOST_MPL_HAS_XXX_TRAIT_DEF(category)
 
  template<class PA>
  struct is_property_map :
    boost::mpl::and_<
      has_key_type<PA>,
      has_value_type<PA>,
      has_reference<PA>,
      has_category<PA>
    >
  {};
 
  template <typename PA>
  struct default_property_traits {
    typedef typename PA::key_type key_type;
    typedef typename PA::value_type value_type;
    typedef typename PA::reference reference;
    typedef typename PA::category   category;
  };
 
  struct null_property_traits {};
 
  template <typename PA>
  struct property_traits :
    boost::mpl::if_<is_property_map<PA>,
      default_property_traits<PA>,
      null_property_traits>::type
  {};

#if 0
  template <typename PA>
  struct property_traits {
    typedef typename PA::key_type key_type;
    typedef typename PA::value_type value_type; 
    typedef typename PA::reference reference;
    typedef typename PA::category   category;
  };
#endif

  //=========================================================================
  // property_traits category tags

  namespace detail {
    enum ePropertyMapID { READABLE_PA, WRITABLE_PA, 
                          READ_WRITE_PA, LVALUE_PA, OP_BRACKET_PA, 
                          RAND_ACCESS_ITER_PA, LAST_PA };
  }
  struct readable_property_map_tag { enum { id = detail::READABLE_PA }; };
  struct writable_property_map_tag { enum { id = detail::WRITABLE_PA }; };
  struct read_write_property_map_tag :
    public readable_property_map_tag,
    public writable_property_map_tag
  { enum { id = detail::READ_WRITE_PA }; };

  struct lvalue_property_map_tag : public read_write_property_map_tag
  { enum { id = detail::LVALUE_PA }; };

  //=========================================================================
  // property_traits specialization for pointers

  template <class T>
  struct property_traits<T*> {
    // BOOST_STATIC_ASSERT(boost::is_same<T, T*>::value && !"Using pointers as property maps is deprecated");
    typedef T value_type;
    typedef value_type& reference;
    typedef std::ptrdiff_t key_type;
    typedef lvalue_property_map_tag category;
  };
  template <class T>
  struct property_traits<const T*> {
    // BOOST_STATIC_ASSERT(boost::is_same<T, T*>::value && !"Using pointers as property maps is deprecated");
    typedef T value_type;
    typedef const value_type& reference;
    typedef std::ptrdiff_t key_type;
    typedef lvalue_property_map_tag category;
  };

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
  // MSVC doesn't have Koenig lookup, so the user has to
  // do boost::get() anyways, and the using clause
  // doesn't really work for MSVC.
} // namespace boost
#endif

  // These need to go in global namespace because Koenig
  // lookup does not apply to T*.

  // V must be convertible to T
  template <class T, class V>
  inline void put(T* pa, std::ptrdiff_t k, const V& val) { pa[k] = val;  }

  template <class T>
  inline const T& get(const T* pa, std::ptrdiff_t k) { return pa[k]; }

#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost {
  using ::put;
  using ::get;
#endif

  //=========================================================================
  // concept checks for property maps

  template <class PMap, class Key>
  struct ReadablePropertyMapConcept
  {
    typedef typename property_traits<PMap>::key_type key_type;
    typedef typename property_traits<PMap>::reference reference;
    typedef typename property_traits<PMap>::category Category;
    typedef boost::readable_property_map_tag ReadableTag;
    void constraints() {
      BOOST_CONCEPT_ASSERT((ConvertibleConcept<Category, ReadableTag>));

      val = get(pmap, k);
    }
    PMap pmap;
    Key k;
    typename property_traits<PMap>::value_type val;
  };
  template <typename KeyArchetype, typename ValueArchetype>
  struct readable_property_map_archetype {
    typedef KeyArchetype key_type;
    typedef ValueArchetype value_type;
    typedef convertible_to_archetype<ValueArchetype> reference;
    typedef readable_property_map_tag category;
  };
  template <typename K, typename V>
  const typename readable_property_map_archetype<K,V>::reference&
  get(const readable_property_map_archetype<K,V>&, 
      const typename readable_property_map_archetype<K,V>::key_type&)
  {
    typedef typename readable_property_map_archetype<K,V>::reference R;
    return static_object<R>::get();
  }


  template <class PMap, class Key>
  struct WritablePropertyMapConcept
  {
    typedef typename property_traits<PMap>::key_type key_type;
    typedef typename property_traits<PMap>::category Category;
    typedef boost::writable_property_map_tag WritableTag;
    void constraints() {
      BOOST_CONCEPT_ASSERT((ConvertibleConcept<Category, WritableTag>));
      put(pmap, k, val);
    }
    PMap pmap;
    Key k;
    typename property_traits<PMap>::value_type val;
  };
  template <typename KeyArchetype, typename ValueArchetype>
  struct writable_property_map_archetype {
    typedef KeyArchetype key_type;
    typedef ValueArchetype value_type;
    typedef void reference;
    typedef writable_property_map_tag category;
  };
  template <typename K, typename V>
  void put(const writable_property_map_archetype<K,V>&, 
           const typename writable_property_map_archetype<K,V>::key_type&, 
           const typename writable_property_map_archetype<K,V>::value_type&) { }


  template <class PMap, class Key>
  struct ReadWritePropertyMapConcept
  {
    typedef typename property_traits<PMap>::category Category;
    typedef boost::read_write_property_map_tag ReadWriteTag;
    void constraints() {
      BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<PMap, Key>));
      BOOST_CONCEPT_ASSERT((WritablePropertyMapConcept<PMap, Key>));
      BOOST_CONCEPT_ASSERT((ConvertibleConcept<Category, ReadWriteTag>));
    }
  };
  template <typename KeyArchetype, typename ValueArchetype>
  struct read_write_property_map_archetype
    : public readable_property_map_archetype<KeyArchetype, ValueArchetype>,
      public writable_property_map_archetype<KeyArchetype, ValueArchetype>
  {
    typedef KeyArchetype key_type;
    typedef ValueArchetype value_type;
    typedef convertible_to_archetype<ValueArchetype> reference;
    typedef read_write_property_map_tag category;
  };


  template <class PMap, class Key>
  struct LvaluePropertyMapConcept
  {
    typedef typename property_traits<PMap>::category Category;
    typedef boost::lvalue_property_map_tag LvalueTag;
    typedef typename property_traits<PMap>::reference reference;

    void constraints() {
      BOOST_CONCEPT_ASSERT((ReadablePropertyMapConcept<PMap, Key>));
      BOOST_CONCEPT_ASSERT((ConvertibleConcept<Category, LvalueTag>));

      typedef typename property_traits<PMap>::value_type value_type;
      BOOST_MPL_ASSERT((boost::mpl::or_<
                          boost::is_same<const value_type&, reference>,
                          boost::is_same<value_type&, reference> >));

      reference ref = pmap[k];
      ignore_unused_variable_warning(ref);
    }
    PMap pmap;
    Key k;
  };
  template <typename KeyArchetype, typename ValueArchetype>
  struct lvalue_property_map_archetype
    : public readable_property_map_archetype<KeyArchetype, ValueArchetype>
  {
    typedef KeyArchetype key_type;
    typedef ValueArchetype value_type;
    typedef const ValueArchetype& reference;
    typedef lvalue_property_map_tag category;
    const value_type& operator[](const key_type&) const {
      return static_object<value_type>::get();
    }
  };

  template <class PMap, class Key>
  struct Mutable_LvaluePropertyMapConcept
  {
    typedef typename property_traits<PMap>::category Category;
    typedef boost::lvalue_property_map_tag LvalueTag;
    typedef typename property_traits<PMap>::reference reference;
    void constraints() {
      BOOST_CONCEPT_ASSERT((ReadWritePropertyMapConcept<PMap, Key>));
      BOOST_CONCEPT_ASSERT((ConvertibleConcept<Category, LvalueTag>));

      typedef typename property_traits<PMap>::value_type value_type;
      BOOST_MPL_ASSERT((boost::is_same<value_type&, reference>));

      reference ref = pmap[k];
      ignore_unused_variable_warning(ref);
    }
    PMap pmap;
    Key k;
  };
  template <typename KeyArchetype, typename ValueArchetype>
  struct mutable_lvalue_property_map_archetype
    : public readable_property_map_archetype<KeyArchetype, ValueArchetype>,
      public writable_property_map_archetype<KeyArchetype, ValueArchetype>
  {
    typedef KeyArchetype key_type;
    typedef ValueArchetype value_type;
    typedef ValueArchetype& reference;
    typedef lvalue_property_map_tag category;
    value_type& operator[](const key_type&) const { 
      return static_object<value_type>::get();
    }
  };

  template <typename T>
  struct typed_identity_property_map;

  // A helper class for constructing a property map
  // from a class that implements operator[]

  template <class Reference, class LvaluePropertyMap>
  struct put_get_helper { };

  template <class PropertyMap, class Reference, class K>
  inline Reference
  get(const put_get_helper<Reference, PropertyMap>& pa, const K& k)
  {
    Reference v = static_cast<const PropertyMap&>(pa)[k];
    return v;
  }
  template <class PropertyMap, class Reference, class K, class V>
  inline void
  put(const put_get_helper<Reference, PropertyMap>& pa, K k, const V& v)
  {
    static_cast<const PropertyMap&>(pa)[k] = v;
  }

  //=========================================================================
  // Adapter to turn a RandomAccessIterator into a property map

  template <class RandomAccessIterator, 
    class IndexMap
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    , class T, class R
#else
    , class T = typename std::iterator_traits<RandomAccessIterator>::value_type
    , class R = typename std::iterator_traits<RandomAccessIterator>::reference
#endif
     >
  class iterator_property_map
    : public boost::put_get_helper< R, 
        iterator_property_map<RandomAccessIterator, IndexMap,
        T, R> >
  {
  public:
    typedef typename property_traits<IndexMap>::key_type key_type;
    typedef T value_type;
    typedef R reference;
    typedef boost::lvalue_property_map_tag category;

    inline iterator_property_map(
      RandomAccessIterator cc = RandomAccessIterator(), 
      const IndexMap& _id = IndexMap() ) 
      : iter(cc), index(_id) { }
    inline R operator[](key_type v) const { return *(iter + get(index, v)) ; }
  protected:
    RandomAccessIterator iter;
    IndexMap index;
  };

#if !defined BOOST_NO_STD_ITERATOR_TRAITS
  template <class RAIter, class ID>
  inline iterator_property_map<
    RAIter, ID,
    typename std::iterator_traits<RAIter>::value_type,
    typename std::iterator_traits<RAIter>::reference>
  make_iterator_property_map(RAIter iter, ID id) {
    BOOST_CONCEPT_ASSERT((RandomAccessIteratorConcept<RAIter>));
    typedef iterator_property_map<
      RAIter, ID,
      typename std::iterator_traits<RAIter>::value_type,
      typename std::iterator_traits<RAIter>::reference> PA;
    return PA(iter, id);
  }
#endif
  template <class RAIter, class Value, class ID>
  inline iterator_property_map<RAIter, ID, Value, Value&>
  make_iterator_property_map(RAIter iter, ID id, Value) {
    BOOST_CONCEPT_ASSERT((RandomAccessIteratorConcept<RAIter>));
    typedef iterator_property_map<RAIter, ID, Value, Value&> PMap;
    return PMap(iter, id);
  }

  template <class RandomAccessIterator, 
    class IndexMap
#ifdef BOOST_NO_STD_ITERATOR_TRAITS
    , class T, class R
#else
    , class T = typename std::iterator_traits<RandomAccessIterator>::value_type
    , class R = typename std::iterator_traits<RandomAccessIterator>::reference
#endif
     >
  class safe_iterator_property_map
    : public boost::put_get_helper< R, 
        safe_iterator_property_map<RandomAccessIterator, IndexMap,
        T, R> >
  {
  public:
    typedef typename property_traits<IndexMap>::key_type key_type; 
    typedef T value_type;
    typedef R reference;
    typedef boost::lvalue_property_map_tag category;

    inline safe_iterator_property_map(
      RandomAccessIterator first, 
      std::size_t n_ = 0, 
      const IndexMap& _id = IndexMap() ) 
      : iter(first), n(n_), index(_id) { }
    inline safe_iterator_property_map() { }
    inline R operator[](key_type v) const {
      BOOST_ASSERT(get(index, v) < n);
      return *(iter + get(index, v)) ;
    }
    typename property_traits<IndexMap>::value_type size() const { return n; }
  protected:
    RandomAccessIterator iter;
    typename property_traits<IndexMap>::value_type n;
    IndexMap index;
  };

  template <class RAIter, class ID>
  inline safe_iterator_property_map<
    RAIter, ID,
    typename std::iterator_traits<RAIter>::value_type,
    typename std::iterator_traits<RAIter>::reference>
  make_safe_iterator_property_map(RAIter iter, std::size_t n, ID id) {
    BOOST_CONCEPT_ASSERT((RandomAccessIteratorConcept<RAIter>));
    typedef safe_iterator_property_map<
      RAIter, ID,
      typename std::iterator_traits<RAIter>::value_type,
      typename std::iterator_traits<RAIter>::reference> PA;
    return PA(iter, n, id);
  }
  template <class RAIter, class Value, class ID>
  inline safe_iterator_property_map<RAIter, ID, Value, Value&>
  make_safe_iterator_property_map(RAIter iter, std::size_t n, ID id, Value) {
    BOOST_CONCEPT_ASSERT((RandomAccessIteratorConcept<RAIter>));
    typedef safe_iterator_property_map<RAIter, ID, Value, Value&> PMap;
    return PMap(iter, n, id);
  }

  //=========================================================================
  // An adaptor to turn a Unique Pair Associative Container like std::map or
  // std::hash_map into an Lvalue Property Map.

  template <typename UniquePairAssociativeContainer>
  class associative_property_map
    : public boost::put_get_helper<
       typename UniquePairAssociativeContainer::value_type::second_type&,
       associative_property_map<UniquePairAssociativeContainer> >
  {
    typedef UniquePairAssociativeContainer C;
  public:
    typedef typename C::key_type key_type;
    typedef typename C::value_type::second_type value_type;
    typedef value_type& reference;
    typedef lvalue_property_map_tag category;
    associative_property_map() : m_c(0) { }
    associative_property_map(C& c) : m_c(&c) { }
    reference operator[](const key_type& k) const {
      return (*m_c)[k];
    }
  private:
    C* m_c;
  };

  template <class UniquePairAssociativeContainer>
  associative_property_map<UniquePairAssociativeContainer>
  make_assoc_property_map(UniquePairAssociativeContainer& c)
  {
    return associative_property_map<UniquePairAssociativeContainer>(c);
  }

  template <typename UniquePairAssociativeContainer>
  class const_associative_property_map
    : public boost::put_get_helper<
       const typename UniquePairAssociativeContainer::value_type::second_type&,
       const_associative_property_map<UniquePairAssociativeContainer> >
  {
    typedef UniquePairAssociativeContainer C;
  public:
    typedef typename C::key_type key_type;
    typedef typename C::value_type::second_type value_type;
    typedef const value_type& reference;
    typedef lvalue_property_map_tag category;
    const_associative_property_map() : m_c(0) { }
    const_associative_property_map(const C& c) : m_c(&c) { }
    reference operator[](const key_type& k) const {
      return m_c->find(k)->second;
    }
  private:
    C const* m_c;
  };
  
  template <class UniquePairAssociativeContainer>
  const_associative_property_map<UniquePairAssociativeContainer>
  make_assoc_property_map(const UniquePairAssociativeContainer& c)
  {
    return const_associative_property_map<UniquePairAssociativeContainer>(c);
  }

  //=========================================================================
  // A property map that always returns the same object by value.
  //
  template <typename ValueType, typename KeyType = void>
  class static_property_map :
      public
  boost::put_get_helper<ValueType,static_property_map<ValueType> >
  { 
    ValueType value;
  public:
    typedef KeyType key_type;
    typedef ValueType value_type;
    typedef ValueType reference;
    typedef readable_property_map_tag category;
    static_property_map(ValueType v) : value(v) {}
    
    template<typename T>
    inline reference operator[](T) const { return value; }
  };

  template <typename KeyType, typename ValueType>
  static_property_map<ValueType, KeyType>
  make_static_property_map(const ValueType& v) {
    return static_property_map<ValueType, KeyType>(v);
  }

  //=========================================================================
  // A property map that always returns a reference to the same object.
  //
  template <typename KeyType, typename ValueType>
  class ref_property_map :
    public
      boost::put_get_helper<ValueType&,ref_property_map<KeyType,ValueType> >
  { 
    ValueType* value;
  public:
    typedef KeyType key_type;
    typedef ValueType value_type;
    typedef ValueType& reference;
    typedef lvalue_property_map_tag category;
    ref_property_map(ValueType& v) : value(&v) {}
    ValueType& operator[](key_type const&) const { return *value; }
  };

  //=========================================================================
  // A generalized identity property map
  template <typename T>
  struct typed_identity_property_map
    : public boost::put_get_helper<T, typed_identity_property_map<T> >
  {
    typedef T key_type;
    typedef T value_type;
    typedef T reference;
    typedef boost::readable_property_map_tag category;

    inline value_type operator[](const key_type& v) const { return v; }
  };

//=========================================================================
  // A property map that applies the identity function to integers
  typedef typed_identity_property_map<std::size_t> identity_property_map;

  //=========================================================================
  // A property map that does not do anything, for
  // when you have to supply a property map, but don't need it.
  namespace detail {
    struct dummy_pmap_reference {
      template <class T>
      dummy_pmap_reference& operator=(const T&) { return *this; }
      operator int() { return 0; }
    };
  }
  class dummy_property_map 
    : public boost::put_get_helper<detail::dummy_pmap_reference,
        dummy_property_map  > 
  {
  public:
    typedef void key_type; 
    typedef int value_type;
    typedef detail::dummy_pmap_reference reference;
    typedef boost::read_write_property_map_tag category;
    inline dummy_property_map() : c(0) { }
    inline dummy_property_map(value_type cc) : c(cc) { }
    inline dummy_property_map(const dummy_property_map& x)
      : c(x.c) { }
    template <class Vertex>
    inline reference operator[](Vertex) const { return reference(); }
   protected:
    value_type c;
  };

  // Convert a Readable property map into a function object
  template <typename PropMap>
  class property_map_function {
    PropMap pm;
    typedef typename property_traits<PropMap>::key_type param_type;
    public:
    explicit property_map_function(const PropMap& pm): pm(pm) {}
    typedef typename property_traits<PropMap>::value_type result_type;
    result_type operator()(const param_type& k) const {return get(pm, k);}
  };

  template <typename PropMap>
  property_map_function<PropMap>
  make_property_map_function(const PropMap& pm) {
    return property_map_function<PropMap>(pm);
  }

} // namespace boost

#ifdef BOOST_GRAPH_USE_MPI
// Hide include from dependency trackers; the right fix
// is not to have it at all, but who knows what'll break
#define BOOST_PMAP_HEADER_NAME <boost/property_map/parallel/parallel_property_maps.hpp>
#include BOOST_PMAP_HEADER_NAME
#undef BOOST_PMAP_HEADER_NAME
#endif

#include <boost/property_map/vector_property_map.hpp>

#endif /* BOOST_PROPERTY_MAP_HPP */

