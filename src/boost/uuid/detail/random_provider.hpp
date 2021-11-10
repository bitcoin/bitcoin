//
// Copyright (c) 2017 James E. King III
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//   https://www.boost.org/LICENSE_1_0.txt)
//
// Platform-specific random entropy provider
//

#ifndef BOOST_UUID_DETAIL_RANDOM_PROVIDER_HPP
#define BOOST_UUID_DETAIL_RANDOM_PROVIDER_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>
#include <boost/move/core.hpp>
#include <boost/move/utility_core.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/uuid/entropy_error.hpp>
#include <climits>
#include <iterator>

// Detection of the platform is separated from inclusion of the correct
// header to facilitate mock testing of the provider implementations.

#include <boost/uuid/detail/random_provider_detect_platform.hpp>
#include <boost/uuid/detail/random_provider_include_platform.hpp>


namespace boost {
namespace uuids {
namespace detail {

//! \brief Contains code common to all random_provider implementations.
//! \note  random_provider_base is required to provide this method:
//!        void get_random_bytes(void *buf, size_t siz);
//! \note  noncopyable because of some base implementations so
//!        this makes it uniform across platforms to avoid any  
//!        porting surprises
class random_provider :
    public detail::random_provider_base
{
    BOOST_MOVABLE_BUT_NOT_COPYABLE(random_provider)

public:
    BOOST_DEFAULTED_FUNCTION(random_provider(), {})

    random_provider(BOOST_RV_REF(random_provider) that) BOOST_NOEXCEPT :
        detail::random_provider_base(boost::move(static_cast< detail::random_provider_base& >(that)))
    {
    }

    random_provider& operator= (BOOST_RV_REF(random_provider) that) BOOST_NOEXCEPT
    {
        static_cast< detail::random_provider_base& >(*this) = boost::move(static_cast< detail::random_provider_base& >(that));
        return *this;
    }

    //! Leverage the provider as a SeedSeq for
    //! PseudoRandomNumberGeneration seeing.
    //! \note: See Boost.Random documentation for more details
    template<class Iter>
    void generate(Iter first, Iter last)
    {
        typedef typename std::iterator_traits<Iter>::value_type value_type;
        BOOST_STATIC_ASSERT(is_integral<value_type>::value);
        BOOST_STATIC_ASSERT(is_unsigned<value_type>::value);
        BOOST_STATIC_ASSERT(sizeof(value_type) * CHAR_BIT >= 32);

        for (; first != last; ++first)
        {
            get_random_bytes(&*first, sizeof(*first));
            *first &= (std::numeric_limits<boost::uint32_t>::max)();
        }
    }

    //! Return the name of the selected provider
    const char * name() const
    {
        return BOOST_UUID_RANDOM_PROVIDER_STRINGIFY(BOOST_UUID_RANDOM_PROVIDER_NAME);
    }
};

} // detail
} // uuids
} // boost

#endif // BOOST_UUID_DETAIL_RANDOM_PROVIDER_HPP
