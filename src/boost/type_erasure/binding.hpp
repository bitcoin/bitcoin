// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_BINDING_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_BINDING_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/find_if.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/end.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_erasure/static_binding.hpp>
#include <boost/type_erasure/is_subconcept.hpp>
#include <boost/type_erasure/detail/adapt_to_vtable.hpp>
#include <boost/type_erasure/detail/null.hpp>
#include <boost/type_erasure/detail/rebind_placeholders.hpp>
#include <boost/type_erasure/detail/vtable.hpp>
#include <boost/type_erasure/detail/normalize.hpp>
#include <boost/type_erasure/detail/instantiate.hpp>
#include <boost/type_erasure/detail/check_map.hpp>

namespace boost {
namespace type_erasure {

template<class P>
class dynamic_binding;

namespace detail {

template<class Source, class Dest, class Map>
struct can_optimize_conversion : ::boost::mpl::and_<
    ::boost::is_same<Source, Dest>,
        ::boost::is_same<
            typename ::boost::mpl::find_if<
                Map,
                ::boost::mpl::not_<
                    ::boost::is_same<
                        ::boost::mpl::first< ::boost::mpl::_1>,
                        ::boost::mpl::second< ::boost::mpl::_1>
                    >
                >
            >::type,
            typename ::boost::mpl::end<Map>::type
        >
    >::type
{};

}

/**
 * Stores the binding of a @c Concept to a set of actual types.
 * @c Concept is interpreted in the same way as with @ref any.
 */
template<class Concept>
class binding
{
    typedef typename ::boost::type_erasure::detail::normalize_concept<
        Concept>::type normalized;
    typedef typename ::boost::mpl::transform<normalized,
        ::boost::type_erasure::detail::maybe_adapt_to_vtable< ::boost::mpl::_1>
    >::type actual_concept;
    typedef typename ::boost::type_erasure::detail::make_vtable<
        actual_concept>::type table_type;
    typedef typename ::boost::type_erasure::detail::get_placeholder_normalization_map<
        Concept
    >::type placeholder_subs;
public:

    /**
     * \pre @ref relaxed must be in @c Concept.
     *
     * \throws Nothing.
     */
    binding() { BOOST_MPL_ASSERT((::boost::type_erasure::is_relaxed<Concept>)); }
    
    /**
     * \pre @c Map must be an MPL map with an entry for each placeholder
     *      referred to by @c Concept.
     *
     * \throws Nothing.
     */
    template<class Map>
    explicit binding(const Map&)
      : impl((
            BOOST_TYPE_ERASURE_INSTANTIATE(Concept, Map),
            static_binding<Map>()
        ))
    {}
    
    /**
     * \pre @c Map must be an MPL map with an entry for each placeholder
     *      referred to by @c Concept.
     *
     * \throws Nothing.
     */
    template<class Map>
    binding(const static_binding<Map>&)
      : impl((
            BOOST_TYPE_ERASURE_INSTANTIATE(Concept, Map),
            static_binding<Map>()
        ))
    {}

    /**
     * Converts from another set of bindings.
     *
     * \pre Map must be an MPL map with an entry for each placeholder
     *      referred to by @c Concept.  The mapped type should be the
     *      corresponding placeholder in Concept2.
     *
     * \throws std::bad_alloc
     */
    template<class Concept2, class Map>
    binding(const binding<Concept2>& other, const Map&
#ifndef BOOST_TYPE_ERASURE_DOXYGEN
        , typename ::boost::enable_if<
            ::boost::mpl::and_<
                ::boost::type_erasure::detail::check_map<Concept, Map>,
                ::boost::type_erasure::is_subconcept<Concept, Concept2, Map>
            >
        >::type* = 0
#endif
        )
      : impl(
            other,
            static_binding<Map>(),
            ::boost::type_erasure::detail::can_optimize_conversion<Concept2, Concept, Map>()
        )
    {}

    /**
     * Converts from another set of bindings.
     *
     * \pre Map must be an MPL map with an entry for each placeholder
     *      referred to by @c Concept.  The mapped type should be the
     *      corresponding placeholder in Concept2.
     *
     * \throws std::bad_alloc
     */
    template<class Concept2, class Map>
    binding(const binding<Concept2>& other, const static_binding<Map>&
#ifndef BOOST_TYPE_ERASURE_DOXYGEN
        , typename ::boost::enable_if<
            ::boost::mpl::and_<
                ::boost::type_erasure::detail::check_map<Concept, Map>,
                ::boost::type_erasure::is_subconcept<Concept, Concept2, Map>
            >
        >::type* = 0
#endif
        )
      : impl(
            other,
            static_binding<Map>(),
            ::boost::type_erasure::detail::can_optimize_conversion<Concept2, Concept, Map>()
        )
    {}

    /**
     * Converts from another set of bindings.
     *
     * \pre Map must be an MPL map with an entry for each placeholder
     *      referred to by @c Concept.  The mapped type should be the
     *      corresponding placeholder in Concept2.
     *
     * \throws std::bad_alloc
     * \throws std::bad_any_cast
     */
    template<class Placeholders, class Map>
    binding(const dynamic_binding<Placeholders>& other, const static_binding<Map>&)
      : impl(
            other,
            static_binding<Map>()
        )
    {}

    /**
     * \return true iff the sets of types that the placeholders
     *         bind to are the same for both arguments.
     *
     * \throws Nothing.
     */
    friend bool operator==(const binding& lhs, const binding& rhs)
    { return *lhs.impl.table == *rhs.impl.table; }
    
    /**
     * \return true iff the arguments do not map to identical
     *         sets of types.
     *
     * \throws Nothing.
     */
    friend bool operator!=(const binding& lhs, const binding& rhs)
    { return !(lhs == rhs); }

    /** INTERNAL ONLY */
    template<class T>
    typename T::type find() const { return impl.table->lookup((T*)0); }
private:
    template<class C2>
    friend class binding;
    template<class P>
    friend class dynamic_binding;
    /** INTERNAL ONLY */
    struct impl_type
    {
        impl_type() {
            table = &::boost::type_erasure::detail::make_vtable_init<
                typename ::boost::mpl::transform<
                    actual_concept,
                    ::boost::type_erasure::detail::get_null_vtable_entry<
                        ::boost::mpl::_1
                    >
                >::type,
                table_type
            >::type::value;
        }
        template<class Map>
        impl_type(const static_binding<Map>&)
        {
            table = &::boost::type_erasure::detail::make_vtable_init<
                typename ::boost::mpl::transform<
                    actual_concept,
                    ::boost::type_erasure::detail::rebind_placeholders<
                        ::boost::mpl::_1,
                        typename ::boost::type_erasure::detail::add_deductions<
                            Map,
                            placeholder_subs
                        >::type
                    >
                >::type,
                table_type
            >::type::value;
        }
        template<class Concept2, class Map>
        impl_type(const binding<Concept2>& other, const static_binding<Map>&, boost::mpl::false_)
          : manager(new table_type)
        {
            manager->template convert_from<
                typename ::boost::type_erasure::detail::convert_deductions<
                    Map,
                    placeholder_subs,
                    typename binding<Concept2>::placeholder_subs
                >::type
            >(*other.impl.table);
            table = manager.get();
        }
        template<class PlaceholderList, class Map>
        impl_type(const dynamic_binding<PlaceholderList>& other, const static_binding<Map>&)
          : manager(new table_type)
        {
            manager->template convert_from<
                // FIXME: What do we need to do with deduced placeholder in other
                typename ::boost::type_erasure::detail::add_deductions<
                    Map,
                    placeholder_subs
                >::type
            >(other.impl);
            table = manager.get();
        }
        template<class Concept2, class Map>
        impl_type(const binding<Concept2>& other, const static_binding<Map>&, boost::mpl::true_)
          : table(other.impl.table),
            manager(other.impl.manager)
        {}
        const table_type* table;
        ::boost::shared_ptr<table_type> manager;
    } impl;
};

}
}

#endif
