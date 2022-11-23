//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace immer {

/*!
 * Disables any special *transience* tracking.  To be used when
 * *reference counting* is available instead.
 */
struct no_transience_policy
{
    template <typename>
    struct apply
    {
        struct type
        {
            struct edit
            {};

            struct owner
            {
                operator edit() const { return {}; }
            };

            struct ownee
            {
                ownee& operator=(edit) { return *this; };
                bool can_mutate(edit) const { return false; }
                bool owned() const { return false; }
            };

            static owner noone;
        };
    };
};

template <typename HP>
typename no_transience_policy::apply<HP>::type::owner
    no_transience_policy::apply<HP>::type::noone = {};

} // namespace immer
