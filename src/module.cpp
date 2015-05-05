// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "module.h"

Module::ModuleSignals Module::Signals;
std::vector<CModuleInterface *> Module::vLoadedModules;

void Module::AddModule(CModuleInterface *module)
{
    module->registerModule();
    vLoadedModules.push_back(module);
}
    
void Module::RemoveAllModules()
{
    BOOST_FOREACH(CModuleInterface* module, vLoadedModules)
    {
        module->unregisterModule();
    }
    
    vLoadedModules.clear();
}