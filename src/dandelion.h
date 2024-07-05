// Copyright (c) 2023-2023 The Navio Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DANDELION_H
#define BITCOIN_DANDELION_H

/** Default value for -dandelion option */
static constexpr bool DEFAULT_DANDELION_ENABLED{true};

/** The minimum amount of time a Dandelion++ transaction is embargoed (seconds) */
static constexpr auto DANDELION_EMBARGO_MIN = 10s;

/** The average additional embargo time beyond the minimum amount (seconds) */
static constexpr auto DANDELION_EMBARGO_AVG = 20s;

/** Probability (percentage) that a Dandelion++ transaction enters fluff phase */
static const int DANDELION_FLUFF_CHANCE = 10;

/** Maximum number of outbound peers designated as Dandelion++ destinations */
static const int DANDELION_MAX_ROUTES = 2;

/** Expected time between Dandelion++ routing shuffles (in seconds). */
static constexpr auto DANDELION_SHUFFLE_INTERVAL = 600s;

#endif // BITCOIN_DANDELION_H
