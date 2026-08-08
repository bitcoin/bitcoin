// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_FATAL_ERROR_H
#define BITCOIN_KERNEL_FATAL_ERROR_H

#include <attributes.h>
#include <kernel/notifications_interface.h>
#include <util/expected.h>
#include <util/translation.h>

#include <string>
#include <utility>

namespace kernel {

//! A fatal error notification that has already been raised.
//!
//! A FatalError can only be created through Raise(), which fires the
//! fatalError notification. Holding a FatalError therefore denotes that
//! the notification has already fired and must not be fired again while the
//! error is propagated.
class FatalError
{
public:
    //! Fire the fatalError notification and return the error for propagation
    //! through util::Expected.
    [[nodiscard]] static util::Unexpected<FatalError> Raise(Notifications& notifications, const bilingual_str& message)
    {
        notifications.fatalError(message);
        return util::Unexpected{FatalError{message.original}};
    }

    FatalError(FatalError&&) = default;
    FatalError& operator=(FatalError&&) = default;
    FatalError(const FatalError&) = delete;
    FatalError& operator=(const FatalError&) = delete;

    //! The untranslated error message for logging.
    const std::string& message() const LIFETIMEBOUND { return m_message; }

private:
    explicit FatalError(std::string message) : m_message{std::move(message)} {}

    std::string m_message;
};

} // namespace kernel

#endif // BITCOIN_KERNEL_FATAL_ERROR_H
