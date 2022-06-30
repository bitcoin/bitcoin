// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_DESIGNATOR_H
#define BITCOIN_UTIL_DESIGNATOR_H

/**
 * Designated initializers can be used to avoid ordering mishaps in aggregate
 * initialization. However, they do not prevent uninitialized members. The
 * checks can be disabled by defining DISABLE_DESIGNATED_INITIALIZER_ERRORS.
 * This should only be needed on MSVC 2019. MSVC 2022 supports them with the
 * option "/std:c++20"
 */
#ifndef DISABLE_DESIGNATED_INITIALIZER_ERRORS
#define Desig(field_name) .field_name =
#else
#define Desig(field_name)
#endif

#endif // BITCOIN_UTIL_DESIGNATOR_H
