// (C) Copyright 2013 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_INTERRUPTION_HPP
#define BOOST_THREAD_INTERRUPTION_HPP

#include <boost/thread/detail/config.hpp>

namespace boost
{
    namespace this_thread
    {
        void BOOST_THREAD_DECL interruption_point();
        bool BOOST_THREAD_DECL interruption_enabled() BOOST_NOEXCEPT;
        bool BOOST_THREAD_DECL interruption_requested() BOOST_NOEXCEPT;
    }
}

#endif // header
