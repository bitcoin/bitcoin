// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef RAVEN_QT_ASSETRECORD_H
#define RAVEN_QT_ASSETRECORD_H

#include "math.h"
#include "amount.h"
#include "tinyformat.h"


/** UI model for unspent assets.
 */
class AssetRecord
{
public:

    AssetRecord():
            name(""), quantity(0), units(0)
    {
    }

    AssetRecord(const std::string _name, const CAmount& _quantity, const int _units):
            name(_name), quantity(_quantity), units(_units)
    {
    }

    std::string formattedQuantity() {
        int64_t quotient = quantity / COIN;
        if (units == 0) {
            return strprintf("%d", quotient);
        } else {
            int64_t remainder = quantity % COIN;
            return strprintf("%d.%0" + std::to_string(units) + "d", quotient, remainder);
        }
    }

    /** @name Immutable attributes
      @{*/
    std::string name;
    CAmount quantity;
    int units;
    /**@}*/

};

#endif // RAVEN_QT_ASSETRECORD_H
