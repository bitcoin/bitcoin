#ifndef BITCOIN_QT_OMNICORE_INIT_H
#define BITCOIN_QT_OMNICORE_INIT_H

namespace OmniCore
{
    //! Shows an user dialog with general warnings and potential risks
    bool AskUserToAcknowledgeRisks();

    //! Setup and initialization related to Omni Core Qt
    bool Initialize();
}

#endif // BITCOIN_QT_OMNICORE_INIT_H
