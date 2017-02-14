// Copyright (c) 2017 Stephen McCarthy
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "allowed_args.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <set>

namespace AllowedArgs {

static const int screenWidth = 79;
static const int optIndent = 2;
static const int msgIndent = 7;

std::string HelpMessageGroup(const std::string &message)
{
    return std::string(message) + std::string("\n\n");
}

std::string HelpMessageOpt(const std::string &option, const std::string &message)
{
    return std::string(optIndent, ' ') + std::string(option) +
           std::string("\n") + std::string(msgIndent, ' ') +
           FormatParagraph(message, screenWidth - msgIndent, msgIndent) +
           std::string("\n\n");
}

AllowedArgs& AllowedArgs::addHeader(const std::string& strHeader, bool debug)
{

    m_helpList.push_back(HelpComponent{strHeader + "\n\n", debug});
    return *this;
}

AllowedArgs& AllowedArgs::addDebugArg(const std::string& strArgsDefinition, CheckValueFunc checkValueFunc, const std::string& strHelp)
{
    return addArg(strArgsDefinition, checkValueFunc, strHelp, true);
}

AllowedArgs& AllowedArgs::addArg(const std::string& strArgsDefinition, CheckValueFunc checkValueFunc, const std::string& strHelp, bool debug)
{
    std::string strArgs = strArgsDefinition;
    std::string strExampleValue;
    size_t is_index = strArgsDefinition.find('=');
    if (is_index != std::string::npos) {
        strExampleValue = strArgsDefinition.substr(is_index + 1);
        strArgs = strArgsDefinition.substr(0, is_index);
    }

    std::stringstream streamArgs(strArgs);
    std::string strArg;
    bool firstArg = true;
    while (std::getline(streamArgs, strArg, ',')) {
        m_args[strArg] = checkValueFunc;

        std::string optionText = std::string(optIndent, ' ') + "-" + strArg;
        if (!strExampleValue.empty())
            optionText += "=" + strExampleValue;
        optionText += "\n";
        m_helpList.push_back(HelpComponent{optionText, debug || !firstArg});

        firstArg = false;
    }

    std::string helpText = std::string(msgIndent, ' ') + FormatParagraph(strHelp, screenWidth - msgIndent, msgIndent) + "\n\n";
    m_helpList.push_back(HelpComponent{helpText, debug});

    return *this;
}

void AllowedArgs::checkArg(const std::string& strArg, const std::string& strValue) const
{
    // Actual checking is disabled until all child classes are implemented

    // if (!m_args.count(strArg))
    //     throw std::runtime_error(strprintf(_("unrecognized option '%s'"), strArg));

    // if (!m_args.at(strArg)(strValue))
    //     throw std::runtime_error(strprintf(_("invalid value '%s' for option '%s'"), strValue, strArg));
}

std::string AllowedArgs::helpMessage() const
{
    const bool showDebug = GetBoolArg("-help-debug", false);
    std::string helpMessage;

    for (HelpComponent helpComponent : m_helpList)
        if (showDebug || !helpComponent.debug)
            helpMessage += helpComponent.text;

    return helpMessage;
}

//////////////////////////////////////////////////////////////////////////////
//
// CheckValueFunc functions
//

static const std::set<char> boolChars{'0', '1'};

static bool validateString(const std::string& str, const std::set<char>& validChars)
{
    for (const char& c : str)
        if (!validChars.count(c))
            return false;
    return true;
}

static bool optionalBool(const std::string& str)
{
    if (str.empty())
        return true;
    return validateString(str, boolChars);
}

//////////////////////////////////////////////////////////////////////////////
//
// Argument definitions
//

// When adding new arguments to a category, please keep alphabetical ordering,
// where appropriate. Do not translate _(...) addDebugArg help text: there are
// many technical terms, and only a very small audience, so it would be an
// unnecessary stress to translators.

static void addHelpOptions(AllowedArgs& allowedArgs)
{
    allowedArgs
        .addHeader(_("Help options:"))
        .addArg("?,h,help", optionalBool, _("This help message"))
        .addArg("version", optionalBool, _("Print version and exit"))
        .addArg("help-debug", optionalBool, _("Show all debugging options (usage: --help -help-debug)"))
        ;
}

static void addChainSelectionOptions(AllowedArgs& allowedArgs)
{
    allowedArgs
        .addHeader(_("Chain selection options:"))
        .addArg("testnet-ft", optionalBool, _("Use the flexible-transactions testnet"))
        .addArg("testnet", optionalBool, _("Use the test chain"))
        .addDebugArg("regtest", optionalBool,
            "Enter regression test mode, which uses a special chain in which blocks can be solved instantly. "
            "This is intended for regression testing tools and app development.")
        ;
}

BitcoinCli::BitcoinCli()
{
}

Bitcoind::Bitcoind()
{
}

BitcoinQt::BitcoinQt()
{
}

BitcoinTx::BitcoinTx()
{
    addHelpOptions(*this);
    addChainSelectionOptions(*this);

    addHeader(_("Transaction options:"))
        .addArg("create", optionalBool, _("Create new, empty TX."))
        .addArg("json", optionalBool, _("Select JSON output"))
        .addArg("txid", optionalBool, _("Output only the hex-encoded transaction id of the resultant transaction."))
        ;
}

ConfigFile::ConfigFile()
{
}

} // namespace AllowedArgs
