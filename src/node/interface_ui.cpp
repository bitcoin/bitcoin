// Copyright (c) 2010-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/interface_ui.h>

#include <util/btcsignals.h>
#include <util/string.h>
#include <util/translation.h>

using util::MakeUnorderedList;

CClientUIInterface uiInterface;

bool InitError(const bilingual_str& str)
{
    uiInterface.ThreadSafeMessageBox(str, CClientUIInterface::MSG_ERROR);
    return false;
}

bool InitError(const bilingual_str& str, const std::vector<std::string>& details)
{
    // For now just flatten the list of error details into a string to pass to
    // the base InitError overload. In the future, if more init code provides
    // error details, the details could be passed separately from the main
    // message for rich display in the GUI. But currently the only init
    // functions which provide error details are ones that run during early init
    // before the GUI uiInterface is registered, so there's no point passing
    // main messages and details separately to uiInterface yet.
    return InitError(details.empty() ? str : str + Untranslated(strprintf(":\n%s", MakeUnorderedList(details))));
}

void InitWarning(const bilingual_str& str)
{
    uiInterface.ThreadSafeMessageBox(str, CClientUIInterface::MSG_WARNING);
}
