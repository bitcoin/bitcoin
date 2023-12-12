// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINJOIN_OPTIONS_H
#define BITCOIN_COINJOIN_OPTIONS_H

#include <amount.h>
#include <atomic>
#include <mutex>

// This header is used by both Wallet and Server libraries
class UniValue;

static constexpr int MIN_COINJOIN_SESSIONS = 1;
static constexpr int MIN_COINJOIN_ROUNDS = 2;
static constexpr int MIN_COINJOIN_AMOUNT = 2;
static constexpr int MIN_COINJOIN_DENOMS_GOAL = 10;
static constexpr int MIN_COINJOIN_DENOMS_HARDCAP = 10;
static constexpr int MAX_COINJOIN_SESSIONS = 10;
static constexpr int MAX_COINJOIN_ROUNDS = 16;
static constexpr int MAX_COINJOIN_DENOMS_GOAL = 100000;
static constexpr int MAX_COINJOIN_DENOMS_HARDCAP = 100000;
static constexpr int MAX_COINJOIN_AMOUNT = MAX_MONEY / COIN;
static constexpr int DEFAULT_COINJOIN_SESSIONS = 4;
static constexpr int DEFAULT_COINJOIN_ROUNDS = 4;
static constexpr int DEFAULT_COINJOIN_AMOUNT = 1000;
static constexpr int DEFAULT_COINJOIN_DENOMS_GOAL = 50;
static constexpr int DEFAULT_COINJOIN_DENOMS_HARDCAP = 300;

static constexpr bool DEFAULT_COINJOIN_AUTOSTART = false;
static constexpr bool DEFAULT_COINJOIN_MULTISESSION = false;

// How many new denom outputs to create before we consider the "goal" loop in CreateDenominated
// a final one and start creating an actual tx. Same limit applies for the "hard cap" part of the algo.
// NOTE: We do not allow txes larger than 100kB, so we have to limit the number of outputs here.
// We still want to create a lot of outputs though.
// Knowing that each CTxOut is ~35b big, 400 outputs should take 400 x ~35b = ~17.5kb.
// More than 500 outputs starts to make qt quite laggy.
// Additionally to need all 500 outputs (assuming a max per denom of 50) you'd need to be trying to
// create denominations for over 3000 dash!
static const int COINJOIN_DENOM_OUTPUTS_THRESHOLD = 500;

// Warn user if mixing in gui or try to create backup if mixing in daemon mode
// when we have only this many keys left
static constexpr int COINJOIN_KEYS_THRESHOLD_WARNING = 100;
// Stop mixing completely, it's too dangerous to continue when we have only this many keys left
static constexpr int COINJOIN_KEYS_THRESHOLD_STOP = 50;
// Pseudorandomly mix up to this many times in addition to base round count
static constexpr int COINJOIN_RANDOM_ROUNDS = 3;

/* Application wide mixing options */
class CCoinJoinClientOptions
{
public:
    static int GetSessions() { return CCoinJoinClientOptions::Get().nCoinJoinSessions; }
    static int GetRounds() { return CCoinJoinClientOptions::Get().nCoinJoinRounds; }
    static int GetRandomRounds() { return CCoinJoinClientOptions::Get().nCoinJoinRandomRounds; }
    static int GetAmount() { return CCoinJoinClientOptions::Get().nCoinJoinAmount; }
    static int GetDenomsGoal() { return CCoinJoinClientOptions::Get().nCoinJoinDenomsGoal; }
    static int GetDenomsHardCap() { return CCoinJoinClientOptions::Get().nCoinJoinDenomsHardCap; }

    static void SetEnabled(bool fEnabled);
    static void SetMultiSessionEnabled(bool fEnabled);
    static void SetSessions(int sessions);
    static void SetRounds(int nRounds);
    static void SetAmount(CAmount amount);
    static void SetDenomsGoal(int denoms_goal);
    static void SetDenomsHardCap(int denoms_hardcap);

    static bool IsEnabled() { return CCoinJoinClientOptions::Get().fEnableCoinJoin; }
    static bool IsMultiSessionEnabled() { return CCoinJoinClientOptions::Get().fCoinJoinMultiSession; }

    static void GetJsonInfo(UniValue& obj);

private:
    static CCoinJoinClientOptions* _instance;
    static std::once_flag onceFlag;

    std::atomic<int> nCoinJoinSessions{DEFAULT_COINJOIN_SESSIONS};
    std::atomic<int> nCoinJoinRounds{DEFAULT_COINJOIN_ROUNDS};
    std::atomic<int> nCoinJoinRandomRounds{COINJOIN_RANDOM_ROUNDS};
    std::atomic<int> nCoinJoinAmount{DEFAULT_COINJOIN_AMOUNT};
    std::atomic<int> nCoinJoinDenomsGoal{DEFAULT_COINJOIN_DENOMS_GOAL};
    std::atomic<int> nCoinJoinDenomsHardCap{DEFAULT_COINJOIN_DENOMS_HARDCAP};
    std::atomic<bool> fEnableCoinJoin{false};
    std::atomic<bool> fCoinJoinMultiSession{DEFAULT_COINJOIN_MULTISESSION};

    CCoinJoinClientOptions() = default;

    CCoinJoinClientOptions(const CCoinJoinClientOptions& other) = delete;
    CCoinJoinClientOptions& operator=(const CCoinJoinClientOptions&) = delete;

    static CCoinJoinClientOptions& Get();
    static void Init();
};

#endif // BITCOIN_COINJOIN_OPTIONS_H
