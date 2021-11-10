// Boost.Geometry Index
//
// R-tree implementation
//
// Copyright (c) 2008 Federico J. Fernandez.
// Copyright (c) 2011-2019 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2020 Caian Benedicto, Campinas, Brazil.
//
// This file was modified by Oracle on 2019-2021.
// Modifications copyright (c) 2019-2021 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_RTREE_HPP
#define BOOST_GEOMETRY_INDEX_RTREE_HPP

// STD
#include <algorithm>
#include <type_traits>

// Boost
#include <boost/container/new_allocator.hpp>
#include <boost/move/move.hpp>
#include <boost/tuple/tuple.hpp>

// Boost.Geometry
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/algorithms/detail/comparable_distance/interface.hpp>
#include <boost/geometry/algorithms/detail/covered_by/interface.hpp>
#include <boost/geometry/algorithms/detail/disjoint/interface.hpp>
#include <boost/geometry/algorithms/detail/equals/interface.hpp>
#include <boost/geometry/algorithms/detail/intersects/interface.hpp>
#include <boost/geometry/algorithms/detail/overlaps/interface.hpp>
#include <boost/geometry/algorithms/detail/touches/interface.hpp>
#include <boost/geometry/algorithms/detail/within/interface.hpp>
#include <boost/geometry/algorithms/centroid.hpp>

#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

// Boost.Geometry.Index
#include <boost/geometry/index/detail/config_begin.hpp>

#include <boost/geometry/index/detail/assert.hpp>
#include <boost/geometry/index/detail/exception.hpp>

#include <boost/geometry/index/detail/rtree/options.hpp>

#include <boost/geometry/index/indexable.hpp>
#include <boost/geometry/index/equal_to.hpp>

#include <boost/geometry/index/detail/translator.hpp>

#include <boost/geometry/index/predicates.hpp>
#include <boost/geometry/index/distance_predicates.hpp>
#include <boost/geometry/index/detail/rtree/adaptors.hpp>

#include <boost/geometry/index/detail/meta.hpp>
#include <boost/geometry/index/detail/utilities.hpp>
#include <boost/geometry/index/detail/rtree/node/node.hpp>

#include <boost/geometry/index/detail/algorithms/is_valid.hpp>

#include <boost/geometry/index/detail/rtree/visitors/insert.hpp>
#include <boost/geometry/index/detail/rtree/visitors/iterator.hpp>
#include <boost/geometry/index/detail/rtree/visitors/remove.hpp>
#include <boost/geometry/index/detail/rtree/visitors/copy.hpp>
#include <boost/geometry/index/detail/rtree/visitors/destroy.hpp>
#include <boost/geometry/index/detail/rtree/visitors/spatial_query.hpp>
#include <boost/geometry/index/detail/rtree/visitors/distance_query.hpp>
#include <boost/geometry/index/detail/rtree/visitors/count.hpp>
#include <boost/geometry/index/detail/rtree/visitors/children_box.hpp>

#include <boost/geometry/index/detail/rtree/linear/linear.hpp>
#include <boost/geometry/index/detail/rtree/quadratic/quadratic.hpp>
#include <boost/geometry/index/detail/rtree/rstar/rstar.hpp>
//#include <boost/geometry/extensions/index/detail/rtree/kmeans/kmeans.hpp>

#include <boost/geometry/index/detail/rtree/pack_create.hpp>

#include <boost/geometry/index/inserter.hpp>

#include <boost/geometry/index/detail/rtree/utilities/view.hpp>

#include <boost/geometry/index/detail/rtree/iterators.hpp>
#include <boost/geometry/index/detail/rtree/query_iterators.hpp>

#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
// serialization
#include <boost/geometry/index/detail/serialization.hpp>
#endif

#include <boost/geometry/util/range.hpp>
#include <boost/geometry/util/type_traits.hpp>

// TODO change the name to bounding_tree

/*!
\defgroup rtree_functions R-tree free functions (boost::geometry::index::)
*/

namespace boost { namespace geometry { namespace index {

/*!
\brief The R-tree spatial index.

This is self-balancing spatial index capable to store various types of Values
and balancing algorithms.

\par Parameters
The user must pass a type defining the Parameters which will
be used in rtree creation process. This type is used e.g. to specify balancing
algorithm with specific parameters like min and max number of elements in node.

\par
Predefined algorithms with compile-time parameters are:
 \li <tt>boost::geometry::index::linear</tt>,
 \li <tt>boost::geometry::index::quadratic</tt>,
 \li <tt>boost::geometry::index::rstar</tt>.

\par
Predefined algorithms with run-time parameters are:
 \li \c boost::geometry::index::dynamic_linear,
 \li \c boost::geometry::index::dynamic_quadratic,
 \li \c boost::geometry::index::dynamic_rstar.

\par IndexableGetter
The object of IndexableGetter type translates from Value to Indexable each time
r-tree requires it. This means that this operation is done for each Value
access. Therefore the IndexableGetter should return the Indexable by
a reference type. The Indexable should not be calculated since it could harm
the performance. The default IndexableGetter can translate all types adapted
to Point, Box or Segment concepts (called Indexables). Furthermore, it can
handle <tt>std::pair<Indexable, T></tt>, <tt>std::tuple<Indexable, ...></tt>
and <tt>boost::tuple<Indexable, ...></tt>. For example, for Value
of type <tt>std::pair<Box, int></tt>, the default IndexableGetter translates
from <tt>std::pair<Box, int> const&</tt> to <tt>Box const&</tt>.

\par EqualTo
The object of EqualTo type compares Values and returns <tt>true</tt> if they
are equal. It's similar to <tt>std::equal_to<></tt>. The default EqualTo
returns the result of <tt>boost::geometry::equals()</tt> for types adapted to
some Geometry concept defined in Boost.Geometry and the result of
<tt>operator==</tt> for other types. Components of Pairs and Tuples are
compared left-to-right.

\tparam Value           The type of objects stored in the container.
\tparam Parameters      Compile-time parameters.
\tparam IndexableGetter The function object extracting Indexable from Value.
\tparam EqualTo         The function object comparing objects of type Value.
\tparam Allocator       The allocator used to allocate/deallocate memory,
                        construct/destroy nodes and Values.
*/
template
<
    typename Value,
    typename Parameters,
    typename IndexableGetter = index::indexable<Value>,
    typename EqualTo = index::equal_to<Value>,
    typename Allocator = boost::container::new_allocator<Value>
>
class rtree
{
    BOOST_COPYABLE_AND_MOVABLE(rtree)

public:
    /*! \brief The type of Value stored in the container. */
    typedef Value value_type;
    /*! \brief R-tree parameters type. */
    typedef Parameters parameters_type;
    /*! \brief The function object extracting Indexable from Value. */
    typedef IndexableGetter indexable_getter;
    /*! \brief The function object comparing objects of type Value. */
    typedef EqualTo value_equal;
    /*! \brief The type of allocator used by the container. */
    typedef Allocator allocator_type;

    // TODO: SHOULD THIS TYPE BE REMOVED?
    /*! \brief The Indexable type to which Value is translated. */
    typedef typename index::detail::indexable_type<
        detail::translator<IndexableGetter, EqualTo>
    >::type indexable_type;

    /*! \brief The Box type used by the R-tree. */
    typedef geometry::model::box<
                geometry::model::point<
                    typename coordinate_type<indexable_type>::type,
                    dimension<indexable_type>::value,
                    typename coordinate_system<indexable_type>::type
                >
            >
    bounds_type;

private:

    typedef bounds_type box_type;

    struct members_holder
        : public detail::translator<IndexableGetter, EqualTo>
        , public Parameters
        , public detail::rtree::allocators
            <
                Allocator,
                Value,
                Parameters,
                bounds_type,
                typename detail::rtree::options_type<Parameters>::type::node_tag
            >
    {
        typedef Value value_type;
        typedef typename rtree::bounds_type bounds_type;
        typedef Parameters parameters_type;
        //typedef IndexableGetter indexable_getter;
        //typedef EqualTo value_equal;
        //typedef Allocator allocator_type;

        typedef bounds_type box_type;
        typedef detail::translator<IndexableGetter, EqualTo> translator_type;
        typedef typename detail::rtree::options_type<Parameters>::type options_type;
        typedef typename options_type::node_tag node_tag;
        typedef detail::rtree::allocators
            <
                Allocator, Value, Parameters, bounds_type, node_tag
            > allocators_type;

        typedef typename detail::rtree::node
            <
                value_type, parameters_type, bounds_type, allocators_type, node_tag
            >::type node;
        typedef typename detail::rtree::internal_node
            <
                value_type, parameters_type, bounds_type, allocators_type, node_tag
            >::type internal_node;
        typedef typename detail::rtree::leaf
            <
                value_type, parameters_type, bounds_type, allocators_type, node_tag
            >::type leaf;

        // TODO: only one visitor type is needed
        typedef typename detail::rtree::visitor
            <
                value_type, parameters_type, bounds_type, allocators_type, node_tag, false
            >::type visitor;
        typedef typename detail::rtree::visitor
            <
                value_type, parameters_type, bounds_type, allocators_type, node_tag, true
            >::type visitor_const;

        typedef typename allocators_type::node_pointer node_pointer;

        typedef ::boost::container::allocator_traits<Allocator> allocator_traits_type;
        typedef typename allocators_type::size_type size_type;

    private:
        members_holder(members_holder const&);
        members_holder & operator=(members_holder const&);

    public:
        template <typename IndGet, typename ValEq, typename Alloc>
        members_holder(IndGet const& ind_get,
                       ValEq const& val_eq,
                       Parameters const& parameters,
                       BOOST_FWD_REF(Alloc) alloc)
            : translator_type(ind_get, val_eq)
            , Parameters(parameters)
            , allocators_type(boost::forward<Alloc>(alloc))
            , values_count(0)
            , leafs_level(0)
            , root(0)
        {}

        template <typename IndGet, typename ValEq>
        members_holder(IndGet const& ind_get,
                       ValEq const& val_eq,
                       Parameters const& parameters)
            : translator_type(ind_get, val_eq)
            , Parameters(parameters)
            , allocators_type()
            , values_count(0)
            , leafs_level(0)
            , root(0)
        {}

        translator_type const& translator() const { return *this; }

        IndexableGetter const& indexable_getter() const { return *this; }
        IndexableGetter & indexable_getter() { return *this; }
        EqualTo const& equal_to() const { return *this; }
        EqualTo & equal_to() { return *this; }
        Parameters const& parameters() const { return *this; }
        Parameters & parameters() { return *this; }
        allocators_type const& allocators() const { return *this; }
        allocators_type & allocators() { return *this; }

        size_type values_count;
        size_type leafs_level;
        node_pointer root;
    };

    typedef typename members_holder::translator_type translator_type;    
    typedef typename members_holder::options_type options_type;
    typedef typename members_holder::allocators_type allocators_type;
    typedef typename members_holder::node node;
    typedef typename members_holder::internal_node internal_node;
    typedef typename members_holder::leaf leaf;

    typedef typename members_holder::node_pointer node_pointer;
    typedef typename members_holder::allocator_traits_type allocator_traits_type;

    friend class detail::rtree::utilities::view<rtree>;
#ifdef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
    friend class detail::rtree::private_view<rtree>;
    friend class detail::rtree::const_private_view<rtree>;
#endif

public:

    /*! \brief Type of reference to Value. */
    typedef typename allocators_type::reference reference;
    /*! \brief Type of reference to const Value. */
    typedef typename allocators_type::const_reference const_reference;
    /*! \brief Type of pointer to Value. */
    typedef typename allocators_type::pointer pointer;
    /*! \brief Type of pointer to const Value. */
    typedef typename allocators_type::const_pointer const_pointer;
    /*! \brief Type of difference type. */
    typedef typename allocators_type::difference_type difference_type;
    /*! \brief Unsigned integral type used by the container. */
    typedef typename allocators_type::size_type size_type;

    /*! \brief Type of const iterator, category ForwardIterator. */
    typedef index::detail::rtree::iterators::iterator
        <
            value_type, options_type, translator_type, box_type, allocators_type
        > const_iterator;

    /*! \brief Type of const query iterator, category ForwardIterator. */
    typedef index::detail::rtree::iterators::query_iterator
        <
            value_type, allocators_type
        > const_query_iterator;

public:

    /*!
    \brief The constructor.

    \param parameters   The parameters object.
    \param getter       The function object extracting Indexable from Value.
    \param equal        The function object comparing Values.

    \par Throws
    If allocator default constructor throws.
    */
    inline explicit rtree(parameters_type const& parameters = parameters_type(),
                          indexable_getter const& getter = indexable_getter(),
                          value_equal const& equal = value_equal())
        : m_members(getter, equal, parameters)
    {}

    /*!
    \brief The constructor.

    \param parameters   The parameters object.
    \param getter       The function object extracting Indexable from Value.
    \param equal        The function object comparing Values.
    \param allocator    The allocator object.

    \par Throws
    If allocator copy constructor throws.
    */
    inline rtree(parameters_type const& parameters,
                 indexable_getter const& getter,
                 value_equal const& equal,
                 allocator_type const& allocator)
        : m_members(getter, equal, parameters, allocator)
    {}

    /*!
    \brief The constructor.

    The tree is created using packing algorithm.

    \param first        The beginning of the range of Values.
    \param last         The end of the range of Values.
    \param parameters   The parameters object.
    \param getter       The function object extracting Indexable from Value.
    \param equal        The function object comparing Values.
    \param allocator    The allocator object.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Iterator>
    inline rtree(Iterator first, Iterator last,
                 parameters_type const& parameters = parameters_type(),
                 indexable_getter const& getter = indexable_getter(),
                 value_equal const& equal = value_equal(),
                 allocator_type const& allocator = allocator_type())
        : m_members(getter, equal, parameters, allocator)
    {
        pack_construct(first, last, boost::container::new_allocator<void>());
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm.

    \param rng          The range of Values.
    \param parameters   The parameters object.
    \param getter       The function object extracting Indexable from Value.
    \param equal        The function object comparing Values.
    \param allocator    The allocator object.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Range>
    inline explicit rtree(Range const& rng,
                          parameters_type const& parameters = parameters_type(),
                          indexable_getter const& getter = indexable_getter(),
                          value_equal const& equal = value_equal(),
                          allocator_type const& allocator = allocator_type())
        : m_members(getter, equal, parameters, allocator)
    {
        pack_construct(::boost::begin(rng), ::boost::end(rng), boost::container::new_allocator<void>());
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param first             The beginning of the range of Values.
    \param last              The end of the range of Values.
    \param parameters        The parameters object.
    \param getter            The function object extracting Indexable from Value.
    \param equal             The function object comparing Values.
    \param allocator         The allocator object for persistent data in the tree.
    \param temp_allocator    The temporary allocator object used when packing.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Iterator, typename PackAlloc>
    inline rtree(Iterator first, Iterator last,
                 parameters_type const& parameters,
                 indexable_getter const& getter,
                 value_equal const& equal,
                 allocator_type const& allocator,
                 PackAlloc const& temp_allocator)
        : m_members(getter, equal, parameters, allocator)
    {
        pack_construct(first, last, temp_allocator);
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param rng               The range of Values.
    \param parameters        The parameters object.
    \param getter            The function object extracting Indexable from Value.
    \param equal             The function object comparing Values.
    \param allocator         The allocator object for persistent data in the tree.
    \param temp_allocator    The temporary allocator object used when packing.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Range, typename PackAlloc>
    inline explicit rtree(Range const& rng,
                          parameters_type const& parameters,
                          indexable_getter const& getter,
                          value_equal const& equal,
                          allocator_type const& allocator,
                          PackAlloc const& temp_allocator)
        : m_members(getter, equal, parameters, allocator)
    {
        pack_construct(::boost::begin(rng), ::boost::end(rng), temp_allocator);
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param first        The beginning of the range of Values.
    \param last         The end of the range of Values.
    \param allocator    The allocator object for persistent data in the tree.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Iterator>
    inline rtree(Iterator first, Iterator last,
                 allocator_type const& allocator)
        : m_members(indexable_getter(), value_equal(), parameters_type(), allocator)
    {
        pack_construct(first, last, boost::container::new_allocator<void>());
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param rng          The range of Values.
    \param allocator    The allocator object for persistent data in the tree.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Range>
    inline explicit rtree(Range const& rng,
                          allocator_type const& allocator)
        : m_members(indexable_getter(), value_equal(), parameters_type(), allocator)
    {
        pack_construct(::boost::begin(rng), ::boost::end(rng), boost::container::new_allocator<void>());
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param first             The beginning of the range of Values.
    \param last              The end of the range of Values.
    \param allocator         The allocator object for persistent data in the tree.
    \param temp_allocator    The temporary allocator object used when packing.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Iterator, typename PackAlloc>
    inline rtree(Iterator first, Iterator last,
                 allocator_type const& allocator,
                 PackAlloc const& temp_allocator)
        : m_members(indexable_getter(), value_equal(), parameters_type(), allocator)
    {
        pack_construct(first, last, temp_allocator);
    }

    /*!
    \brief The constructor.

    The tree is created using packing algorithm and a temporary packing allocator.

    \param rng               The range of Values.
    \param allocator         The allocator object for persistent data in the tree.
    \param temp_allocator    The temporary allocator object used when packing.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Range, typename PackAlloc>
    inline explicit rtree(Range const& rng,
                          allocator_type const& allocator,
                          PackAlloc const& temp_allocator)
        : m_members(indexable_getter(), value_equal(), parameters_type(), allocator)
    {
        pack_construct(::boost::begin(rng), ::boost::end(rng), temp_allocator);
    }

    /*!
    \brief The destructor.

    \par Throws
    Nothing.
    */
    inline ~rtree()
    {
        this->raw_destroy(*this);
    }

    /*!
    \brief  The copy constructor.

    It uses parameters, translator and allocator from the source tree.

    \param src          The rtree which content will be copied.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor throws.
    \li If allocation throws or returns invalid value.
    */
    inline rtree(rtree const& src)
        : m_members(src.m_members.indexable_getter(),
                    src.m_members.equal_to(),
                    src.m_members.parameters(),
                    allocator_traits_type::select_on_container_copy_construction(src.get_allocator()))
    {
        this->raw_copy(src, *this, false);
    }

    /*!
    \brief The copy constructor.

    It uses Parameters and translator from the source tree.

    \param src          The rtree which content will be copied.
    \param allocator    The allocator which will be used.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor throws.
    \li If allocation throws or returns invalid value.
    */
    inline rtree(rtree const& src, allocator_type const& allocator)
        : m_members(src.m_members.indexable_getter(),
                    src.m_members.equal_to(),
                    src.m_members.parameters(), allocator)
    {
        this->raw_copy(src, *this, false);
    }

    /*!
    \brief The moving constructor.

    It uses parameters, translator and allocator from the source tree.

    \param src          The rtree which content will be moved.

    \par Throws
    Nothing.
    */
    inline rtree(BOOST_RV_REF(rtree) src)
        : m_members(src.m_members.indexable_getter(),
                    src.m_members.equal_to(),
                    src.m_members.parameters(),
                    boost::move(src.m_members.allocators()))
    {
        boost::swap(m_members.values_count, src.m_members.values_count);
        boost::swap(m_members.leafs_level, src.m_members.leafs_level);
        boost::swap(m_members.root, src.m_members.root);
    }

    /*!
    \brief The moving constructor.

    It uses parameters and translator from the source tree.

    \param src          The rtree which content will be moved.
    \param allocator    The allocator.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor throws (only if allocators aren't equal).
    \li If allocation throws or returns invalid value (only if allocators aren't equal).
    */
    inline rtree(BOOST_RV_REF(rtree) src, allocator_type const& allocator)
        : m_members(src.m_members.indexable_getter(),
                    src.m_members.equal_to(),
                    src.m_members.parameters(),
                    allocator)
    {
        if ( src.m_members.allocators() == allocator )
        {
            boost::swap(m_members.values_count, src.m_members.values_count);
            boost::swap(m_members.leafs_level, src.m_members.leafs_level);
            boost::swap(m_members.root, src.m_members.root);
        }
        else
        {
            this->raw_copy(src, *this, false);
        }
    }

    /*!
    \brief The assignment operator.

    It uses parameters and translator from the source tree.

    \param src          The rtree which content will be copied.

    \par Throws
    \li If Value copy constructor throws.
    \li If allocation throws.
    \li If allocation throws or returns invalid value.
    */
    inline rtree & operator=(BOOST_COPY_ASSIGN_REF(rtree) src)
    {
        if ( &src != this )
        {
            allocators_type & this_allocs = m_members.allocators();
            allocators_type const& src_allocs = src.m_members.allocators();

            // NOTE: if propagate is true for std allocators on darwin 4.2.1, glibc++
            // (allocators stored as base classes of members_holder)
            // copying them changes values_count, in this case it doesn't cause errors since data must be copied
            
            typedef std::integral_constant<bool,
                allocator_traits_type::propagate_on_container_copy_assignment::value
            > propagate;
            
            if ( propagate::value && !(this_allocs == src_allocs) )
                this->raw_destroy(*this);
            detail::assign_cond(this_allocs, src_allocs, propagate());

            // It uses m_allocators
            this->raw_copy(src, *this, true);
        }

        return *this;
    }

    /*!
    \brief The moving assignment.

    It uses parameters and translator from the source tree.

    \param src          The rtree which content will be moved.

    \par Throws
    Only if allocators aren't equal.
    \li If Value copy constructor throws.
    \li If allocation throws or returns invalid value.
    */
    inline rtree & operator=(BOOST_RV_REF(rtree) src)
    {
        if ( &src != this )
        {
            allocators_type & this_allocs = m_members.allocators();
            allocators_type & src_allocs = src.m_members.allocators();
            
            if ( this_allocs == src_allocs )
            {
                this->raw_destroy(*this);

                m_members.indexable_getter() = src.m_members.indexable_getter();
                m_members.equal_to() = src.m_members.equal_to();
                m_members.parameters() = src.m_members.parameters();

                boost::swap(m_members.values_count, src.m_members.values_count);
                boost::swap(m_members.leafs_level, src.m_members.leafs_level);
                boost::swap(m_members.root, src.m_members.root);

                // NOTE: if propagate is true for std allocators on darwin 4.2.1, glibc++
                // (allocators stored as base classes of members_holder)
                // moving them changes values_count
                
                typedef std::integral_constant<bool,
                    allocator_traits_type::propagate_on_container_move_assignment::value
                > propagate;
                detail::move_cond(this_allocs, src_allocs, propagate());
            }
            else
            {
// TODO - shouldn't here propagate_on_container_copy_assignment be checked like in operator=(const&)?

                // It uses m_allocators
                this->raw_copy(src, *this, true);
            }
        }

        return *this;
    }

    /*!
    \brief Swaps contents of two rtrees.

    Parameters, translator and allocators are swapped as well.

    \param other    The rtree which content will be swapped with this rtree content.

    \par Throws
    If allocators swap throws.
    */
    void swap(rtree & other)
    {
        boost::swap(m_members.indexable_getter(), other.m_members.indexable_getter());
        boost::swap(m_members.equal_to(), other.m_members.equal_to());
        boost::swap(m_members.parameters(), other.m_members.parameters());
        
        // NOTE: if propagate is true for std allocators on darwin 4.2.1, glibc++
        // (allocators stored as base classes of members_holder)
        // swapping them changes values_count
        
        typedef std::integral_constant<bool,
            allocator_traits_type::propagate_on_container_swap::value
        > propagate;
        detail::swap_cond(m_members.allocators(), other.m_members.allocators(), propagate());

        boost::swap(m_members.values_count, other.m_members.values_count);
        boost::swap(m_members.leafs_level, other.m_members.leafs_level);
        boost::swap(m_members.root, other.m_members.root);
    }

    /*!
    \brief Insert a value to the index.

    \param value    The value which will be stored in the container.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    inline void insert(value_type const& value)
    {
        if ( !m_members.root )
            this->raw_create();

        this->raw_insert(value);
    }

    /*!
    \brief Insert a range of values to the index.

    \param first    The beginning of the range of values.
    \param last     The end of the range of values.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    template <typename Iterator>
    inline void insert(Iterator first, Iterator last)
    {
        if ( !m_members.root )
            this->raw_create();

        for ( ; first != last ; ++first )
            this->raw_insert(*first);
    }

    /*!
    \brief Insert a value created using convertible object or a range of values to the index.

    \param conv_or_rng      An object of type convertible to value_type or a range of values.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    template <typename ConvertibleOrRange>
    inline void insert(ConvertibleOrRange const& conv_or_rng)
    {
        if ( !m_members.root )
            this->raw_create();

        typedef std::is_convertible<ConvertibleOrRange, value_type> is_conv_t;
        typedef range::detail::is_range<ConvertibleOrRange> is_range_t;
        BOOST_GEOMETRY_STATIC_ASSERT((is_conv_t::value || is_range_t::value),
            "The argument has to be convertible to Value type or be a Range.",
            ConvertibleOrRange);

        this->insert_dispatch(conv_or_rng, is_conv_t());
    }

    /*!
    \brief Remove a value from the container.

    In contrast to the \c std::set or <tt>std::map erase()</tt> method
    this method removes only one value from the container.

    \param value    The value which will be removed from the container.

    \return         1 if the value was removed, 0 otherwise.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    inline size_type remove(value_type const& value)
    {
        if ( !m_members.root )
            return 0;

        return this->raw_remove(value);
    }

    /*!
    \brief Remove a range of values from the container.

    In contrast to the \c std::set or <tt>std::map erase()</tt> method
    it doesn't take iterators pointing to values stored in this container. It removes values equal
    to these passed as a range. Furthermore this method removes only one value for each one passed
    in the range, not all equal values.

    \param first    The beginning of the range of values.
    \param last     The end of the range of values.

    \return         The number of removed values.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    template <typename Iterator>
    inline size_type remove(Iterator first, Iterator last)
    {
        size_type result = 0;

        if ( !m_members.root )
            return result;

        for ( ; first != last ; ++first )
            result += this->raw_remove(*first);
        return result;
    }

    /*!
    \brief Remove value corresponding to an object convertible to it or a range of values from the container.

    In contrast to the \c std::set or <tt>std::map erase()</tt> method
    it removes values equal to these passed as a range. Furthermore, this method removes only
    one value for each one passed in the range, not all equal values.

    \param conv_or_rng      The object of type convertible to value_type or a range of values.

    \return         The number of removed values.

    \par Throws
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.

    \warning
    This operation only guarantees that there will be no memory leaks.
    After an exception is thrown the R-tree may be left in an inconsistent state,
    elements must not be inserted or removed. Other operations are allowed however
    some of them may return invalid data.
    */
    template <typename ConvertibleOrRange>
    inline size_type remove(ConvertibleOrRange const& conv_or_rng)
    {
        if ( !m_members.root )
            return 0;

        typedef std::is_convertible<ConvertibleOrRange, value_type> is_conv_t;
        typedef range::detail::is_range<ConvertibleOrRange> is_range_t;
        BOOST_GEOMETRY_STATIC_ASSERT((is_conv_t::value || is_range_t::value),
            "The argument has to be convertible to Value type or be a Range.",
            ConvertibleOrRange);

        return this->remove_dispatch(conv_or_rng, is_conv_t());
    }

    /*!
    \brief Finds values meeting passed predicates e.g. nearest to some Point and/or intersecting some Box.

    This query function performs spatial and k-nearest neighbor searches. It allows to pass a set of predicates.
    Values will be returned only if all predicates are met.

    <b>Spatial predicates</b>
    
    Spatial predicates may be generated by one of the functions listed below:
    \li \c boost::geometry::index::contains(),
    \li \c boost::geometry::index::covered_by(),
    \li \c boost::geometry::index::covers(),
    \li \c boost::geometry::index::disjoint(),
    \li \c boost::geometry::index::intersects(),
    \li \c boost::geometry::index::overlaps(),
    \li \c boost::geometry::index::within(),

    It is possible to negate spatial predicates:
    \li <tt>! boost::geometry::index::contains()</tt>,
    \li <tt>! boost::geometry::index::covered_by()</tt>,
    \li <tt>! boost::geometry::index::covers()</tt>,
    \li <tt>! boost::geometry::index::disjoint()</tt>,
    \li <tt>! boost::geometry::index::intersects()</tt>,
    \li <tt>! boost::geometry::index::overlaps()</tt>,
    \li <tt>! boost::geometry::index::within()</tt>

    <b>Satisfies predicate</b>
    
    This is a special kind of predicate which allows to pass a user-defined function or function object which checks
    if Value should be returned by the query. It's generated by:
    \li \c boost::geometry::index::satisfies().

    <b>Nearest predicate</b>

    If the nearest predicate is passed a k-nearest neighbor search will be performed. This query will result
    in returning k values to the output iterator. Only one nearest predicate may be passed to the query.
    It may be generated by:
    \li \c boost::geometry::index::nearest().
        
    <b>Connecting predicates</b>

    Predicates may be passed together connected with \c operator&&().

    \par Example
    \verbatim
    // return elements intersecting box
    tree.query(bgi::intersects(box), std::back_inserter(result));
    // return elements intersecting poly but not within box
    tree.query(bgi::intersects(poly) && !bgi::within(box), std::back_inserter(result));
    // return elements overlapping box and meeting my_fun unary predicate
    tree.query(bgi::overlaps(box) && bgi::satisfies(my_fun), std::back_inserter(result));
    // return 5 elements nearest to pt and elements are intersecting box
    tree.query(bgi::nearest(pt, 5) && bgi::intersects(box), std::back_inserter(result));

    // For each found value do_something (it is a type of function object)
    tree.query(bgi::intersects(box),
               boost::make_function_output_iterator(do_something()));

    // For each value stored in the rtree do_something
    // always_true is a type of function object always returning true
    tree.query(bgi::satisfies(always_true()),
               boost::make_function_output_iterator(do_something()));

    // C++11 (lambda expression)
    tree.query(bgi::intersects(box),
               boost::make_function_output_iterator([](value_type const& val){
                   // do something
               }));

    // C++14 (generic lambda expression)
    tree.query(bgi::intersects(box),
               boost::make_function_output_iterator([](auto const& val){
                   // do something
               }));
    \endverbatim

    \par Throws
    If Value copy constructor or copy assignment throws.
    If predicates copy throws.

    \warning
    Only one \c nearest() predicate may be passed to the query. Passing more of them results in compile-time error.

    \param predicates   Predicates.
    \param out_it       The output iterator, e.g. generated by std::back_inserter().

    \return             The number of values found.
    */
    template <typename Predicates, typename OutIter>
    size_type query(Predicates const& predicates, OutIter out_it) const
    {
        if ( !m_members.root )
            return 0;

        static const std::size_t distance_predicates_count = detail::predicates_count_distance<Predicates>::value;
        static const bool is_distance_predicate = 0 < distance_predicates_count;
        BOOST_GEOMETRY_STATIC_ASSERT((distance_predicates_count <= 1),
            "Only one distance predicate can be passed.",
            Predicates);

        return query_dispatch(predicates, out_it,
                              std::integral_constant<bool, is_distance_predicate>());
    }

    /*!
    \brief Returns a query iterator pointing at the begin of the query range.

    This method returns an iterator which may be used to perform iterative queries.
    For the information about predicates which may be passed to this method see query().

    \par Example
    \verbatim    
    for ( Rtree::const_query_iterator it = tree.qbegin(bgi::nearest(pt, 10000)) ;
          it != tree.qend() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }

    // C++11 (auto)
    for ( auto it = tree.qbegin(bgi::nearest(pt, 3)) ; it != tree.qend() ; ++it )
    {
        // do something with value
    }

    // C++14 (generic lambda expression)
    std::for_each(tree.qbegin(bgi::nearest(pt, 3)), tree.qend(), [](auto const& val){
        // do something with value
    });
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    If predicates copy throws.
    If allocation throws.

    \warning
    The modification of the rtree may invalidate the iterators.

    \param predicates   Predicates.
    
    \return             The iterator pointing at the begin of the query range.
    */
    template <typename Predicates>
    const_query_iterator qbegin(Predicates const& predicates) const
    {
        return const_query_iterator(qbegin_(predicates));
    }

    /*!
    \brief Returns a query iterator pointing at the end of the query range.

    This method returns an iterator which may be used to check if the query has ended.

    \par Example
    \verbatim    
    for ( Rtree::const_query_iterator it = tree.qbegin(bgi::nearest(pt, 10000)) ;
          it != tree.qend() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }

    // C++11 (auto)
    for ( auto it = tree.qbegin(bgi::nearest(pt, 3)) ; it != tree.qend() ; ++it )
    {
        // do something with value
    }

    // C++14 (generic lambda expression)
    std::for_each(tree.qbegin(bgi::nearest(pt, 3)), tree.qend(), [](auto const& val){
        // do something with value
    });
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    Nothing

    \warning
    The modification of the rtree may invalidate the iterators.
    
    \return             The iterator pointing at the end of the query range.
    */
    const_query_iterator qend() const
    {
        return const_query_iterator();
    }

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_EXPERIMENTAL
private:
#endif
    /*!
    \brief Returns a query iterator pointing at the begin of the query range.

    This method returns an iterator which may be used to perform iterative queries.
    For the information about predicates which may be passed to this method see query().
    
    The type of the returned iterator depends on the type of passed Predicates but the iterator of this type
    may be assigned to the variable of const_query_iterator type. If you'd like to use the type of the iterator
    returned by this method you may get the type e.g. by using C++11 decltype or Boost.Typeof library.
    This iterator may be compared with iterators returned by both versions of qend() method.

    \par Example
    \verbatim
    // Store the result in the container using std::copy() - it requires both iterators of the same type
    std::copy(tree.qbegin_(bgi::intersects(box)), tree.qend_(bgi::intersects(box)), std::back_inserter(result));

    // Store the result in the container using std::copy() and type-erased iterators
    Rtree::const_query_iterator first = tree.qbegin_(bgi::intersects(box));
    Rtree::const_query_iterator last = tree.qend_();
    std::copy(first, last, std::back_inserter(result));

    // Boost.Typeof
    typedef BOOST_TYPEOF(tree.qbegin(bgi::nearest(pt, 10000))) Iter;
    for ( Iter it = tree.qbegin_(bgi::nearest(pt, 10000)) ; it != tree.qend_() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }

    // C++11 (auto)
    for ( auto it = tree.qbegin_(bgi::nearest(pt, 10000)) ; it != tree.qend_() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    If predicates copy throws.
    If allocation throws.

    \warning
    The modification of the rtree may invalidate the iterators.

    \param predicates   Predicates.
    
    \return             The iterator pointing at the begin of the query range.
    */
    template <typename Predicates>
    std::conditional_t
        <
            detail::predicates_count_distance<Predicates>::value == 0,
            detail::rtree::iterators::spatial_query_iterator<members_holder, Predicates>,
            detail::rtree::iterators::distance_query_iterator
                <
                    members_holder, Predicates,
                    detail::predicates_find_distance<Predicates>::value
                >
        >
    qbegin_(Predicates const& predicates) const
    {
        static const std::size_t distance_predicates_count = detail::predicates_count_distance<Predicates>::value;
        BOOST_GEOMETRY_STATIC_ASSERT((distance_predicates_count <= 1),
            "Only one distance predicate can be passed.",
            Predicates);

        typedef std::conditional_t
            <
                detail::predicates_count_distance<Predicates>::value == 0,
                detail::rtree::iterators::spatial_query_iterator<members_holder, Predicates>,
                detail::rtree::iterators::distance_query_iterator
                    <
                        members_holder, Predicates,
                        detail::predicates_find_distance<Predicates>::value
                    >
            > iterator_type;

        if ( !m_members.root )
            return iterator_type(m_members.parameters(), m_members.translator(), predicates);

        return iterator_type(m_members.root, m_members.parameters(), m_members.translator(), predicates);
    }

    /*!
    \brief Returns the query iterator pointing at the end of the query range.

    This method returns the iterator which may be used to perform iterative queries. For the information
    about the predicates which may be passed to this method see query().
    
    The type of the returned iterator depends on the type of passed Predicates but the iterator of this type
    may be assigned to the variable of const_query_iterator type. If you'd like to use the type of the iterator
    returned by this method you may get the type e.g. by using C++11 decltype or Boost.Typeof library.

    The type of the iterator returned by this method is the same as the one returned by qbegin() to which
    the same predicates were passed.

    \par Example
    \verbatim
    // Store the result in the container using std::copy() - it requires both iterators of the same type
    std::copy(tree.qbegin_(bgi::intersects(box)), tree.qend_(bgi::intersects(box)), std::back_inserter(result));
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    If predicates copy throws.

    \warning
    The modification of the rtree may invalidate the iterators.

    \param predicates   Predicates.
    
    \return             The iterator pointing at the end of the query range.
    */
    template <typename Predicates>
    std::conditional_t
        <
            detail::predicates_count_distance<Predicates>::value == 0,
            detail::rtree::iterators::spatial_query_iterator<members_holder, Predicates>,
            detail::rtree::iterators::distance_query_iterator
                <
                    members_holder, Predicates,
                    detail::predicates_find_distance<Predicates>::value
                >
        >
    qend_(Predicates const& predicates) const
    {
        static const std::size_t distance_predicates_count = detail::predicates_count_distance<Predicates>::value;
        BOOST_GEOMETRY_STATIC_ASSERT((distance_predicates_count <= 1),
            "Only one distance predicate can be passed.",
            Predicates);

        typedef std::conditional_t
            <
                detail::predicates_count_distance<Predicates>::value == 0,
                detail::rtree::iterators::spatial_query_iterator<members_holder, Predicates>,
                detail::rtree::iterators::distance_query_iterator
                    <
                        members_holder, Predicates,
                        detail::predicates_find_distance<Predicates>::value
                    >
            > iterator_type;

        return iterator_type(m_members.parameters(), m_members.translator(), predicates);
    }

    /*!
    \brief Returns the query iterator pointing at the end of the query range.

    This method returns the iterator which may be compared with the iterator returned by qbegin() in order to
    check if the query has ended.
    
    The type of the returned iterator is different than the type returned by qbegin() but the iterator of this type
    may be assigned to the variable of const_query_iterator type. If you'd like to use the type of the iterator returned by this
    method, which most certainly will be faster than the type-erased iterator, you may get the type
    e.g. by using C++11 decltype or Boost.Typeof library.

    The type of the iterator returned by this method is different than the type returned by qbegin().

    \par Example
    \verbatim
    // Store the result in the container using std::copy() and type-erased iterators
    Rtree::const_query_iterator first = tree.qbegin_(bgi::intersects(box));
    Rtree::const_query_iterator last = tree.qend_();
    std::copy(first, last, std::back_inserter(result));

    // Boost.Typeof
    typedef BOOST_TYPEOF(tree.qbegin(bgi::nearest(pt, 10000))) Iter;
    for ( Iter it = tree.qbegin_(bgi::nearest(pt, 10000)) ; it != tree.qend_() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }

    // C++11 (auto)
    for ( auto it = tree.qbegin_(bgi::nearest(pt, 10000)) ; it != tree.qend_() ; ++it )
    {
        // do something with value
        if ( has_enough_nearest_values() )
            break;
    }
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    Nothing

    \warning
    The modification of the rtree may invalidate the iterators.
    
    \return             The iterator pointing at the end of the query range.
    */
    detail::rtree::iterators::end_query_iterator<value_type, allocators_type>
    qend_() const
    {
        return detail::rtree::iterators::end_query_iterator<value_type, allocators_type>();
    }

public:

    /*!
    \brief Returns the iterator pointing at the begin of the rtree values range.

    This method returns the iterator which may be used to iterate over all values
    stored in the rtree.

    \par Example
    \verbatim
    // Copy all values into the vector
    std::copy(tree.begin(), tree.end(), std::back_inserter(vec));

    for ( Rtree::const_iterator it = tree.begin() ; it != tree.end() ; ++it )
    {
        // do something with value
    }

    // C++11 (auto)
    for ( auto it = tree.begin() ; it != tree.end() ; ++it )
    {
        // do something with value
    }

    // C++14 (generic lambda expression)
    std::for_each(tree.begin(), tree.end(), [](auto const& val){
        // do something with value
    })
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    If allocation throws.

    \warning
    The modification of the rtree may invalidate the iterators.

    \return             The iterator pointing at the begin of the range.
    */
    const_iterator begin() const
    {
        if ( !m_members.root )
            return const_iterator();

        return const_iterator(m_members.root);
    }

    /*!
    \brief Returns the iterator pointing at the end of the rtree values range.

    This method returns the iterator which may be compared with the iterator returned by begin()
    in order to check if the iteration has ended.

    \par Example
    \verbatim
    for ( Rtree::const_iterator it = tree.begin() ; it != tree.end() ; ++it )
    {
        // do something with value
    }

    // C++11 (lambda expression)
    std::for_each(tree.begin(), tree.end(), [](value_type const& val){
        // do something with value
    })
    \endverbatim

    \par Iterator category
    ForwardIterator

    \par Throws
    Nothing.

    \warning
    The modification of the rtree may invalidate the iterators.

    \return             The iterator pointing at the end of the range.
    */
    const_iterator end() const
    {
        return const_iterator();
    }

    /*!
    \brief Returns the number of stored values.

    \return         The number of stored values.

    \par Throws
    Nothing.
    */
    inline size_type size() const
    {
        return m_members.values_count;
    }

    /*!
    \brief Query if the container is empty.

    \return         true if the container is empty.

    \par Throws
    Nothing.
    */
    inline bool empty() const
    {
        return 0 == m_members.values_count;
    }

    /*!
    \brief Removes all values stored in the container.

    \par Throws
    Nothing.
    */
    inline void clear()
    {
        this->raw_destroy(*this);
    }

    /*!
    \brief Returns the box able to contain all values stored in the container.

    Returns the box able to contain all values stored in the container.
    If the container is empty the result of \c geometry::assign_inverse() is returned.

    \return     The box able to contain all values stored in the container or an invalid box if
                there are no values in the container.

    \par Throws
    Nothing.
    */
    inline bounds_type bounds() const
    {
        bounds_type result;
        // in order to suppress the uninitialized variable warnings
        geometry::assign_inverse(result);

        if ( m_members.root )
        {
            detail::rtree::visitors::children_box
                <
                    members_holder
                > box_v(result, m_members.parameters(), m_members.translator());
            detail::rtree::apply_visitor(box_v, *m_members.root);
        }

        return result;
    }

    /*!
    \brief Count Values or Indexables stored in the container.
    
    For indexable_type it returns the number of values which indexables equals the parameter.
    For value_type it returns the number of values which equals the parameter.

    \param vori The value or indexable which will be counted.

    \return     The number of values found.

    \par Throws
    Nothing.
    */
    template <typename ValueOrIndexable>
    size_type count(ValueOrIndexable const& vori) const
    {
        if ( !m_members.root )
            return 0;

        // the input should be convertible to Value or Indexable type
        typedef typename index::detail::convertible_type
            <
                ValueOrIndexable,
                value_type,
                indexable_type
            >::type value_or_indexable;

        static const bool is_void = std::is_void<value_or_indexable>::value;
        BOOST_GEOMETRY_STATIC_ASSERT((! is_void),
            "The argument has to be convertible to Value or Indexable type.",
            ValueOrIndexable);

        // NOTE: If an object of convertible but not the same type is passed
        // into the function, here a temporary will be created.
        return this->template raw_count<value_or_indexable>(vori);
    }

    /*!
    \brief Returns parameters.

    \return     The parameters object.

    \par Throws
    Nothing.
    */
    inline parameters_type parameters() const
    {
        return m_members.parameters();
    }

    /*!
    \brief Returns function retrieving Indexable from Value.

    \return     The indexable_getter object.

    \par Throws
    Nothing.
    */
    indexable_getter indexable_get() const
    {
        return m_members.indexable_getter();
    }

    /*!
    \brief Returns function comparing Values

    \return     The value_equal function.

    \par Throws
    Nothing.
    */
    value_equal value_eq() const
    {
        return m_members.equal_to();
    }

    /*!
    \brief Returns allocator used by the rtree.

    \return     The allocator.

    \par Throws
    If allocator copy constructor throws.
    */
    allocator_type get_allocator() const
    {
        return m_members.allocators().allocator();
    }

private:

    /*!
    \brief Returns the translator object.

    \return     The translator object.

    \par Throws
    Nothing.
    */
    inline translator_type translator() const
    {
        return m_members.translator();
    }

    /*!
    \brief Apply a visitor to the nodes structure in order to perform some operator.

    This function is not a part of the 'official' interface. However it makes
    possible e.g. to pass a visitor drawing the tree structure.

    \param visitor  The visitor object.

    \par Throws
    If Visitor::operator() throws.
    */
    template <typename Visitor>
    inline void apply_visitor(Visitor & visitor) const
    {
        if ( m_members.root )
            detail::rtree::apply_visitor(visitor, *m_members.root);
    }

    /*!
    \brief Returns the depth of the R-tree.

    This function is not a part of the 'official' interface.

    \return     The depth of the R-tree.

    \par Throws
    Nothing.
    */
    inline size_type depth() const
    {
        return m_members.leafs_level;
    }

private:

    /*!
    \pre Root node must exist - m_root != 0.

    \brief Insert a value to the index.

    \param value    The value which will be stored in the container.

    \par Exception-safety
    basic
    */
    inline void raw_insert(value_type const& value)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_members.root, "The root must exist");
        // CONSIDER: alternative - ignore invalid indexable or throw an exception
        BOOST_GEOMETRY_INDEX_ASSERT(detail::is_valid(m_members.translator()(value)), "Indexable is invalid");

        detail::rtree::visitors::insert<value_type, members_holder>
            insert_v(m_members.root, m_members.leafs_level, value,
                     m_members.parameters(), m_members.translator(), m_members.allocators());

        detail::rtree::apply_visitor(insert_v, *m_members.root);

// TODO
// Think about this: If exception is thrown, may the root be removed?
// Or it is just cleared?

// TODO
// If exception is thrown, m_values_count may be invalid
        ++m_members.values_count;
    }

    /*!
    \brief Remove the value from the container.

    \param value    The value which will be removed from the container.

    \par Exception-safety
    basic
    */
    inline size_type raw_remove(value_type const& value)
    {
        // TODO: awulkiew - assert for correct value (indexable) ?
        BOOST_GEOMETRY_INDEX_ASSERT(m_members.root, "The root must exist");

        detail::rtree::visitors::remove<members_holder>
            remove_v(m_members.root, m_members.leafs_level, value,
                     m_members.parameters(), m_members.translator(), m_members.allocators());

        detail::rtree::apply_visitor(remove_v, *m_members.root);

        // If exception is thrown, m_values_count may be invalid

        if ( remove_v.is_value_removed() )
        {
            BOOST_GEOMETRY_INDEX_ASSERT(0 < m_members.values_count, "unexpected state");

            --m_members.values_count;

            return 1;
        }

        return 0;
    }

    /*!
    \brief Create an empty R-tree i.e. new empty root node and clear other attributes.

    \par Exception-safety
    strong
    */
    inline void raw_create()
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 == m_members.root, "the tree is already created");

        m_members.root = detail::rtree::create_node<allocators_type, leaf>::apply(m_members.allocators()); // MAY THROW (N: alloc)
        m_members.values_count = 0;
        m_members.leafs_level = 0;
    }

    /*!
    \brief Destroy the R-tree i.e. all nodes and clear attributes.

    \param t    The container which is going to be destroyed.

    \par Exception-safety
    nothrow
    */
    inline void raw_destroy(rtree & t)
    {
        if ( t.m_members.root )
        {
            detail::rtree::visitors::destroy<members_holder>
                ::apply(t.m_members.root, t.m_members.allocators());

            t.m_members.root = 0;
        }
        t.m_members.values_count = 0;
        t.m_members.leafs_level = 0;
    }

    /*!
    \brief Copy the R-tree i.e. whole nodes structure, values and other attributes.
    It uses destination's allocators to create the new structure.

    \param src                  The source R-tree.
    \param dst                  The destination R-tree.
    \param copy_tr_and_params   If true, translator and parameters will also be copied.

    \par Exception-safety
    strong
    */
    inline void raw_copy(rtree const& src, rtree & dst, bool copy_tr_and_params) const
    {
        detail::rtree::visitors::copy<members_holder> copy_v(dst.m_members.allocators());

        if ( src.m_members.root )
            detail::rtree::apply_visitor(copy_v, *src.m_members.root);                      // MAY THROW (V, E: alloc, copy, N: alloc)

        if ( copy_tr_and_params )
        {
            dst.m_members.indexable_getter() = src.m_members.indexable_getter();
            dst.m_members.equal_to() = src.m_members.equal_to();
            dst.m_members.parameters() = src.m_members.parameters();
        }

        // TODO use subtree_destroyer
        if ( dst.m_members.root )
        {
            detail::rtree::visitors::destroy<members_holder>
                ::apply(dst.m_members.root, dst.m_members.allocators());

            dst.m_members.root = 0;
        }

        dst.m_members.root = copy_v.result;
        dst.m_members.values_count = src.m_members.values_count;
        dst.m_members.leafs_level = src.m_members.leafs_level;
    }

    /*!
    \brief Insert a value corresponding to convertible object into the index.

    \param val_conv    The object convertible to value.

    \par Exception-safety
    basic
    */
    template <typename ValueConvertible>
    inline void insert_dispatch(ValueConvertible const& val_conv,
                                std::true_type /*is_convertible*/)
    {
        this->raw_insert(val_conv);
    }

    /*!
    \brief Insert a range of values into the index.

    \param rng    The range of values.

    \par Exception-safety
    basic
    */
    template <typename Range>
    inline void insert_dispatch(Range const& rng,
                                std::false_type /*is_convertible*/)
    {
        typedef typename boost::range_const_iterator<Range>::type It;
        for ( It it = boost::const_begin(rng); it != boost::const_end(rng) ; ++it )
            this->raw_insert(*it);
    }

    /*!
    \brief Remove a value corresponding to convertible object from the index.

    \param val_conv    The object convertible to value.

    \par Exception-safety
    basic
    */
    template <typename ValueConvertible>
    inline size_type remove_dispatch(ValueConvertible const& val_conv,
                                     std::true_type /*is_convertible*/)
    {
        return this->raw_remove(val_conv);
    }

    /*!
    \brief Remove a range of values from the index.

    \param rng    The range of values which will be removed from the container.

    \par Exception-safety
    basic
    */
    template <typename Range>
    inline size_type remove_dispatch(Range const& rng,
                                     std::false_type /*is_convertible*/)
    {
        size_type result = 0;
        typedef typename boost::range_const_iterator<Range>::type It;
        for ( It it = boost::const_begin(rng); it != boost::const_end(rng) ; ++it )
            result += this->raw_remove(*it);
        return result;
    }

    /*!
    \brief Return values meeting predicates.

    \par Exception-safety
    strong
    */
    template <typename Predicates, typename OutIter>
    size_type query_dispatch(Predicates const& predicates, OutIter out_it, std::false_type /*is_distance_predicate*/) const
    {
        detail::rtree::visitors::spatial_query<members_holder, Predicates, OutIter>
            find_v(m_members.parameters(), m_members.translator(), predicates, out_it);

        detail::rtree::apply_visitor(find_v, *m_members.root);

        return find_v.found_count;
    }

    /*!
    \brief Perform nearest neighbour search.

    \par Exception-safety
    strong
    */
    template <typename Predicates, typename OutIter>
    size_type query_dispatch(Predicates const& predicates, OutIter out_it, std::true_type /*is_distance_predicate*/) const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_members.root, "The root must exist");

        static const std::size_t distance_predicate_index = detail::predicates_find_distance<Predicates>::value;
        detail::rtree::visitors::distance_query<
            members_holder,
            Predicates,
            distance_predicate_index,
            OutIter
        > distance_v(m_members.parameters(), m_members.translator(), predicates, out_it);

        detail::rtree::apply_visitor(distance_v, *m_members.root);

        return distance_v.finish();
    }
    
    /*!
    \brief Count elements corresponding to value or indexable.

    \par Exception-safety
    strong
    */
    template <typename ValueOrIndexable>
    size_type raw_count(ValueOrIndexable const& vori) const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_members.root, "The root must exist");

        detail::rtree::visitors::count
            <
                ValueOrIndexable,
                members_holder
            > count_v(vori, m_members.parameters(), m_members.translator());

        detail::rtree::apply_visitor(count_v, *m_members.root);

        return count_v.found_count;
    }

    /*!
    \brief The constructor TODO.

    The tree is created using packing algorithm.

    \param first             The beginning of the range of Values.
    \param last              The end of the range of Values.
    \param temp_allocator    The temporary allocator object to be used by the packing algorithm.

    \par Throws
    \li If allocator copy constructor throws.
    \li If Value copy constructor or copy assignment throws.
    \li If allocation throws or returns invalid value.
    */
    template<typename Iterator, typename PackAlloc>
    inline void pack_construct(Iterator first, Iterator last, PackAlloc const& temp_allocator)
    {
        typedef detail::rtree::pack<members_holder> pack;
        size_type vc = 0, ll = 0;
        m_members.root = pack::apply(first, last, vc, ll,
                                     m_members.parameters(), m_members.translator(),
                                     m_members.allocators(), temp_allocator);
        m_members.values_count = vc;
        m_members.leafs_level = ll;
    }

    members_holder m_members;
};

/*!
\brief Insert a value to the index.

It calls <tt>rtree::insert(value_type const&)</tt>.

\ingroup rtree_functions

\param tree The spatial index.
\param v    The value which will be stored in the index.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline void insert(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
                   Value const& v)
{
    tree.insert(v);
}

/*!
\brief Insert a range of values to the index.

It calls <tt>rtree::insert(Iterator, Iterator)</tt>.

\ingroup rtree_functions

\param tree     The spatial index.
\param first    The beginning of the range of values.
\param last     The end of the range of values.
*/
template<typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
         typename Iterator>
inline void insert(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
                   Iterator first, Iterator last)
{
    tree.insert(first, last);
}

/*!
\brief Insert a value created using convertible object or a range of values to the index.

It calls <tt>rtree::insert(ConvertibleOrRange const&)</tt>.

\ingroup rtree_functions

\param tree             The spatial index.
\param conv_or_rng      The object of type convertible to value_type or a range of values.
*/
template<typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
         typename ConvertibleOrRange>
inline void insert(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
                   ConvertibleOrRange const& conv_or_rng)
{
    tree.insert(conv_or_rng);
}

/*!
\brief Remove a value from the container.

Remove a value from the container. In contrast to the \c std::set or <tt>std::map erase()</tt> method
this function removes only one value from the container.

It calls <tt>rtree::remove(value_type const&)</tt>.

\ingroup rtree_functions

\param tree The spatial index.
\param v    The value which will be removed from the index.

\return     1 if value was removed, 0 otherwise.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::size_type
remove(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
       Value const& v)
{
    return tree.remove(v);
}

/*!
\brief Remove a range of values from the container.

Remove a range of values from the container. In contrast to the \c std::set or <tt>std::map erase()</tt> method
it doesn't take iterators pointing to values stored in this container. It removes values equal
to these passed as a range. Furthermore this function removes only one value for each one passed
in the range, not all equal values.

It calls <tt>rtree::remove(Iterator, Iterator)</tt>.

\ingroup rtree_functions

\param tree     The spatial index.
\param first    The beginning of the range of values.
\param last     The end of the range of values.

\return         The number of removed values.
*/
template<typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
         typename Iterator>
inline typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::size_type
remove(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
       Iterator first, Iterator last)
{
    return tree.remove(first, last);
}

/*!
\brief Remove a value corresponding to an object convertible to it or a range of values from the container.

Remove a value corresponding to an object convertible to it or a range of values from the container.
In contrast to the \c std::set or <tt>std::map erase()</tt> method
it removes values equal to these passed as a range. Furthermore this method removes only
one value for each one passed in the range, not all equal values.

It calls <tt>rtree::remove(ConvertibleOrRange const&)</tt>.

\ingroup rtree_functions

\param tree                 The spatial index.
\param conv_or_rng      The object of type convertible to value_type or the range of values.

\return         The number of removed values.
*/
template<typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
         typename ConvertibleOrRange>
inline typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::size_type
remove(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree,
       ConvertibleOrRange const& conv_or_rng)
{
    return tree.remove(conv_or_rng);
}

/*!
\brief Finds values meeting passed predicates e.g. nearest to some Point and/or intersecting some Box.

This query function performs spatial and k-nearest neighbor searches. It allows to pass a set of predicates.
Values will be returned only if all predicates are met.

<b>Spatial predicates</b>
    
Spatial predicates may be generated by one of the functions listed below:
\li \c boost::geometry::index::contains(),
\li \c boost::geometry::index::covered_by(),
\li \c boost::geometry::index::covers(),
\li \c boost::geometry::index::disjoint(),
\li \c boost::geometry::index::intersects(),
\li \c boost::geometry::index::overlaps(),
\li \c boost::geometry::index::within(),

It is possible to negate spatial predicates:
\li <tt>! boost::geometry::index::contains()</tt>,
\li <tt>! boost::geometry::index::covered_by()</tt>,
\li <tt>! boost::geometry::index::covers()</tt>,
\li <tt>! boost::geometry::index::disjoint()</tt>,
\li <tt>! boost::geometry::index::intersects()</tt>,
\li <tt>! boost::geometry::index::overlaps()</tt>,
\li <tt>! boost::geometry::index::within()</tt>

<b>Satisfies predicate</b>

This is a special kind of predicate which allows to pass a user-defined function or function object which checks
if Value should be returned by the query. It's generated by:
\li \c boost::geometry::index::satisfies().

<b>Nearest predicate</b>

If the nearest predicate is passed a k-nearest neighbor search will be performed. This query will result
in returning k values to the output iterator. Only one nearest predicate may be passed to the query.
It may be generated by:
\li \c boost::geometry::index::nearest().
        
<b>Connecting predicates</b>

Predicates may be passed together connected with \c operator&&().

\par Example
\verbatim
// return elements intersecting box
bgi::query(tree, bgi::intersects(box), std::back_inserter(result));
// return elements intersecting poly but not within box
bgi::query(tree, bgi::intersects(poly) && !bgi::within(box), std::back_inserter(result));
// return elements overlapping box and meeting my_fun value predicate
bgi::query(tree, bgi::overlaps(box) && bgi::satisfies(my_fun), std::back_inserter(result));
// return 5 elements nearest to pt and elements are intersecting box
bgi::query(tree, bgi::nearest(pt, 5) && bgi::intersects(box), std::back_inserter(result));

// For each found value do_something (it is a type of function object)
tree.query(bgi::intersects(box),
           boost::make_function_output_iterator(do_something()));
\endverbatim

\par Throws
If Value copy constructor or copy assignment throws.

\warning
Only one \c nearest() predicate may be passed to the query. Passing more of them results in compile-time error.

\ingroup rtree_functions

\param tree         The rtree.
\param predicates   Predicates.
\param out_it       The output iterator, e.g. generated by std::back_inserter().

\return             The number of values found.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
          typename Predicates, typename OutIter> inline
typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::size_type
query(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree,
      Predicates const& predicates,
      OutIter out_it)
{
    return tree.query(predicates, out_it);
}

/*!
\brief Returns the query iterator pointing at the begin of the query range.

This method returns the iterator which may be used to perform iterative queries. For the information
about the predicates which may be passed to this method see query().
    
\par Example
\verbatim
std::for_each(bgi::qbegin(tree, bgi::nearest(pt, 3)), bgi::qend(tree), do_something());
\endverbatim

\par Iterator category
ForwardIterator

\par Throws
If predicates copy throws.
If allocation throws.

\warning
The modification of the rtree may invalidate the iterators.

\ingroup rtree_functions

\param tree         The rtree.
\param predicates   Predicates.
    
\return             The iterator pointing at the begin of the query range.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator,
          typename Predicates> inline
typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::const_query_iterator
qbegin(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree,
       Predicates const& predicates)
{
    return tree.qbegin(predicates);
}

/*!
\brief Returns the query iterator pointing at the end of the query range.

This method returns the iterator which may be used to check if the query has ended.
    
\par Example
\verbatim
std::for_each(bgi::qbegin(tree, bgi::nearest(pt, 3)), bgi::qend(tree), do_something());
\endverbatim

\par Iterator category
ForwardIterator

\par Throws
Nothing

\warning
The modification of the rtree may invalidate the iterators.

\ingroup rtree_functions
    
\return             The iterator pointing at the end of the query range.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator> inline
typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::const_query_iterator
qend(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.qend();
}

/*!
\brief Returns the iterator pointing at the begin of the rtree values range.

This method returns the iterator which may be used to iterate over all values
stored in the rtree.

\par Example
\verbatim
std::for_each(bgi::begin(tree), bgi::end(tree), do_something());
// the same as
std::for_each(boost::begin(tree), boost::end(tree), do_something());
\endverbatim

\par Iterator category
ForwardIterator

\par Throws
If allocation throws.

\warning
The modification of the rtree may invalidate the iterators.

\ingroup rtree_functions

\return             The iterator pointing at the begin of the range.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator> inline
typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::const_iterator
begin(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.begin();
}

/*!
\brief Returns the iterator pointing at the end of the rtree values range.

This method returns the iterator which may be compared with the iterator returned by begin()
in order to check if the iteration has ended.

\par Example
\verbatim
std::for_each(bgi::begin(tree), bgi::end(tree), do_something());
// the same as
std::for_each(boost::begin(tree), boost::end(tree), do_something());
\endverbatim

\par Iterator category
ForwardIterator

\par Throws
Nothing.

\warning
The modification of the rtree may invalidate the iterators.

\ingroup rtree_functions

\return             The iterator pointing at the end of the range.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator> inline
typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::const_iterator
end(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.end();
}

/*!
\brief Remove all values from the index.

It calls \c rtree::clear().

\ingroup rtree_functions

\param tree     The spatial index.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline void clear(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & tree)
{
    return tree.clear();
}

/*!
\brief Get the number of values stored in the index.

It calls \c rtree::size().

\ingroup rtree_functions

\param tree     The spatial index.

\return         The number of values stored in the index.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline size_t size(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.size();
}

/*!
\brief Query if there are no values stored in the index.

It calls \c rtree::empty().

\ingroup rtree_functions

\param tree     The spatial index.

\return         true if there are no values in the index.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline bool empty(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.bounds();
}

/*!
\brief Get the box containing all stored values or an invalid box if the index has no values.

It calls \c rtree::envelope().

\ingroup rtree_functions

\param tree     The spatial index.

\return         The box containing all stored values or an invalid box.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline typename rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>::bounds_type
bounds(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> const& tree)
{
    return tree.bounds();
}

/*!
\brief Exchanges the contents of the container with those of other.

It calls \c rtree::swap().

\ingroup rtree_functions

\param l     The first rtree.
\param r     The second rtree.
*/
template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
inline void swap(rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & l,
                 rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator> & r)
{
    return l.swap(r);
}

}}} // namespace boost::geometry::index

// Boost.Range adaptation
namespace boost {

template <typename Value, typename Parameters, typename IndexableGetter, typename EqualTo, typename Allocator>
struct range_mutable_iterator
    <
        boost::geometry::index::rtree<Value, Parameters, IndexableGetter, EqualTo, Allocator>
    >
{
    typedef typename boost::geometry::index::rtree
        <
            Value, Parameters, IndexableGetter, EqualTo, Allocator
        >::const_iterator type;
};

} // namespace boost

#include <boost/geometry/index/detail/config_end.hpp>

#endif // BOOST_GEOMETRY_INDEX_RTREE_HPP
