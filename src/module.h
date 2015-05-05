// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MODULE_H
#define BITCOIN_MODULE_H

#include <vector>

#include <boost/foreach.hpp>
#include <boost/signals2/signal.hpp>

class CModuleInterface
{
public:
    virtual ~CModuleInterface() {} ;
    
    virtual void registerModule() {};
    virtual void unregisterModule() {};
};

class Module {
public:

    enum SignalStage
    {
        SignalStageStarted,
        SignalStageStage0,
        SignalStageFinished
    };
    
    struct ModuleSignals {
        boost::signals2::signal<void (std::string&)> AppendHelpMessage; //!append modules help to the global help string
        boost::signals2::signal<void ()> ParameterParsed; //!signal after the parameters have been parsed
        boost::signals2::signal<void ()> ConfigRead; //!signal after the config was read completely
        boost::signals2::signal<void ()> ValidateArguments; //!possibility to validate arguments
        
        //!signal for parse startup arguments and possibility to stop the excution by adding a string to the error string
        boost::signals2::signal<void (std::string&,std::string&)> ParseArguments;
        boost::signals2::signal<void (SignalStage)> DebugStartupInfo; //! signal for pushing debug intos to the log
        boost::signals2::signal<void ()> NodeStarted; //! signal after the node has started
        boost::signals2::signal<void (SignalStage,std::string&,std::string&)> ModuleInit; //! signal for the init process
        
        boost::signals2::signal<void (SignalStage)> Shutdown; //! signal for the shutdown process
    };
    
    static ModuleSignals Signals;

    static std::vector<CModuleInterface *> vLoadedModules;

    static void AddModule(CModuleInterface *module);
    static void RemoveAllModules();
};
#endif // BITCOIN_MODULE_H