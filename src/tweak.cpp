#include "main.h"
#include "net.h"
#include "tweak.h"
#include "rpcserver.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <boost/thread.hpp>

using namespace std;

// http://www.geeksforgeeks.org/wildcard-character-matching/
// The main function that checks if two given strings
// match. The first string may contain wildcard characters
bool match(const char *first, const char* second)
{
    // If we reach at the end of both strings, we are done
    if (*first == '\0' && *second == '\0')
        return true;
 
    // Make sure that the characters after '*' are present
    // in second string. This function assumes that the first
    // string will not contain two consecutive '*'
    if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return false;
 
    // If the first string contains '?', or current characters
    // of both strings match
    if (*first == '?' || *first == *second)
        return match(first+1, second+1);
 
    // If there is *, then there are two possibilities
    // a) We consider current character of second string
    // b) We ignore current character of second string.
    if (*first == '*')
        return match(first+1, second) || match(first, second+1);
    return false;
}

void LoadTweaks()
{
  for (CTweakMap::iterator it = tweaks.begin(); it != tweaks.end(); ++it)
    {
    std::string name("-");
    name.append(it->second->GetName());
    std::string result = GetArg(name.c_str(), "");
    if (result.size())
      {
      it->second->Set(UniValue(result));
      }
    }
}

// RPC Get a particular tweak
UniValue gettweak(const UniValue& params, bool fHelp)
{
  if (fHelp)
    {
        throw runtime_error(
            "get"
            "\nReturns the value of a configuration setting\n"
            "\nArguments: configuration setting name\n"
            "\nResult:\n"
            "  {\n"
            "    \"setting name\" : value of the setting\n"
            "    ...\n"
            "  }\n"
            "\nExamples:\n" +
            HelpExampleCli("get a b", "") + HelpExampleRpc("get a b", ""));
    }

  UniValue ret(UniValue::VOBJ);
  bool help=false;
  for (unsigned int i=0;i<params.size(); i++)
    {
    string name = params[i].get_str();
    if (name == "help") help=true;
    CTweakMap::iterator item = tweaks.find(name);
    if (item != tweaks.end())
      {
      if (help) ret.push_back(Pair(item->second->GetName(),item->second->GetHelp()));
      else ret.push_back(Pair(item->second->GetName(),item->second->Get()));
      }
    else
      {
      for(CTweakMap::iterator item = tweaks.begin(); item != tweaks.end(); ++item)
        {
        if (match(name.c_str(), item->first.c_str()))
          {
          if (help) ret.push_back(Pair(item->second->GetName(),item->second->GetHelp()));
          else ret.push_back(Pair(item->second->GetName(),item->second->Get()));
          }
        }
      }
    }
  return ret;
}
// RPC Set a particular tweak
UniValue settweak(const UniValue& params, bool fHelp)
{
  if (fHelp)
    {
        throw runtime_error(
            "set"
            "\nSets the value of a configuration option.  Parameters must be of the format name=value, with no spaces (use name=\"the value\" for strings)\n"
            "\nArguments: <configuration setting name>=<value> <configuration setting name2>=<value2>...\n"
            "\nResult:\n"
            "nothing or error string\n"
            "\nExamples:\n" +
            HelpExampleCli("set a 5", "") + HelpExampleRpc("get a b", ""));
    }

  std::string result;
  // First validate all the parameters that are being set
  for (unsigned int i=0;i<params.size(); i++)
    {
    string s = params[i].get_str();
    size_t split = s.find("=");
    if (split == std::string::npos)
      {
      throw runtime_error("Invalid assignment format, missing =");
      }
    std::string name = s.substr(0,split);
    std::string value = s.substr(split+1);
    
    CTweakMap::iterator item = tweaks.find(name);
    if (item != tweaks.end())
      {
	std::string ret = item->second->Validate(value);
        if (!ret.empty())
          {
          result.append(ret);
          result.append("\n");
	  }
      }
    }
  if (!result.empty())  // If there were any validation failures, give up
    {
    throw runtime_error(result);
    }

  // Now assign
  UniValue ret(UniValue::VARR);
  for (unsigned int i=0;i<params.size(); i++)
    {
    string s = params[i].get_str();
    size_t split = s.find("=");
    if (split == std::string::npos)
      {
      throw runtime_error("Invalid assignment format, missing =");
      }
    std::string name = s.substr(0,split);
    std::string value = s.substr(split+1);
    
    CTweakMap::iterator item = tweaks.find(name);
    if (item != tweaks.end())
      {
	UniValue tmp = item->second->Set(value);
        if (!tmp.isNull())
          {
	    ret.push_back(tmp);
	  }
      }
    }

  if (!ret.empty())
    {
      return ret;
    }
  return NullUniValue;
}
