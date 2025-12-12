// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit

//! @file util/messages.h defines helper functions types that can be used with
//! util::Result class to for dealing with error and warning messages. It keeps
//! message representation separate from implementation details of the Result
//! class allowing each to be understood to changed more easily.

#ifndef BITCOIN_UTIL_MESSAGES_H
#define BITCOIN_UTIL_MESSAGES_H

#include <util/translation.h>

namespace util {
//! Default MessagesType for Result class, simple list of errors and warnings.
struct Messages {
    std::vector<bilingual_str> errors{};
    std::vector<bilingual_str> warnings{};
};

//! Wrapper object to pass an error string to the Result constructor.
struct Error {
    bilingual_str message;
};
//! Wrapper object to pass a warning string to the Result constructor.
struct Warning {
    bilingual_str message;
};

//! Helper function to join messages in space separated string.
bilingual_str JoinMessages(const Messages& messages);

//! Helper function to move messages from one instance to another.
void MoveMessages(Messages& dst, Messages& src);

//! Join error and warning messages in a space separated string. This is
//! intended for simple applications where there's probably only one error or
//! warning message to report, but multiple messages should not be lost if they
//! are present. More complicated applications should use Result::messages_ptr()
//! method directly.
template <typename Result>
bilingual_str ErrorString(const Result& result)
{
    const auto* messages{result.messages_ptr()};
    return messages ? JoinMessages(*messages) : bilingual_str{};
}
} // namespace util

#endif // BITCOIN_UTIL_MESSAGES_H
