// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HEADERSSYNC_PARAMS_H
#define BITCOIN_HEADERSSYNC_PARAMS_H

// The two constants below are computed using the simulation script in
// contrib/devtools/headerssync-params.py.

//! Store one header commitment per HEADER_COMMITMENT_PERIOD blocks.
constexpr size_t HEADER_COMMITMENT_PERIOD{624};

//! Only feed headers to validation once this many headers on top have been
//! received and validated against commitments.
constexpr size_t REDOWNLOAD_BUFFER_SIZE{14827}; // 14827/624 = ~23.8 commitments

#endif // BITCOIN_HEADERSSYNC_PARAMS_H
