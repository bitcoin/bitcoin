// Boost.Geometry Index
//
// R-tree algorithms parameters
//
// Copyright (c) 2011-2017 Adam Wulkiewicz, Lodz, Poland.
//
// This file was modified by Oracle on 2019-2020.
// Modifications copyright (c) 2019-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_PARAMETERS_HPP
#define BOOST_GEOMETRY_INDEX_PARAMETERS_HPP


#include <limits>

#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/index/detail/exception.hpp>


namespace boost { namespace geometry { namespace index {

namespace detail { 

template <size_t MaxElements>
struct default_min_elements_s
{
    static const size_t raw_value = (MaxElements * 3) / 10;
    static const size_t value = 1 <= raw_value ? raw_value : 1;
};

inline size_t default_min_elements_d()
{
    return (std::numeric_limits<size_t>::max)();
}

inline size_t default_min_elements_d_calc(size_t max_elements, size_t min_elements)
{
    if ( default_min_elements_d() == min_elements )
    {
        size_t raw_value = (max_elements * 3) / 10;
        return 1 <= raw_value ? raw_value : 1;
    }
    
    return min_elements;
}

template <size_t MaxElements>
struct default_rstar_reinserted_elements_s
{
    static const size_t value = (MaxElements * 3) / 10;
};

inline size_t default_rstar_reinserted_elements_d()
{
    return (std::numeric_limits<size_t>::max)();
}

inline size_t default_rstar_reinserted_elements_d_calc(size_t max_elements, size_t reinserted_elements)
{
    if ( default_rstar_reinserted_elements_d() == reinserted_elements )
    {
        return (max_elements * 3) / 10;
    }
    
    return reinserted_elements;
}

} // namespace detail

/*!
\brief Linear r-tree creation algorithm parameters.

\tparam MaxElements     Maximum number of elements in nodes.
\tparam MinElements     Minimum number of elements in nodes. Default: 0.3*Max.
*/
template <size_t MaxElements,
          size_t MinElements = detail::default_min_elements_s<MaxElements>::value>
struct linear
{
    BOOST_GEOMETRY_STATIC_ASSERT((0 < MinElements && 2*MinElements <= MaxElements+1),
        "Invalid MaxElements or MinElements.",
        std::integer_sequence<size_t, MaxElements, MinElements>);

    static const size_t max_elements = MaxElements;
    static const size_t min_elements = MinElements;

    static size_t get_max_elements() { return MaxElements; }
    static size_t get_min_elements() { return MinElements; }
};

/*!
\brief Quadratic r-tree creation algorithm parameters.

\tparam MaxElements     Maximum number of elements in nodes.
\tparam MinElements     Minimum number of elements in nodes. Default: 0.3*Max.
*/
template <size_t MaxElements,
          size_t MinElements = detail::default_min_elements_s<MaxElements>::value>
struct quadratic
{
    BOOST_GEOMETRY_STATIC_ASSERT((0 < MinElements && 2*MinElements <= MaxElements+1),
        "Invalid MaxElements or MinElements.",
        std::integer_sequence<size_t, MaxElements, MinElements>);

    static const size_t max_elements = MaxElements;
    static const size_t min_elements = MinElements;

    static size_t get_max_elements() { return MaxElements; }
    static size_t get_min_elements() { return MinElements; }
};

/*!
\brief R*-tree creation algorithm parameters.

\tparam MaxElements             Maximum number of elements in nodes.
\tparam MinElements             Minimum number of elements in nodes. Default: 0.3*Max.
\tparam ReinsertedElements      The number of elements reinserted by forced reinsertions algorithm.
                                If 0 forced reinsertions are disabled. Maximum value is Max+1-Min.
                                Greater values are truncated. Default: 0.3*Max.
\tparam OverlapCostThreshold    The number of most suitable leafs taken into account while choosing
                                the leaf node to which currently inserted value will be added. If
                                value is in range (0, MaxElements) - the algorithm calculates
                                nearly minimum overlap cost, otherwise all leafs are analyzed
                                and true minimum overlap cost is calculated. Default: 32.
*/
template <size_t MaxElements,
          size_t MinElements = detail::default_min_elements_s<MaxElements>::value,
          size_t ReinsertedElements = detail::default_rstar_reinserted_elements_s<MaxElements>::value,
          size_t OverlapCostThreshold = 32>
struct rstar
{
    BOOST_GEOMETRY_STATIC_ASSERT((0 < MinElements && 2*MinElements <= MaxElements+1),
        "Invalid MaxElements or MinElements.",
        std::integer_sequence<size_t, MaxElements, MinElements>);

    static const size_t max_elements = MaxElements;
    static const size_t min_elements = MinElements;
    static const size_t reinserted_elements = ReinsertedElements;
    static const size_t overlap_cost_threshold = OverlapCostThreshold;

    static size_t get_max_elements() { return MaxElements; }
    static size_t get_min_elements() { return MinElements; }
    static size_t get_reinserted_elements() { return ReinsertedElements; }
    static size_t get_overlap_cost_threshold() { return OverlapCostThreshold; }
};

//template <size_t MaxElements, size_t MinElements>
//struct kmeans
//{
//    static const size_t max_elements = MaxElements;
//    static const size_t min_elements = MinElements;
//};

/*!
\brief Linear r-tree creation algorithm parameters - run-time version.
*/
class dynamic_linear
{
public:
    /*!
    \brief The constructor.

    \param max_elements     Maximum number of elements in nodes.
    \param min_elements     Minimum number of elements in nodes. Default: 0.3*Max.
    */
    explicit dynamic_linear(size_t max_elements,
                            size_t min_elements = detail::default_min_elements_d())
        : m_max_elements(max_elements)
        , m_min_elements(detail::default_min_elements_d_calc(max_elements, min_elements))
    {
        if (!(0 < m_min_elements && 2*m_min_elements <= m_max_elements+1))
            detail::throw_invalid_argument("invalid min or/and max parameters of dynamic_linear");
    }

    size_t get_max_elements() const { return m_max_elements; }
    size_t get_min_elements() const { return m_min_elements; }

private:
    size_t m_max_elements;
    size_t m_min_elements;
};

/*!
\brief Quadratic r-tree creation algorithm parameters - run-time version.
*/
class dynamic_quadratic
{
public:
    /*!
    \brief The constructor.

    \param max_elements     Maximum number of elements in nodes.
    \param min_elements     Minimum number of elements in nodes. Default: 0.3*Max.
    */
    explicit dynamic_quadratic(size_t max_elements,
                               size_t min_elements = detail::default_min_elements_d())
        : m_max_elements(max_elements)
        , m_min_elements(detail::default_min_elements_d_calc(max_elements, min_elements))
    {
        if (!(0 < m_min_elements && 2*m_min_elements <= m_max_elements+1))
            detail::throw_invalid_argument("invalid min or/and max parameters of dynamic_quadratic");
    }

    size_t get_max_elements() const { return m_max_elements; }
    size_t get_min_elements() const { return m_min_elements; }

private:
    size_t m_max_elements;
    size_t m_min_elements;
};

/*!
\brief R*-tree creation algorithm parameters - run-time version.
*/
class dynamic_rstar
{
public:
    /*!
    \brief The constructor.

    \param max_elements             Maximum number of elements in nodes.
    \param min_elements             Minimum number of elements in nodes. Default: 0.3*Max.
    \param reinserted_elements      The number of elements reinserted by forced reinsertions algorithm.
                                    If 0 forced reinsertions are disabled. Maximum value is Max-Min+1.
                                    Greater values are truncated. Default: 0.3*Max.
    \param overlap_cost_threshold   The number of most suitable leafs taken into account while choosing
                                    the leaf node to which currently inserted value will be added. If
                                    value is in range (0, MaxElements) - the algorithm calculates
                                    nearly minimum overlap cost, otherwise all leafs are analyzed
                                    and true minimum overlap cost is calculated. Default: 32.
    */
    explicit dynamic_rstar(size_t max_elements,
                           size_t min_elements = detail::default_min_elements_d(),
                           size_t reinserted_elements = detail::default_rstar_reinserted_elements_d(),
                           size_t overlap_cost_threshold = 32)
        : m_max_elements(max_elements)
        , m_min_elements(detail::default_min_elements_d_calc(max_elements, min_elements))
        , m_reinserted_elements(detail::default_rstar_reinserted_elements_d_calc(max_elements, reinserted_elements))
        , m_overlap_cost_threshold(overlap_cost_threshold)
    {
        if (!(0 < m_min_elements && 2*m_min_elements <= m_max_elements+1))
            detail::throw_invalid_argument("invalid min or/and max parameters of dynamic_rstar");
    }

    size_t get_max_elements() const { return m_max_elements; }
    size_t get_min_elements() const { return m_min_elements; }
    size_t get_reinserted_elements() const { return m_reinserted_elements; }
    size_t get_overlap_cost_threshold() const { return m_overlap_cost_threshold; }

private:
    size_t m_max_elements;
    size_t m_min_elements;
    size_t m_reinserted_elements;
    size_t m_overlap_cost_threshold;
};


template <typename Parameters, typename Strategy>
class parameters
    : public Parameters
    , private Strategy
{
public:
    parameters()
        : Parameters(), Strategy()
    {}

    parameters(Parameters const& params)
        : Parameters(params), Strategy()
    {}

    parameters(Parameters const& params, Strategy const& strategy)
        : Parameters(params), Strategy(strategy)
    {}

    Strategy const& strategy() const
    {
        return static_cast<Strategy const&>(*this);
    }
};


namespace detail
{

template <typename Parameters>
struct strategy_type
{
    typedef default_strategy type;
    typedef default_strategy result_type;
};

template <typename Parameters, typename Strategy>
struct strategy_type< parameters<Parameters, Strategy> >
{
    typedef Strategy type;
    typedef Strategy const& result_type;
};


template <typename Parameters>
struct get_strategy_impl
{
    static inline default_strategy apply(Parameters const&)
    {
        return default_strategy();
    }
};

template <typename Parameters, typename Strategy>
struct get_strategy_impl<parameters<Parameters, Strategy> >
{
    static inline Strategy const& apply(parameters<Parameters, Strategy> const& parameters)
    {
        return parameters.strategy();
    }
};

template <typename Parameters>
inline typename strategy_type<Parameters>::result_type
    get_strategy(Parameters const& parameters)
{
    return get_strategy_impl<Parameters>::apply(parameters);
}

} // namespace detail


}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_INDEX_PARAMETERS_HPP
