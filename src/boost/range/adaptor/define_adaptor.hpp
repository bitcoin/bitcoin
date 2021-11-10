// Boost.Range library
//
//  Copyright Neil Groves 2010. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_DEFINE_ADAPTOR_HPP_INCLUDED
#define BOOST_RANGE_DEFINE_ADAPTOR_HPP_INCLUDED

#include <boost/tuple/tuple.hpp>

#define BOOST_DEFINE_RANGE_ADAPTOR( adaptor_name, range_adaptor ) \
    struct adaptor_name##_forwarder {}; \
    \
    template<typename Range> range_adaptor <Range> \
        operator|(Range& rng, adaptor_name##_forwarder) \
    { \
        return range_adaptor <Range>( rng ); \
    } \
    \
    template<typename Range> range_adaptor <const Range> \
        operator|(const Range& rng, adaptor_name##_forwarder) \
    { \
        return range_adaptor <const Range>( rng ); \
    } \
    \
    static adaptor_name##_forwarder adaptor_name = adaptor_name##_forwarder(); \
    \
    template<typename Range> \
    range_adaptor <Range> \
    make_##adaptor_name(Range& rng) \
    { \
        return range_adaptor <Range>(rng); \
    } \
    \
    template<typename Range> \
    range_adaptor <const Range> \
    make_##adaptor_name(const Range& rng) \
    { \
        return range_adaptor <const Range>(rng); \
    }

#define BOOST_DEFINE_RANGE_ADAPTOR_1( adaptor_name, range_adaptor, arg1_type ) \
    struct adaptor_name \
    { \
        explicit adaptor_name (arg1_type arg1_) \
            : arg1(arg1_) {} \
        arg1_type arg1; \
    }; \
    \
    template<typename Range> range_adaptor <Range> \
        operator|(Range& rng, adaptor_name args) \
    { \
        return range_adaptor <Range>(rng, args.arg1); \
    } \
    \
    template<typename Range> range_adaptor <const Range> \
        operator|(const Range& rng, adaptor_name args) \
    { \
        return range_adaptor <const Range>(rng, args.arg1); \
    } \
    \
    template<typename Range> \
    range_adaptor <Range> \
    make_##adaptor_name(Range& rng, arg1_type arg1) \
    { \
        return range_adaptor <Range>(rng, arg1); \
    } \
    \
    template<typename Range> \
    range_adaptor <const Range> \
    make_##adaptor_name(const Range& rng, arg1_type arg1) \
    { \
        return range_adaptor <const Range>(rng, arg1); \
    }

#define BOOST_RANGE_ADAPTOR_2( adaptor_name, range_adaptor, arg1_type, arg2_type ) \
    struct adaptor_name \
    { \
        explicit adaptor_name (arg1_type arg1_, arg2_type arg2_) \
            : arg1(arg1_), arg2(arg2_) {} \
        arg1_type arg1; \
        arg2_type arg2; \
    }; \
    \
    template<typename Range> range_adaptor <Range> \
    operator|(Range& rng, adaptor_name args) \
    { \
        return range_adaptor <Range>(rng, args.arg1, args.arg2); \
    } \
    template<typename Range> \
    range_adaptor <Range> \
    make_##adaptor_name(Range& rng, arg1_type arg1, arg2_type arg2) \
    { \
        return range_adaptor <Range>(rng, arg1, arg2); \
    } \
    template<typename Range> \
    range_adaptor <const Range> \
    make_##adaptor_name(const Range& rng, arg1_type arg1, arg2_type arg2) \
    { \
        return range_adaptor <const Range>(rng, arg1, arg2); \
    }


#endif // include guard
