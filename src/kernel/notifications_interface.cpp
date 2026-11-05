// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/notifications_interface.h>

#include <util/expected.h>
#include <util/translation.h>

#include <utility>

namespace kernel {

util::Unexpected<FatalError> FatalError::Raise(Notifications& notifications, bilingual_str message)
{
    notifications.fatalError(message);
    return util::Unexpected{FatalError{std::move(message.original)}};
}

util::Unexpected<FatalError> FatalError::RaiseFlush(Notifications& notifications, bilingual_str message)
{
    notifications.flushError(message);
    return util::Unexpected{FatalError{std::move(message.original)}};
}

} // namespace kernel
