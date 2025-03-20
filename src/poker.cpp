// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker.h>
#include <validation.h>
#include <random>
#include <interfaces/wallet.h>
#include <key_io.h>

bool PokerWorker::Init()
{
    LogPrintf("[poker] Initializing PokerWorker...\n");
    bool off = false;
    if (!m_running.compare_exchange_strong(off, true)) {
        LogPrintf("[poker] PokerWorker already running\n");
        return false;
    }
    m_handler_load_wallet = m_node.wallet_loader->handleLoadWallet([this](std::unique_ptr<interfaces::Wallet> wallet) {
        {
            auto playerResult = wallet->getNewDestination(OutputType::BECH32, "Poker");
            if (!playerResult) {
                LogPrintf("[poker] Failed to get new player address \n");
                return;
            }
            CTxDestination playerDestination = playerResult.value();
           

            uint256 latest_block;
            {
                LOCK(cs_main);
                CBlockIndex* tip = m_node.chainman->ActiveChain().Tip();
                if (!tip) { 
                     LogPrintf("[poker] ERROR: Could not get active chain tip.\n");
                     return; 
                }
                latest_block = tip->GetBlockHash();
            }
            
            LOCK(m_game_state_mutex);
            gameState.currentBlockHash = latest_block;
            gameState.gameBlockHash = latest_block;
            gameState.playerAddress = EncodeDestination(playerDestination);

            CreateGameHash();
        }
    });
    std::thread([this] {
        util::ThreadRename(strprintf("pokerworker.%i", 0));
        LogPrintf("[poker] Starting PokerWorker processing thread\n");
        m_shutdown = true;
        m_shutdown.notify_one();
        LogPrintf("[poker] PokerWorker processing thread stopped\n");
    }).detach();
    LogPrintf("[poker] PokerWorker initialized successfully\n");
    return true;
}

void PokerWorker::WaitShutdown()
{
    m_shutdown.wait(false);
}

void PokerWorker::Stop()
{
    LogDebug(BCLog::POKER, "Stopping Poker Worker\n");
    m_running = false;
}

void PokerWorker::CreateGameHash()
{
    std::string gameHashStr = gameState.gameBlockHash.GetHex() + gameState.playerAddress;
    HashWriter hasher{};
    hasher << gameHashStr;
    uint256 gameHash = hasher.GetHash();
    gameState.gameHash = gameHash;
}

void PokerWorker::ActiveTipChange(const CBlockIndex& new_tip, bool is_ibd)
{
    if (is_ibd) return;
    if (!m_running) return;
    
    uint256 latest_block = new_tip.GetBlockHash();
    if (gameState.round == PokerRound::Showdown || gameState.round == PokerRound::Fold) {
        LOCK(m_game_state_mutex);
        gameState.round = PokerRound::PreFlop;
        gameState.gameBlockHash = latest_block;
    }
    gameState.currentBlockHash = latest_block;
}

void PokerWorker::NewGame() {

    if (!m_node.wallet_loader) {
        LogPrintf("[poker] No wallet loader available\n");
        return;
    }

    if (gameState.playerAddress.empty()) {
        LogPrintf("[poker] No wallets loaded\n");
        return;
    }

    gameState.gameBlockHash = gameState.currentBlockHash;
    CreateGameHash();

    gameState.ClearGameState();
    GenerateDeck();
    DealCards();
    LogPrintf("[poker] New game started with block hash: %s\n", gameState.gameBlockHash.GetHex());
}

void PokerWorker::GenerateDeck() {
    gameState.deck.reserve(52);
    for (int s = 0; s < 4; ++s) {
        for (int r = 2; r <= 14; ++r) {
            gameState.deck.emplace_back(static_cast<Rank>(r), static_cast<Suit>(s));
        }
    }
    ShuffleDeck();
}

void PokerWorker::AdvanceGame() {
    LOCK(m_game_state_mutex);

    if (gameState.round == PokerRound::Fold || gameState.round == PokerRound::Showdown) {
        if (gameState.currentBlockHash != gameState.gameBlockHash) {
            gameState.round = PokerRound::PreFlop;
            return;
        }
    }

    if (gameState.round == PokerRound::PreFlop) {
        NewGame();
        return;
    }

    // Burn a card
    gameState.deck.pop_back();
    
    // The flop
    if (gameState.round == PokerRound::Flop) {
        gameState.board.push_back(gameState.deck.back());
        gameState.deck.pop_back();
        gameState.board.push_back(gameState.deck.back());
        gameState.deck.pop_back();
        gameState.board.push_back(gameState.deck.back());
        gameState.deck.pop_back();

        gameState.round = PokerRound::Turn;
    // The turn
    } else if (gameState.round == PokerRound::Turn) {
        gameState.board.push_back(gameState.deck.back());
        gameState.deck.pop_back();

        gameState.round = PokerRound::River;
    // The river
    } else if (gameState.round == PokerRound::River) {
        gameState.board.push_back(gameState.deck.back());
        gameState.deck.pop_back();

        gameState.playerScore = EvaluateHand(gameState.playerHand);
        gameState.satoshisScore = EvaluateHand(gameState.satoshisHand);

        gameState.round = PokerRound::Showdown;
    }
}

void PokerWorker::ShuffleDeck() {
    std::string seedStr = gameState.gameHash.GetHex();
    std::seed_seq seed(seedStr.begin(), seedStr.end());
    std::mt19937 rng(seed);
    std::shuffle(gameState.deck.begin(), gameState.deck.end(), rng);
}

void PokerWorker::DealCards() {    
    for (int i = 0; i < 2; i++) {
        gameState.playerHand.push_back(gameState.deck.back());
        gameState.deck.pop_back();
        
        gameState.satoshisHand.push_back(gameState.deck.back());
        gameState.deck.pop_back();
    }

    gameState.round = PokerRound::Flop;
}

uint32_t PokerWorker::EvaluateHand(const std::vector<Card>& hand) {

    if (hand.size() != 2) {
        throw std::invalid_argument("Hand must contain exactly 2 cards.");
    }

    if (gameState.board.size() != 5) {
        throw std::invalid_argument("Board must contain exactly 5 cards.");
    }

    auto rankValue = [](Rank r) -> int {
        return (r == Rank::Ace) ? 14 : static_cast<int>(r);
    };
    
    std::vector<Card> combinedCards = hand;
    combinedCards.insert(combinedCards.end(), gameState.board.begin(), gameState.board.end());
    
    uint32_t bestScore = 0;
    const size_t totalCards = combinedCards.size();
    
    if (totalCards >= 5) {
        // Bit mask for combinations: 1 means card is selected
        const size_t maxMask = 1 << totalCards;
        for (size_t mask = 0; mask < maxMask; ++mask) {
            if (__builtin_popcount(mask) != 5) {
                continue;
            }
            
            std::array<Card, 5> fiveCardHand;
            size_t index = 0;
            for (size_t i = 0; i < totalCards; ++i) {
                if (mask & (1 << i)) {
                    fiveCardHand[index++] = combinedCards[i];
                }
            }
            
            std::sort(fiveCardHand.begin(), fiveCardHand.end(), 
                      [&rankValue](const Card& a, const Card& b) {
                          return rankValue(a.rank) > rankValue(b.rank);
                      });
            
            std::array<int, 5> ranks;
            std::map<int, int> rankCounts;
            
            bool isFlush = true;
            Suit firstSuit = fiveCardHand[0].suit;
            
            for (size_t i = 0; i < 5; ++i) {
                ranks[i] = rankValue(fiveCardHand[i].rank);
                rankCounts[ranks[i]]++;
                
                if (fiveCardHand[i].suit != firstSuit) {
                    isFlush = false;
                }
            }
            
            // Check straight
            bool isStraight = true;
            for (size_t i = 0; i < 4; ++i) {
                if (ranks[i] != ranks[i+1] + 1) {
                    isStraight = false;
                    break;
                }
            }
            // Special A-5 straight check
            bool isAceLowStraight = (ranks[0] == 14 && ranks[1] == 5 &&
                                     ranks[2] == 4 && ranks[3] == 3 && ranks[4] == 2);
            if (isAceLowStraight) {
                isStraight = true;
            }
            
            uint32_t score = 0;
            if (isStraight && isFlush) {
                // Straight Flush
                int highCard = (isAceLowStraight ? 5 : ranks[0]);
                score = (static_cast<uint32_t>(HandRank::STRAIGHT_FLUSH) << PokerConstants::RANK_SHIFT)
                        | (highCard << PokerConstants::HIGH_CARD_SHIFT);
            }
            else {
                int fourRank = 0, threeRank = 0;
                std::array<int, 2> pairRanks = {0, 0};
                int pairCount = 0;
                
                for (auto& [r, c] : rankCounts) {
                    if      (c == 4) fourRank = r;
                    else if (c == 3) threeRank = r;
                    else if (c == 2) {
                        if (pairCount < 2) {
                            pairRanks[pairCount++] = r;
                        }
                    }
                }
                
                if (pairCount > 1 && pairRanks[0] < pairRanks[1]) {
                    std::swap(pairRanks[0], pairRanks[1]);
                }
                
                // Collect kickers (ranks not in the main "set")
                std::vector<int> kickers;
                for (int r : ranks) {
                    // Skip ranks that are part of the quads/trips/pairs
                    if (r == fourRank) continue;
                    if (r == threeRank) continue;
                    if (r == pairRanks[0]) continue;
                    if (r == pairRanks[1]) continue;
                    kickers.push_back(r);
                }
                
                if (fourRank > 0) {
                    // Four of a kind
                    // [Primary rank in HIGH_CARD_SHIFT, kicker in SECOND_CARD_SHIFT]
                    score = (static_cast<uint32_t>(HandRank::FOUR_OF_A_KIND) << PokerConstants::RANK_SHIFT)
                            | (fourRank << PokerConstants::HIGH_CARD_SHIFT);
                    if (!kickers.empty()) {
                        score |= (kickers[0] << PokerConstants::SECOND_CARD_SHIFT);
                    }
                }
                else if (threeRank > 0 && pairCount > 0) {
                    // Full House
                    // [Trips rank in HIGH_CARD_SHIFT, pair rank in SECOND_CARD_SHIFT]
                    score = (static_cast<uint32_t>(HandRank::FULL_HOUSE) << PokerConstants::RANK_SHIFT)
                            | (threeRank << PokerConstants::HIGH_CARD_SHIFT)
                            | (pairRanks[0] << PokerConstants::SECOND_CARD_SHIFT);
                }
                else if (isFlush) {
                    // Flush
                    // [Cards in descending order: ranks[0] is highest, etc.]
                    score = (static_cast<uint32_t>(HandRank::FLUSH) << PokerConstants::RANK_SHIFT)
                            | (ranks[0] << PokerConstants::HIGH_CARD_SHIFT)
                            | (ranks[1] << PokerConstants::SECOND_CARD_SHIFT)
                            | (ranks[2] << PokerConstants::THIRD_CARD_SHIFT)
                            | (ranks[3] << PokerConstants::FOURTH_CARD_SHIFT)
                            | (ranks[4]);
                }
                else if (isStraight) {
                    // Straight
                    int highCard = isAceLowStraight ? 5 : ranks[0];
                    score = (static_cast<uint32_t>(HandRank::STRAIGHT) << PokerConstants::RANK_SHIFT)
                            | (highCard << PokerConstants::HIGH_CARD_SHIFT);
                }
                else if (threeRank > 0) {
                    // Three of a kind
                    // [Trip rank is in HIGH_CARD_SHIFT, then 2 kickers]
                    score = (static_cast<uint32_t>(HandRank::THREE_OF_A_KIND) << PokerConstants::RANK_SHIFT)
                            | (threeRank << PokerConstants::HIGH_CARD_SHIFT);
                    if (kickers.size() >= 1) {
                        score |= (kickers[0] << PokerConstants::SECOND_CARD_SHIFT);
                    }
                    if (kickers.size() >= 2) {
                        score |= (kickers[1] << PokerConstants::THIRD_CARD_SHIFT);
                    }
                }
                else if (pairCount == 2) {
                    // Two Pair
                    // [Higher pair -> HIGH_CARD_SHIFT, lower pair -> SECOND_CARD_SHIFT, kicker -> THIRD_CARD_SHIFT]
                    score = (static_cast<uint32_t>(HandRank::TWO_PAIR) << PokerConstants::RANK_SHIFT)
                            | (pairRanks[0] << PokerConstants::HIGH_CARD_SHIFT)
                            | (pairRanks[1] << PokerConstants::SECOND_CARD_SHIFT);
                    if (!kickers.empty()) {
                        score |= (kickers[0] << PokerConstants::THIRD_CARD_SHIFT);
                    }
                }
                else if (pairCount == 1) {
                    // One Pair
                    // [Pair rank -> HIGH_CARD_SHIFT, then 3 kickers in descending order]
                    score = (static_cast<uint32_t>(HandRank::ONE_PAIR) << PokerConstants::RANK_SHIFT)
                            | (pairRanks[0] << PokerConstants::HIGH_CARD_SHIFT);
                    if (kickers.size() >= 1) {
                        score |= (kickers[0] << PokerConstants::SECOND_CARD_SHIFT);
                    }
                    if (kickers.size() >= 2) {
                        score |= (kickers[1] << PokerConstants::THIRD_CARD_SHIFT);
                    }
                    if (kickers.size() >= 3) {
                        score |= (kickers[2] << PokerConstants::FOURTH_CARD_SHIFT);
                    }
                }
                else {
                    // High Card
                    // [5 cards in descending order]
                    score = (static_cast<uint32_t>(HandRank::HIGH_CARD) << PokerConstants::RANK_SHIFT)
                            | (ranks[0] << PokerConstants::HIGH_CARD_SHIFT)
                            | (ranks[1] << PokerConstants::SECOND_CARD_SHIFT)
                            | (ranks[2] << PokerConstants::THIRD_CARD_SHIFT)
                            | (ranks[3] << PokerConstants::FOURTH_CARD_SHIFT)
                            | (ranks[4]);
                }
            }
            
            if (score > bestScore) {
                bestScore = score;
            }
        }
    }
    
    return bestScore;
}


std::string PokerWorker::GetHandDescription(uint32_t score) {
    HandRank rankEnum = static_cast<HandRank>(score >> PokerConstants::RANK_SHIFT);

    uint32_t kickers = score & ((1U << PokerConstants::RANK_SHIFT) - 1);

    auto extractRank = [kickers](uint32_t shift) -> int {
        return (kickers >> shift) & 0xF;
    };

    auto rankToChar = [](int r) -> std::string {
        static const std::array<std::string, 15> symbols = {
            "?",  // 0 - unused
            "A",  // 1 - Ace (low)
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "T",  // 10
            "J",  // 11
            "Q",  // 12
            "K",  // 13
            "A"   // 14 - Ace (high)
        };
        return (r >= 1 && r <= 14) ? symbols[r] : "?";
    };

    std::stringstream description;

    switch (rankEnum) {
        case HandRank::STRAIGHT_FLUSH: {
            int highCard = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            if (highCard == 14) {
                description << "Royal Flush";
            } else {
                description << rankToChar(highCard) << "-high Straight Flush";
            }
            break;
        }
        case HandRank::FOUR_OF_A_KIND: {
            // The 4-of-a-kind rank sits in HIGH_CARD_SHIFT,
            // The single kicker is in SECOND_CARD_SHIFT.
            int quadRank   = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int kickerRank = extractRank(PokerConstants::SECOND_CARD_SHIFT);

            description << "Four of a Kind, " << rankToChar(quadRank) << "s"
                        << " (Kicker: " << rankToChar(kickerRank) << ")";
            break;
        }
        case HandRank::FULL_HOUSE: {
            // The triple rank is in HIGH_CARD_SHIFT, and the pair rank in SECOND_CARD_SHIFT.
            int threeR = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int pairR  = extractRank(PokerConstants::SECOND_CARD_SHIFT);

            description << "Full House, " << rankToChar(threeR) << "s full of "
                        << rankToChar(pairR) << "s";
            break;
        }
        case HandRank::FLUSH: {
            // For a flush, we store all five card ranks in descending order:
            // ranks[0] => HIGH_CARD_SHIFT, ranks[1] => SECOND_CARD_SHIFT, etc.
            int r1 = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int r2 = extractRank(PokerConstants::SECOND_CARD_SHIFT);
            int r3 = extractRank(PokerConstants::THIRD_CARD_SHIFT);
            int r4 = extractRank(PokerConstants::FOURTH_CARD_SHIFT);
            int r5 = extractRank(0); // The last 4 bits

            description << "Flush, " << rankToChar(r1) << "-high "
                        << "(" << rankToChar(r1) << "," << rankToChar(r2) << "," 
                        << rankToChar(r3) << "," << rankToChar(r4) << ","
                        << rankToChar(r5) << ")";
            break;
        }
        case HandRank::STRAIGHT: {
            // For a straight, we just store the top card in HIGH_CARD_SHIFT.
            int highCard = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            description << rankToChar(highCard) << "-high Straight";
            break;
        }
        case HandRank::THREE_OF_A_KIND: {
            // The trips are in HIGH_CARD_SHIFT, then next 2 kickers in SECOND_ and THIRD_.
            int threeR = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int k1     = extractRank(PokerConstants::SECOND_CARD_SHIFT);
            int k2     = extractRank(PokerConstants::THIRD_CARD_SHIFT);

            description << "Three of a Kind, " << rankToChar(threeR) << "s"
                        << " (Kickers: " << rankToChar(k1) << ", " << rankToChar(k2) << ")";
            break;
        }
        case HandRank::TWO_PAIR: {
            // The higher pair goes in HIGH_CARD_SHIFT, the lower pair in SECOND_CARD_SHIFT,
            // and the single kicker in THIRD_CARD_SHIFT.
            int pair1 = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int pair2 = extractRank(PokerConstants::SECOND_CARD_SHIFT);
            int k     = extractRank(PokerConstants::THIRD_CARD_SHIFT);

            description << "Two Pair, " << rankToChar(pair1) << "s and "
                        << rankToChar(pair2) << "s (Kicker: " << rankToChar(k) << ")";
            break;
        }
        case HandRank::ONE_PAIR: {
            // The pair rank is in HIGH_CARD_SHIFT,
            // and three kickers are in SECOND_, THIRD_, and FOURTH_CARD_SHIFT.
            int p  = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int k1 = extractRank(PokerConstants::SECOND_CARD_SHIFT);
            int k2 = extractRank(PokerConstants::THIRD_CARD_SHIFT);
            int k3 = extractRank(PokerConstants::FOURTH_CARD_SHIFT);

            description << "One Pair, " << rankToChar(p) << "s"
                        << " (Kickers: " << rankToChar(k1) << ", "
                        << rankToChar(k2) << ", " << rankToChar(k3) << ")";
            break;
        }
        case HandRank::HIGH_CARD: {
            // Same pattern as Flush: five ranks in descending order
            int r1 = extractRank(PokerConstants::HIGH_CARD_SHIFT);
            int r2 = extractRank(PokerConstants::SECOND_CARD_SHIFT);
            int r3 = extractRank(PokerConstants::THIRD_CARD_SHIFT);
            int r4 = extractRank(PokerConstants::FOURTH_CARD_SHIFT);
            int r5 = extractRank(0);

            description << "High Card " << rankToChar(r1)
                        << " (" << rankToChar(r1) << "," << rankToChar(r2) << ","
                        << rankToChar(r3) << "," << rankToChar(r4) << ","
                        << rankToChar(r5) << ")";
            break;
        }
        default:
            description << "Unknown Hand Rank";
            break;
    }

    return description.str();
}
