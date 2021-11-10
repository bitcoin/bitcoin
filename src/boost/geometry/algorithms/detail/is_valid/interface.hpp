// Boost.Geometry

// Copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_INTERFACE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_INTERFACE_HPP

#include <sstream>
#include <string>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant_fwd.hpp>

#include <boost/geometry/algorithms/dispatch/is_valid.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/policies/is_valid/default_policy.hpp>
#include <boost/geometry/policies/is_valid/failing_reason_policy.hpp>
#include <boost/geometry/policies/is_valid/failure_type_policy.hpp>
#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/services.hpp>


namespace boost { namespace geometry
{
    
namespace resolve_strategy
{

template
<
    typename Strategy,
    bool IsUmbrella = strategies::detail::is_umbrella_strategy<Strategy>::value
>
struct is_valid
{
    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const& geometry,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        return dispatch::is_valid<Geometry>::apply(geometry, visitor, strategy);
    }

};

template <typename Strategy>
struct is_valid<Strategy, false>
{
    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const& geometry,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        using strategies::relate::services::strategy_converter;
        return dispatch::is_valid
            <
                Geometry
            >::apply(geometry, visitor,
                     strategy_converter<Strategy>::get(strategy));
    }
};

template <>
struct is_valid<default_strategy, false>
{
    template <typename Geometry, typename VisitPolicy>
    static inline bool apply(Geometry const& geometry,
                             VisitPolicy& visitor,
                             default_strategy)
    {
        // NOTE: Currently the strategy is only used for Areal geometries
        typedef typename strategies::relate::services::default_strategy
            <
                Geometry, Geometry
            >::type strategy_type;

        return dispatch::is_valid<Geometry>::apply(geometry, visitor,
                                                   strategy_type());
    }
};

} // namespace resolve_strategy

namespace resolve_variant
{

template <typename Geometry>
struct is_valid
{
    template <typename VisitPolicy, typename Strategy>
    static inline bool apply(Geometry const& geometry,
                             VisitPolicy& visitor,
                             Strategy const& strategy)
    {
        concepts::check<Geometry const>();

        return resolve_strategy::is_valid
                <
                    Strategy
                >::apply(geometry, visitor, strategy);
    }
};

template <BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct is_valid<boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> >
{
    template <typename VisitPolicy, typename Strategy>
    struct visitor : boost::static_visitor<bool>
    {
        visitor(VisitPolicy& policy, Strategy const& strategy)
            : m_policy(policy)
            , m_strategy(strategy)
        {}

        template <typename Geometry>
        bool operator()(Geometry const& geometry) const
        {
            return is_valid<Geometry>::apply(geometry, m_policy, m_strategy);
        }

        VisitPolicy& m_policy;
        Strategy const& m_strategy;
    };

    template <typename VisitPolicy, typename Strategy>
    static inline bool
    apply(boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> const& geometry,
          VisitPolicy& policy_visitor,
          Strategy const& strategy)
    {
        return boost::apply_visitor(visitor<VisitPolicy, Strategy>(policy_visitor, strategy),
                                    geometry);
    }
};

} // namespace resolve_variant


// Undocumented for now
template <typename Geometry, typename VisitPolicy, typename Strategy>
inline bool is_valid(Geometry const& geometry,
                     VisitPolicy& visitor,
                     Strategy const& strategy)
{
    return resolve_variant::is_valid<Geometry>::apply(geometry, visitor, strategy);
}


/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Is_valid}
\param geometry \param_geometry
\param strategy \param_strategy{is_valid}
\return \return_check{is valid (in the OGC sense);
furthermore, the following geometries are considered valid:
multi-geometries with no elements,
linear geometries containing spikes,
areal geometries with duplicate (consecutive) points}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/is_valid.qbk]}
*/
template <typename Geometry, typename Strategy>
inline bool is_valid(Geometry const& geometry, Strategy const& strategy)
{
    is_valid_default_policy<> visitor;
    return resolve_variant::is_valid<Geometry>::apply(geometry, visitor, strategy);
}

/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_check{is valid (in the OGC sense);
    furthermore, the following geometries are considered valid:
    multi-geometries with no elements,
    linear geometries containing spikes,
    areal geometries with duplicate (consecutive) points}

\qbk{[include reference/algorithms/is_valid.qbk]}
*/
template <typename Geometry>
inline bool is_valid(Geometry const& geometry)
{
    return is_valid(geometry, default_strategy());
}


/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Is_valid}
\param geometry \param_geometry
\param failure An enumeration value indicating that the geometry is
    valid or not, and if not valid indicating the reason why
\param strategy \param_strategy{is_valid}
\return \return_check{is valid (in the OGC sense);
    furthermore, the following geometries are considered valid:
    multi-geometries with no elements,
    linear geometries containing spikes,
    areal geometries with duplicate (consecutive) points}

\qbk{distinguish,with failure value and strategy}
\qbk{[include reference/algorithms/is_valid_with_failure.qbk]}
*/
template <typename Geometry, typename Strategy>
inline bool is_valid(Geometry const& geometry, validity_failure_type& failure, Strategy const& strategy)
{
    failure_type_policy<> visitor;
    bool result = resolve_variant::is_valid<Geometry>::apply(geometry, visitor, strategy);
    failure = visitor.failure();
    return result;
}

/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param failure An enumeration value indicating that the geometry is
    valid or not, and if not valid indicating the reason why
\return \return_check{is valid (in the OGC sense);
    furthermore, the following geometries are considered valid:
    multi-geometries with no elements,
    linear geometries containing spikes,
    areal geometries with duplicate (consecutive) points}

\qbk{distinguish,with failure value}
\qbk{[include reference/algorithms/is_valid_with_failure.qbk]}
*/
template <typename Geometry>
inline bool is_valid(Geometry const& geometry, validity_failure_type& failure)
{
    return is_valid(geometry, failure, default_strategy());
}


/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Is_valid}
\param geometry \param_geometry
\param message A string containing a message stating if the geometry
    is valid or not, and if not valid a reason why
\param strategy \param_strategy{is_valid}
\return \return_check{is valid (in the OGC sense);
    furthermore, the following geometries are considered valid:
    multi-geometries with no elements,
    linear geometries containing spikes,
    areal geometries with duplicate (consecutive) points}

\qbk{distinguish,with message and strategy}
\qbk{[include reference/algorithms/is_valid_with_message.qbk]}
*/
template <typename Geometry, typename Strategy>
inline bool is_valid(Geometry const& geometry, std::string& message, Strategy const& strategy)
{
    std::ostringstream stream;
    failing_reason_policy<> visitor(stream);
    bool result = resolve_variant::is_valid<Geometry>::apply(geometry, visitor, strategy);
    message = stream.str();
    return result;
}

/*!
\brief \brief_check{is valid (in the OGC sense)}
\ingroup is_valid
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\param message A string containing a message stating if the geometry
    is valid or not, and if not valid a reason why
\return \return_check{is valid (in the OGC sense);
    furthermore, the following geometries are considered valid:
    multi-geometries with no elements,
    linear geometries containing spikes,
    areal geometries with duplicate (consecutive) points}

\qbk{distinguish,with message}
\qbk{[include reference/algorithms/is_valid_with_message.qbk]}
*/
template <typename Geometry>
inline bool is_valid(Geometry const& geometry, std::string& message)
{
    return is_valid(geometry, message, default_strategy());
}


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_INTERFACE_HPP
