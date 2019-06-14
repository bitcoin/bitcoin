#ifndef OMNICORE_QT_INIT_H
#define OMNICORE_QT_INIT_H

namespace OmniCore
{
    //! Shows an user dialog with general warnings and potential risks
    bool AskUserToAcknowledgeRisks();

    //! Setup and initialization related to Omni Core Qt
    bool Initialize();
}

#endif // OMNICORE_QT_INIT_H
