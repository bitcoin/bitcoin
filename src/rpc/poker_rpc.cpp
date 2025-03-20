// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <poker.h>
#include <rpc/server.h>
#include <rpc/server_util.h>

using node::NodeContext;

static std::string renderCards(const std::vector<Card>& cards) {
    std::vector<std::string> cardLines(7);

    for (const auto& card : cards) {
        std::string cardStr = card.toString();
        std::string rank, suit;

        if (cardStr.size() >= 4 && (unsigned char)cardStr[cardStr.size() - 3] >= 0xE2) {
            suit = cardStr.substr(cardStr.size() - 3);
            rank = cardStr.substr(0, cardStr.size() - 3);
        } else {
            suit = cardStr.substr(cardStr.size() - 1);
            rank = cardStr.substr(0, cardStr.size() - 1);
        }

        bool isTwoChar = (rank.size() == 2);
        std::string topRank    = isTwoChar ? rank + "      " : rank + "       ";
        std::string bottomRank = isTwoChar ? "      " + rank : "       " + rank;

        cardLines[0] += "┌─────────┐";
        cardLines[1] += "│ " + topRank + "│";
        cardLines[2] += "│         │";
        cardLines[3] += "│    " + suit + "    │";
        cardLines[4] += "│         │";
        cardLines[5] += "│" + bottomRank + " │";
        cardLines[6] += "└─────────┘";
    }

    return std::accumulate(cardLines.begin(), cardLines.end(), std::string(),
        [](const std::string& a, const std::string& b) {
            return a + b + "\n";
        });
}

static std::string renderHiddenCards() {
    std::vector<std::string> cardLines(7);

    for (int i = 0; i < 2; i++) {
        cardLines[0] += "┌─────────┐";
        cardLines[1] += "│ ******* │";
        cardLines[2] += "│ ******* │";
        cardLines[3] += "│ ******* │"; 
        cardLines[4] += "│ ******* │";
        cardLines[5] += "│ ******* │";
        cardLines[6] += "└─────────┘";
    }

    return std::accumulate(cardLines.begin(), cardLines.end(), std::string(),
        [](const std::string& a, const std::string& b) {
            return a + b + "\n";
        });
}

static UniValue handlePlay(NodeContext &node) {
    auto& poker_worker = node.poker_worker;

    if (poker_worker->gameState.gameHash == uint256::ZERO) {
        throw JSONRPCError(RPC_MISC_ERROR, "Load a wallet to play!");
    }
   
    const std::string roundNames[] = {"The Pre-Flop:", "The Flop:", "The Turn:", "The River:", "The Showdown:", "Folded"};
    PokerRound round = poker_worker->gameState.round;
    
    poker_worker->AdvanceGame();
    std::string block_hash = poker_worker->gameState.gameBlockHash.GetHex();
   

    std::string gui = "[Poker]\n[Block: " + block_hash + "]\n\n";

    std::string playerHand, satoshisHand, satoshisScore, playerScore, result, endText;
    if (round == PokerRound::Showdown) {
        playerHand = renderCards(poker_worker->gameState.playerHand);
        satoshisHand = renderCards(poker_worker->gameState.satoshisHand);
        playerScore = poker_worker->GetHandDescription(poker_worker->gameState.playerScore);
        satoshisScore = poker_worker->GetHandDescription(poker_worker->gameState.satoshisScore);
        if (poker_worker->gameState.playerScore > poker_worker->gameState.satoshisScore) {
            result = " You win! Wait for the next block to play again.";
        } else if (poker_worker->gameState.playerScore < poker_worker->gameState.satoshisScore) {
            result = " Satoshi wins! Wait for the next block to play again.";
        } else {
            result = " It's a tie! Wait for the next block to play again.";
        }
    } else if (round == PokerRound::Fold) {
        playerHand = renderHiddenCards();
        satoshisHand = renderHiddenCards();
        endText = "You folded! Wait for the next block to play again.";
    } else {
        playerHand = renderCards(poker_worker->gameState.playerHand);
        satoshisHand = renderHiddenCards();
        endText = "Use rpc command (poker play) to advance the game or fold with (poker fold).";
    }

    gui += "Your Hand: " + playerScore + "\n" + playerHand
           + roundNames[static_cast<int>(round)] + result
           + "\n" + renderCards(poker_worker->gameState.board) 
           + "Satoshi's Hand: " + satoshisScore + "\n" + satoshisHand
           + endText + "\n";

    return UniValue(gui);
}


static UniValue handleFold(NodeContext &node) {
    auto& poker_worker = node.poker_worker;
    if (poker_worker->gameState.round == PokerRound::PreFlop) {
        return UniValue("You cannot fold before the cards are dealt!!");
    }
    if (poker_worker->gameState.round == PokerRound::Showdown) {
        return UniValue("Game is already over!! Wait for the next block to play again.");
    }
    if (poker_worker->gameState.round == PokerRound::Fold) {
        return UniValue("You already folded!! Wait for the next block to play again.");
    }
    poker_worker->gameState.round = PokerRound::Fold;
    return UniValue("You folded!! Wait for the next block to play again.");
}

static RPCHelpMan poker()
{
    return RPCHelpMan{"poker",
        "Play a round of poker against your node! Use move 'play', or 'fold'.",
        {
            {"move", RPCArg::Type::STR, RPCArg::Optional::NO, "The poker move 'play', or 'fold'."}
        },
        RPCResult{
            RPCResult::Type::STR, "", "The result of the poker action."
        },
        RPCExamples{
            HelpExampleCli("poker", "\"play\"")
            + HelpExampleRpc("poker", "\"play\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {   
            std::string subcommand = request.params[0].get_str();
            NodeContext &node = EnsureAnyNodeContext(request.context);

            if (!node.poker_worker) {
                throw JSONRPCError(RPC_MISC_ERROR, "Poker not initialized, start bitcoind with -poker flag");
            }

            if (subcommand == "play") {
                return handlePlay(node);
            } else if (subcommand == "fold") {
                return handleFold(node);
            } else {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown move, use 'play', or 'fold'");
            }
        },
    };
}


void RegisterPokerRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"poker", &poker},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}

