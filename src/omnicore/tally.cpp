#include "omnicore/tally.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"

#include <stdint.h>
#include <map>

/**
 * Creates an empty tally.
 */
CMPTally::CMPTally()
{
    my_it = mp_token.begin();
}

/**
 * Resets the internal iterator.
 *
 * @return Identifier of the first tally element.
 */
uint32_t CMPTally::init()
{
    uint32_t propertyId = 0;
    my_it = mp_token.begin();
    if (my_it != mp_token.end()) {
        propertyId = my_it->first;
    }
    return propertyId;
}

/**
 * Advances the internal iterator.
 *
 * @return Identifier of the tally element before the update.
 */
uint32_t CMPTally::next()
{
    uint32_t ret = 0;
    if (my_it != mp_token.end()) {
        ret = my_it->first;
        ++my_it;
    }
    return ret;
}

/**
 * Checks whether the addition of a + b overflows.
 *
 * @param a  The number
 * @param b  The other number
 * @return True, if a + b overflows
 */
static bool isOverflow(int64_t a, int64_t b)
{
    return (((b > 0) && (a > (std::numeric_limits<int64_t>::max() - b))) ||
            ((b < 0) && (a < (std::numeric_limits<int64_t>::min() - b))));
}

/**
 * Updates the number of tokens for the given tally type.
 *
 * Negative balances are only permitted for pending balances.
 *
 * @param propertyId  The identifier of the tally to update
 * @param amount      The amount to add
 * @param ttype       The tally type
 * @return True, if the update was successful
 */
bool CMPTally::updateMoney(uint32_t propertyId, int64_t amount, TallyType ttype)
{
    if (TALLY_TYPE_COUNT <= ttype || amount == 0) {
        return false;
    }
    bool fUpdated = false;
    int64_t now64 = mp_token[propertyId].balance[ttype];

    if (isOverflow(now64, amount)) {
        PrintToLog("%s(): ERROR: arithmetic overflow [%d + %d]\n", __func__, now64, amount);
        return false;
    }

    if (PENDING != ttype && (now64 + amount) < 0) {
        // NOTE:
        // Negative balances are only permitted for pending balances
    } else {

        now64 += amount;
        mp_token[propertyId].balance[ttype] = now64;

        fUpdated = true;
    }

    return fUpdated;
}

/**
 * Returns the number of tokens for the given tally type.
 *
 * @param propertyId  The identifier of the tally to lookup
 * @param ttype       The tally type
 * @return The balance
 */
int64_t CMPTally::getMoney(uint32_t propertyId, TallyType ttype) const
{
    if (TALLY_TYPE_COUNT <= ttype) {
        return 0;
    }
    int64_t money = 0;
    TokenMap::const_iterator it = mp_token.find(propertyId);

    if (it != mp_token.end()) {
        const BalanceRecord& record = it->second;
        money = record.balance[ttype];
    }

    return money;
}

/**
 * Returns the number of available tokens.
 *
 * The number of available tokens can be negative, if there is a pending
 * balance.
 *
 * @param propertyId  The identifier of the tally to lookup
 * @return The available balance
 */
int64_t CMPTally::getMoneyAvailable(uint32_t propertyId) const
{
    TokenMap::const_iterator it = mp_token.find(propertyId);

    if (it != mp_token.end()) {
        const BalanceRecord& record = it->second;
        if (record.balance[PENDING] < 0) {
            return record.balance[BALANCE] + record.balance[PENDING];
        } else {
            return record.balance[BALANCE];
        }
    }

    return 0;
}

/**
 * Returns the number of reserved tokens.
 *
 * Balances can be reserved by sell offers, pending accepts, or offers
 * on the distributed exchange.
 *
 * @param propertyId  The identifier of the tally to lookup
 * @return The reserved balance
 */
int64_t CMPTally::getMoneyReserved(uint32_t propertyId) const
{
    int64_t money = 0;
    TokenMap::const_iterator it = mp_token.find(propertyId);

    if (it != mp_token.end()) {
        const BalanceRecord& record = it->second;
        money += record.balance[SELLOFFER_RESERVE];
        money += record.balance[ACCEPT_RESERVE];
        money += record.balance[METADEX_RESERVE];
    }

    return money;
}

/**
 * Compares the tally with another tally and returns true, if they are equal.
 *
 * @param rhs  The other tally
 * @return True, if both tallies are equal
 */
bool CMPTally::operator==(const CMPTally& rhs) const
{
    if (mp_token.size() != rhs.mp_token.size()) {
        return false;
    }
    TokenMap::const_iterator pc1 = mp_token.begin();
    TokenMap::const_iterator pc2 = rhs.mp_token.begin();

    for (unsigned int i = 0; i < mp_token.size(); ++i) {
        if (pc1->first != pc2->first) {
            return false;
        }
        const BalanceRecord& record1 = pc1->second;
        const BalanceRecord& record2 = pc2->second;

        for (int ttype = 0; ttype < TALLY_TYPE_COUNT; ++ttype) {
            if (record1.balance[ttype] != record2.balance[ttype]) {
                return false;
            }
        }
        ++pc1;
        ++pc2;
    }

    assert(pc1 == mp_token.end());
    assert(pc2 == rhs.mp_token.end());

    return true;
}

/**
 * Compares the tally with another tally and returns true, if they are not equal.
 *
 * @param rhs  The other tally
 * @return True, if both tallies are not equal
 */
bool CMPTally::operator!=(const CMPTally& rhs) const
{
    return !operator==(rhs);
}

/**
 * Prints a balance record to the console.
 *
 * @param propertyId  The identifier of the tally to print
 * @param bDivisible  Whether the token is divisible or indivisible
 * @return The total number of tokens of the tally
 */
int64_t CMPTally::print(uint32_t propertyId, bool bDivisible) const
{
    int64_t balance = 0;
    int64_t selloffer_reserve = 0;
    int64_t accept_reserve = 0;
    int64_t pending = 0;
    int64_t metadex_reserve = 0;

    TokenMap::const_iterator it = mp_token.find(propertyId);

    if (it != mp_token.end()) {
        const BalanceRecord& record = it->second;
        balance = record.balance[BALANCE];
        selloffer_reserve = record.balance[SELLOFFER_RESERVE];
        accept_reserve = record.balance[ACCEPT_RESERVE];
        pending = record.balance[PENDING];
        metadex_reserve = record.balance[METADEX_RESERVE];
    }

    if (bDivisible) {
        PrintToConsole("%22s [ SO_RESERVE= %22s, ACCEPT_RESERVE= %22s, METADEX_RESERVE= %22s ] %22s\n",
                FormatDivisibleMP(balance, true), FormatDivisibleMP(selloffer_reserve, true),
                FormatDivisibleMP(accept_reserve, true), FormatDivisibleMP(metadex_reserve, true),
                FormatDivisibleMP(pending, true));
    } else {
        PrintToConsole("%14d [ SO_RESERVE= %14d, ACCEPT_RESERVE= %14d, METADEX_RESERVE= %14d ] %14d\n",
                balance, selloffer_reserve, accept_reserve, metadex_reserve, pending);
    }

    return (balance + selloffer_reserve + accept_reserve + metadex_reserve);
}
