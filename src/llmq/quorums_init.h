// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_INIT_H
#define SYSCOIN_LLMQ_QUORUMS_INIT_H

class CDBWrapper;
class CConnman;
class BanMan;
class PeerManager;
class ChainstateManager;
namespace llmq
{

// If true, we will connect to all new quorums and watch their communication
static const bool DEFAULT_WATCH_QUORUMS = false;

// Init/destroy LLMQ globals
void InitLLMQSystem(bool unitTests, CConnman& connman, BanMan& banman, PeerManager& peerman, ChainstateManager& chainman, bool fWipe = false);
void DestroyLLMQSystem();

// Manage scheduled tasks, threads, listeners etc.
void StartLLMQSystem();
void StopLLMQSystem();
void InterruptLLMQSystem();
} // namespace llmq

#endif //SYSCOIN_LLMQ_QUORUMS_INIT_H
