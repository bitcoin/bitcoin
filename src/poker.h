// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POKER_H
#define POKER_H

#include <validationinterface.h>
#include <node/context.h>
#include <map>
#include <string>
#include <wallet/wallet.h>

enum class Suit { Hearts, Diamonds, Spades, Clubs };

enum class Rank { Two = 2, Three, Four, Five, Six, Seven, Eight,
                  Nine, Ten, Jack, Queen, King, Ace };

enum HandRank : uint32_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8 // Includes Royal Flush
};

enum class PokerRound { 
    PreFlop = 0,
    Flop = 1, 
    Turn = 2, 
    River = 3,
    Showdown = 4,
    Fold = 5
};

struct PokerConstants {
    static constexpr uint32_t RANK_SHIFT = 20;
    static constexpr uint32_t HIGH_CARD_SHIFT = 16;
    static constexpr uint32_t SECOND_CARD_SHIFT = 12;
    static constexpr uint32_t THIRD_CARD_SHIFT = 8;
    static constexpr uint32_t FOURTH_CARD_SHIFT = 4;
};
                

struct Card {
    Rank rank;
    Suit suit;

    std::string toString() const {
        static const std::map<Rank, std::string> rankStrings = {
            {Rank::Two, "2"}, {Rank::Three, "3"}, {Rank::Four, "4"},
            {Rank::Five, "5"}, {Rank::Six, "6"}, {Rank::Seven, "7"},
            {Rank::Eight, "8"}, {Rank::Nine, "9"}, {Rank::Ten, "10"},
            {Rank::Jack, "J"}, {Rank::Queen, "Q"}, {Rank::King, "K"},
            {Rank::Ace, "A"}
        };

        static const std::map<Suit, std::string> suitStrings = {
            {Suit::Hearts, "♥"}, {Suit::Diamonds, "♦"},
            {Suit::Spades, "♠"}, {Suit::Clubs, "♣"}
        };

        return rankStrings.at(rank) + suitStrings.at(suit);
    }
};

struct GameState {
    std::vector<Card> playerHand;
    std::vector<Card> satoshisHand;
    uint32_t playerScore;
    uint32_t satoshisScore;
    std::vector<Card> deck;
    uint256 currentBlockHash;
    uint256 gameBlockHash;
    uint256 gameHash;
    std::vector<Card> board;
    std::string playerAddress;

    PokerRound round = PokerRound::PreFlop; 

    GameState() {
        deck.reserve(52);
    }

    void ClearGameState() {
        playerHand.clear();
        satoshisHand.clear();
        playerScore = 0;
        satoshisScore = 0;
        deck.clear();
        board.clear();
        round = PokerRound::PreFlop;
    }
};


class PokerWorker final : public CValidationInterface
{
public:
    PokerWorker(node::NodeContext& node) : m_node(node) {
    }
    PokerWorker() = delete;
    ~PokerWorker()
    {
        while (!m_shutdown) {
        }
    }

    bool IsShutdown()
    {
        return m_shutdown;
    }

    bool Init();

    void Stop();

    void WaitShutdown();

    void ActiveTipChange(const CBlockIndex& new_tip, bool is_ibd) final EXCLUSIVE_LOCKS_REQUIRED(!m_game_state_mutex);

    void NewGame() EXCLUSIVE_LOCKS_REQUIRED(m_game_state_mutex);

    void AdvanceGame() EXCLUSIVE_LOCKS_REQUIRED(!m_game_state_mutex);

    void DealCards();

    uint32_t EvaluateHand(const std::vector<Card>& hand);

    void GenerateDeck();

    void ShuffleDeck();

    void CreateGameHash();

    std::string GetHandDescription(uint32_t score);

    GameState gameState;

private:
    std::atomic<bool> m_running;
    std::atomic<bool> m_shutdown;
    node::NodeContext& m_node;
    std::unique_ptr<interfaces::Handler> m_handler_load_wallet;

    mutable Mutex m_game_state_mutex;
};


#endif // POKER_H
