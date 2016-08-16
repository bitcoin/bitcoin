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

#ifndef ADMINRPCBINDING_H
#define ADMINRPCBINDING_H

#include <string>
#include <functional>

namespace Streaming {
    class MessageBuilder;
}
class UniValue;
class Message;

namespace AdminRPCBinding
{
    class Parser {
    public:
        Parser(const std::string &method, int answerMessageId, int reserve = -1);
        virtual ~Parser() {}

        inline int messageSize(const UniValue &result) const {
            if (m_reserve > 0)
                return m_reserve;
            return messageSizeCalc(result);
        }
        inline const std::string &method() const {
            return m_method;
        }
        int answerMessageId() const {
            return m_answerMessageId;
        }

        virtual void parser(Streaming::MessageBuilder &builder, const UniValue &result);
        virtual void createRequest(const Message &message, UniValue &output);
        virtual int messageSizeCalc(const UniValue&) const;

    protected:
        int m_reserve;
        int m_answerMessageId;
        std::string m_method;
    };

    Parser* createParser(const Message &message);
}

#endif
