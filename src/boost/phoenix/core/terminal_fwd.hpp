/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PHOENIX_CORE_TERMINAL_FWD_HPP
#define BOOST_PHOENIX_CORE_TERMINAL_FWD_HPP

namespace boost { namespace phoenix
{
    namespace rule
    {
        struct argument;
        struct custom_terminal;
        struct terminal;
    }

    template <typename T, typename Dummy = void>
    struct is_custom_terminal;

    template <typename T, typename Dummy = void>
    struct custom_terminal;
}}

#endif
