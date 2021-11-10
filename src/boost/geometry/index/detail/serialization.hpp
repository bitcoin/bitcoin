// Boost.Geometry Index
//
// Copyright (c) 2011-2015 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_SERIALIZATION_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_SERIALIZATION_HPP

//#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
//#include <boost/serialization/nvp.hpp>

// TODO
// how about using the unsigned type capable of storing Max in compile-time versions?

// TODO
// - add wrappers for Point and Box and implement serialize for those wrappers instead of
//   raw geometries
//   PROBLEM: after implementing this, how Values would be set?
// - store the name of the parameters to know how to load and detect errors
// - in the header, once store info about the Indexable and Bounds types (geometry type, point CS, Dim, etc.)
//   each geometry save without this info

// TODO - move to index/detail/serialization.hpp
namespace boost { namespace geometry { namespace index { namespace detail {

// TODO - use boost::move?
template<typename T>
class serialization_storage
{
public:
    template <typename Archive>
    serialization_storage(Archive & ar, unsigned int version)
    {
        boost::serialization::load_construct_data_adl(ar, this->address(), version);
    }
    ~serialization_storage()
    {
        this->address()->~T();
    }
    T * address()
    {
        return static_cast<T*>(m_storage.address());
    }
private:
    boost::aligned_storage<sizeof(T), boost::alignment_of<T>::value> m_storage;
};

// TODO - save and load item_version? see: collections_load_imp and collections_save_imp
// this should be done once for the whole container
// versions of all used types should be stored

template <typename T, typename Archive> inline
T serialization_load(const char * name, Archive & ar)
{
    namespace bs = boost::serialization;    
    serialization_storage<T> storage(ar, bs::version<T>::value);        // load_construct_data
    ar >> boost::serialization::make_nvp(name, *storage.address());   // serialize
    //ar >> *storage.address();                                           // serialize
    return *storage.address();
}

template <typename T, typename Archive> inline
void serialization_save(T const& t, const char * name, Archive & ar)
{
    namespace bs = boost::serialization;
    bs::save_construct_data_adl(ar, boost::addressof(t), bs::version<T>::value);  // save_construct_data
    ar << boost::serialization::make_nvp(name, t);                                // serialize
    //ar << t;                                                                      // serialize
}
    
}}}}

// TODO - move to index/serialization/rtree.hpp
namespace boost { namespace serialization {

// boost::geometry::index::linear

template<class Archive, size_t Max, size_t Min>
void save_construct_data(Archive & ar, const boost::geometry::index::linear<Max, Min> * params, unsigned int )
{
    size_t max = params->get_max_elements(), min = params->get_min_elements();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
}
template<class Archive, size_t Max, size_t Min>
void load_construct_data(Archive & ar, boost::geometry::index::linear<Max, Min> * params, unsigned int )
{
    size_t max, min;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    if ( max != params->get_max_elements() || min != params->get_min_elements() )
        // TODO change exception type
        BOOST_THROW_EXCEPTION(std::runtime_error("parameters not compatible"));
    // the constructor musn't be called for this type
    //::new(params)boost::geometry::index::linear<Max, Min>();
}
template<class Archive, size_t Max, size_t Min> void serialize(Archive &, boost::geometry::index::linear<Max, Min> &, unsigned int) {}

// boost::geometry::index::quadratic

template<class Archive, size_t Max, size_t Min>
void save_construct_data(Archive & ar, const boost::geometry::index::quadratic<Max, Min> * params, unsigned int )
{
    size_t max = params->get_max_elements(), min = params->get_min_elements();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
}
template<class Archive, size_t Max, size_t Min>
void load_construct_data(Archive & ar, boost::geometry::index::quadratic<Max, Min> * params, unsigned int )
{
    size_t max, min;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    if ( max != params->get_max_elements() || min != params->get_min_elements() )
        // TODO change exception type
        BOOST_THROW_EXCEPTION(std::runtime_error("parameters not compatible"));
    // the constructor musn't be called for this type
    //::new(params)boost::geometry::index::quadratic<Max, Min>();
}
template<class Archive, size_t Max, size_t Min> void serialize(Archive &, boost::geometry::index::quadratic<Max, Min> &, unsigned int) {}

// boost::geometry::index::rstar

template<class Archive, size_t Max, size_t Min, size_t RE, size_t OCT>
void save_construct_data(Archive & ar, const boost::geometry::index::rstar<Max, Min, RE, OCT> * params, unsigned int )
{
    size_t max = params->get_max_elements()
         , min = params->get_min_elements()
         , re = params->get_reinserted_elements()
         , oct = params->get_overlap_cost_threshold();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
    ar << boost::serialization::make_nvp("re", re);
    ar << boost::serialization::make_nvp("oct", oct);
}
template<class Archive, size_t Max, size_t Min, size_t RE, size_t OCT>
void load_construct_data(Archive & ar, boost::geometry::index::rstar<Max, Min, RE, OCT> * params, unsigned int )
{
    size_t max, min, re, oct;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    ar >> boost::serialization::make_nvp("re", re);
    ar >> boost::serialization::make_nvp("oct", oct);
    if ( max != params->get_max_elements() || min != params->get_min_elements() ||
         re != params->get_reinserted_elements() || oct != params->get_overlap_cost_threshold() )
        // TODO change exception type
        BOOST_THROW_EXCEPTION(std::runtime_error("parameters not compatible"));
    // the constructor musn't be called for this type
    //::new(params)boost::geometry::index::rstar<Max, Min, RE, OCT>();
}
template<class Archive, size_t Max, size_t Min, size_t RE, size_t OCT>
void serialize(Archive &, boost::geometry::index::rstar<Max, Min, RE, OCT> &, unsigned int) {}

// boost::geometry::index::dynamic_linear

template<class Archive>
inline void save_construct_data(Archive & ar, const boost::geometry::index::dynamic_linear * params, unsigned int )
{
    size_t max = params->get_max_elements(), min = params->get_min_elements();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
}
template<class Archive>
inline void load_construct_data(Archive & ar, boost::geometry::index::dynamic_linear * params, unsigned int )
{
    size_t max, min;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    ::new(params)boost::geometry::index::dynamic_linear(max, min);
}
template<class Archive> void serialize(Archive &, boost::geometry::index::dynamic_linear &, unsigned int) {}

// boost::geometry::index::dynamic_quadratic

template<class Archive>
inline void save_construct_data(Archive & ar, const boost::geometry::index::dynamic_quadratic * params, unsigned int )
{
    size_t max = params->get_max_elements(), min = params->get_min_elements();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
}
template<class Archive>
inline void load_construct_data(Archive & ar, boost::geometry::index::dynamic_quadratic * params, unsigned int )
{
    size_t max, min;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    ::new(params)boost::geometry::index::dynamic_quadratic(max, min);
}
template<class Archive> void serialize(Archive &, boost::geometry::index::dynamic_quadratic &, unsigned int) {}

// boost::geometry::index::dynamic_rstar

template<class Archive>
inline void save_construct_data(Archive & ar, const boost::geometry::index::dynamic_rstar * params, unsigned int )
{
    size_t max = params->get_max_elements()
         , min = params->get_min_elements()
         , re = params->get_reinserted_elements()
         , oct = params->get_overlap_cost_threshold();
    ar << boost::serialization::make_nvp("max", max);
    ar << boost::serialization::make_nvp("min", min);
    ar << boost::serialization::make_nvp("re", re);
    ar << boost::serialization::make_nvp("oct", oct);
}
template<class Archive>
inline void load_construct_data(Archive & ar, boost::geometry::index::dynamic_rstar * params, unsigned int )
{
    size_t max, min, re, oct;
    ar >> boost::serialization::make_nvp("max", max);
    ar >> boost::serialization::make_nvp("min", min);
    ar >> boost::serialization::make_nvp("re", re);
    ar >> boost::serialization::make_nvp("oct", oct);
    ::new(params)boost::geometry::index::dynamic_rstar(max, min, re, oct);
}
template<class Archive> void serialize(Archive &, boost::geometry::index::dynamic_rstar &, unsigned int) {}

}} // boost::serialization

// TODO - move to index/detail/serialization.hpp or maybe geometry/serialization.hpp
namespace boost { namespace geometry { namespace index { namespace detail {

template <typename P, size_t I = 0, size_t D = geometry::dimension<P>::value>
struct serialize_point
{
    template <typename Archive>
    static inline void save(Archive & ar, P const& p, unsigned int version)
    {
        typename coordinate_type<P>::type c = get<I>(p);
        ar << boost::serialization::make_nvp("c", c);
        serialize_point<P, I+1, D>::save(ar, p, version);
    }

    template <typename Archive>
    static inline void load(Archive & ar, P & p, unsigned int version)
    {
        typename geometry::coordinate_type<P>::type c;
        ar >> boost::serialization::make_nvp("c", c);
        set<I>(p, c);
        serialize_point<P, I+1, D>::load(ar, p, version);
    }
};

template <typename P, size_t D>
struct serialize_point<P, D, D>
{
    template <typename Archive> static inline void save(Archive &, P const&, unsigned int) {}
    template <typename Archive> static inline void load(Archive &, P &, unsigned int) {}
};

}}}}

// TODO - move to index/detail/serialization.hpp or maybe geometry/serialization.hpp
namespace boost { namespace serialization {

template<class Archive, typename T, size_t D, typename C>
void save(Archive & ar, boost::geometry::model::point<T, D, C> const& p, unsigned int version)
{
    boost::geometry::index::detail::serialize_point< boost::geometry::model::point<T, D, C> >::save(ar, p, version);
}
template<class Archive, typename T, size_t D, typename C>
void load(Archive & ar, boost::geometry::model::point<T, D, C> & p, unsigned int version)
{
    boost::geometry::index::detail::serialize_point< boost::geometry::model::point<T, D, C> >::load(ar, p, version);
}
template<class Archive, typename T, size_t D, typename C>
inline void serialize(Archive & ar, boost::geometry::model::point<T, D, C> & o, const unsigned int version) { split_free(ar, o, version); }

template<class Archive, typename P>
inline void serialize(Archive & ar, boost::geometry::model::box<P> & b, const unsigned int)
{
    ar & boost::serialization::make_nvp("min", b.min_corner());
    ar & boost::serialization::make_nvp("max", b.max_corner());
}

}} // boost::serialization

// TODO - move to index/detail/rtree/visitors/save.hpp
namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree { namespace visitors {

// TODO move saving and loading of the rtree outside the rtree, this will require adding some kind of members_view

template <typename Archive, typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class save
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
public:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    save(Archive & archive, unsigned int version)
        : m_archive(archive), m_version(version)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        // CONSIDER: change to elements_type::size_type or size_type
        // or use fixed-size type like uint32 or even uint16?
        size_t s = elements.size();
        m_archive << boost::serialization::make_nvp("s", s);

        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            serialization_save(it->first, "b", m_archive);

            rtree::apply_visitor(*this, *it->second);
        }
    }

    inline void operator()(leaf const& l)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        //typedef typename elements_type::size_type elements_size;
        elements_type const& elements = rtree::elements(l);

        // CONSIDER: change to elements_type::size_type or size_type
        // or use fixed-size type like uint32 or even uint16?
        size_t s = elements.size();
        m_archive << boost::serialization::make_nvp("s", s);

        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            serialization_save(*it, "v", m_archive);
        }
    }

private:
    Archive & m_archive;
    unsigned int m_version;
};

}}}}}} // boost::geometry::index::detail::rtree::visitors

// TODO - move to index/detail/rtree/load.hpp
namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree {

template <typename MembersHolder>
class load
{
    typedef typename MembersHolder::parameters_type parameters_type;
    typedef typename MembersHolder::translator_type translator_type;
    typedef typename MembersHolder::allocators_type allocators_type;

    typedef typename MembersHolder::node node;
    typedef typename MembersHolder::internal_node internal_node;
    typedef typename MembersHolder::leaf leaf;

    typedef typename allocators_type::node_pointer node_pointer;
    typedef typename allocators_type::size_type size_type;

    typedef rtree::subtree_destroyer<MembersHolder> subtree_destroyer;

public:
    template <typename Archive> inline static
    node_pointer apply(Archive & ar, unsigned int version, size_type leafs_level,
                       size_type & values_count,
                       parameters_type const& parameters,
                       translator_type const& translator,
                       allocators_type & allocators)
    {
        values_count = 0;
        return raw_apply(ar, version, leafs_level, values_count, parameters, translator, allocators);
    }

private:
    template <typename Archive> inline static
    node_pointer raw_apply(Archive & ar, unsigned int version, size_type leafs_level,
                           size_type & values_count,
                           parameters_type const& parameters,
                           translator_type const& translator,
                           allocators_type & allocators,
                           size_type current_level = 0)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(current_level <= leafs_level, "invalid parameter");

        typedef typename rtree::elements_type<internal_node>::type elements_type;
        typedef typename elements_type::value_type element_type;
        //typedef typename elements_type::size_type elements_size;

        // CONSIDER: change to elements_type::size_type or size_type
        // or use fixed-size type like uint32 or even uint16?
        size_t elements_count;
        ar >> boost::serialization::make_nvp("s", elements_count);

        // leafs_level == 0 implies current_level == 0
        if ( (elements_count < parameters.get_min_elements() && leafs_level > 0)
                || parameters.get_max_elements() < elements_count )
            BOOST_THROW_EXCEPTION(std::runtime_error("rtree loading error"));

        if ( current_level < leafs_level )
        {
            node_pointer n = rtree::create_node<allocators_type, internal_node>::apply(allocators);         // MAY THROW (A)
            subtree_destroyer auto_remover(n, allocators);    
            internal_node & in = rtree::get<internal_node>(*n);

            elements_type & elements = rtree::elements(in);

            elements.reserve(elements_count);                                                               // MAY THROW (A)

            for ( size_t i = 0 ; i < elements_count ; ++i )
            {
                typedef typename elements_type::value_type::first_type box_type;
                box_type b = serialization_load<box_type>("b", ar);
                node_pointer n = raw_apply(ar, version, leafs_level, values_count, parameters, translator, allocators, current_level+1); // recursive call
                elements.push_back(element_type(b, n));
            }

            auto_remover.release();
            return n;
        }
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(current_level == leafs_level, "unexpected value");

            node_pointer n = rtree::create_node<allocators_type, leaf>::apply(allocators);                  // MAY THROW (A)
            subtree_destroyer auto_remover(n, allocators);
            leaf & l = rtree::get<leaf>(*n);

            typedef typename rtree::elements_type<leaf>::type elements_type;
            typedef typename elements_type::value_type element_type;
            elements_type & elements = rtree::elements(l);

            values_count += elements_count;

            elements.reserve(elements_count);                                                               // MAY THROW (A)

            for ( size_t i = 0 ; i < elements_count ; ++i )
            {
                element_type el = serialization_load<element_type>("v", ar);                                     // MAY THROW (C)
                elements.push_back(el);                                                                     // MAY THROW (C)
            }

            auto_remover.release();
            return n;
        }
    }
};

}}}}} // boost::geometry::index::detail::rtree

// TODO - move to index/detail/rtree/private_view.hpp
namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree {

template <typename Rtree>
class const_private_view
{
public:
    typedef typename Rtree::size_type size_type;

    typedef typename Rtree::translator_type translator_type;
    typedef typename Rtree::value_type value_type;
    typedef typename Rtree::options_type options_type;
    typedef typename Rtree::box_type box_type;
    typedef typename Rtree::allocators_type allocators_type;    

    const_private_view(Rtree const& rt) : m_rtree(rt) {}

    typedef typename Rtree::members_holder members_holder;

    members_holder const& members() const { return m_rtree.m_members; }

private:
    const_private_view(const_private_view const&);
    const_private_view & operator=(const_private_view const&);

    Rtree const& m_rtree;
};

template <typename Rtree>
class private_view
{
public:
    typedef typename Rtree::size_type size_type;

    typedef typename Rtree::translator_type translator_type;
    typedef typename Rtree::value_type value_type;
    typedef typename Rtree::options_type options_type;
    typedef typename Rtree::box_type box_type;
    typedef typename Rtree::allocators_type allocators_type;    

    private_view(Rtree & rt) : m_rtree(rt) {}

    typedef typename Rtree::members_holder members_holder;

    members_holder & members() { return m_rtree.m_members; }
    members_holder const& members() const { return m_rtree.m_members; }

private:
    private_view(private_view const&);
    private_view & operator=(private_view const&);

    Rtree & m_rtree;
};

}}}}} // namespace boost::geometry::index::detail::rtree

// TODO - move to index/serialization/rtree.hpp
namespace boost { namespace serialization {

template<class Archive, typename V, typename P, typename I, typename E, typename A>
void save(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, unsigned int version)
{
    namespace detail = boost::geometry::index::detail;

    typedef boost::geometry::index::rtree<V, P, I, E, A> rtree;
    typedef detail::rtree::const_private_view<rtree> view;
    typedef typename view::translator_type translator_type;
    typedef typename view::value_type value_type;
    typedef typename view::options_type options_type;
    typedef typename view::box_type box_type;
    typedef typename view::allocators_type allocators_type;

    view tree(rt);

    detail::serialization_save(tree.members().parameters(), "parameters", ar);

    ar << boost::serialization::make_nvp("values_count", tree.members().values_count);
    ar << boost::serialization::make_nvp("leafs_level", tree.members().leafs_level);

    if ( tree.members().values_count )
    {
        BOOST_GEOMETRY_INDEX_ASSERT(tree.members().root, "root shouldn't be null_ptr");

        detail::rtree::visitors::save<Archive, value_type, options_type, translator_type, box_type, allocators_type> save_v(ar, version);
        detail::rtree::apply_visitor(save_v, *tree.members().root);
    }
}

template<class Archive, typename V, typename P, typename I, typename E, typename A>
void load(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> & rt, unsigned int version)
{
    namespace detail = boost::geometry::index::detail;

    typedef boost::geometry::index::rtree<V, P, I, E, A> rtree;
    typedef detail::rtree::private_view<rtree> view;
    typedef typename view::size_type size_type;
    typedef typename view::translator_type translator_type;
    typedef typename view::value_type value_type;
    typedef typename view::options_type options_type;
    typedef typename view::box_type box_type;
    typedef typename view::allocators_type allocators_type;
    typedef typename view::members_holder members_holder;

    typedef typename options_type::parameters_type parameters_type;
    typedef typename allocators_type::node_pointer node_pointer;
    typedef detail::rtree::subtree_destroyer<members_holder> subtree_destroyer;

    view tree(rt);

    parameters_type params = detail::serialization_load<parameters_type>("parameters", ar);

    size_type values_count, leafs_level;
    ar >> boost::serialization::make_nvp("values_count", values_count);
    ar >> boost::serialization::make_nvp("leafs_level", leafs_level);

    node_pointer n(0);
    if ( 0 < values_count )
    {
        size_type loaded_values_count = 0;
        n = detail::rtree::load<members_holder>
            ::apply(ar, version, leafs_level, loaded_values_count, params, tree.members().translator(), tree.members().allocators());                                        // MAY THROW

        subtree_destroyer remover(n, tree.members().allocators());
        if ( loaded_values_count != values_count )
            BOOST_THROW_EXCEPTION(std::runtime_error("unexpected number of values")); // TODO change exception type
        remover.release();
    }

    tree.members().parameters() = params;
    tree.members().values_count = values_count;
    tree.members().leafs_level = leafs_level;

    subtree_destroyer remover(tree.members().root, tree.members().allocators());
    tree.members().root = n;
}

template<class Archive, typename V, typename P, typename I, typename E, typename A> inline
void serialize(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> & rt, unsigned int version)
{
    split_free(ar, rt, version);
}

template<class Archive, typename V, typename P, typename I, typename E, typename A> inline
void serialize(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, unsigned int version)
{
    split_free(ar, rt, version);
}

}} // boost::serialization

#endif // BOOST_GEOMETRY_INDEX_DETAIL_SERIALIZATION_HPP
