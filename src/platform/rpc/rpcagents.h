#ifndef CROWN_PLATFORM_RPCAGENTS_H
#define CROWN_PLATFORM_RPCAGENTS_H

#include "json/json_spirit_value.h"

json_spirit::Value agents(const json_spirit::Array& params, bool fHelp);

namespace detail
{
    json_spirit::Value vote(const json_spirit::Array& params, bool fHelp);
    json_spirit::Value list(const json_spirit::Array& params, bool fHelp);
    json_spirit::Value listcandidates(const json_spirit::Array& params, bool fHelp);
}

#endif //CROWN_PLATFORM_RPCAGENTS_H
