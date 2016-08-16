/*
 * This file is part of the bitcoin-classic project
 * Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef NETWORKENUMS_H
#define NETWORKENUMS_H

namespace Network {
enum HeaderTags {
    HeaderEnd = 0,
    ServiceId = 1,
    MessageId = 2,
    SequenceStart = 3,  ///< An int-value of the total body-size.
    LastInSequence = 4,  ///< bool. If present indicates its part of a sequence. If true, last in sequence.
    Ping = 5,
    Pong = 6,
};

enum ServiceTypes {
    /// This service-id is 'internal' for NetworkManager.
    SystemServiceId = 126,
};
}

#endif
