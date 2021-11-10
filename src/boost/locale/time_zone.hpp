//
//  Copyright (c) 2009-2011 Artyom Beilis (Tonkikh)
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_LOCALE_TIME_ZONE_HPP_INCLUDED
#define BOOST_LOCALE_TIME_ZONE_HPP_INCLUDED

#include <boost/locale/config.hpp>
#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4275 4251 4231 4660)
#endif

#include <string>


namespace boost {
    namespace locale {
        ///
        /// \addtogroup date_time
        ///
        /// @{

        ///
        /// \brief namespace that holds functions for operating with global
        /// time zone
        ///
        namespace time_zone {
            ///
            /// Get global time zone identifier. If empty, system time zone is used
            ///
            BOOST_LOCALE_DECL std::string global();
            ///
            /// Set global time zone identifier returning previous one. If empty, system time zone is used
            ///
            BOOST_LOCALE_DECL std::string global(std::string const &new_tz);
        }

        /// @}

    } // locale
} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif


#endif

// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
