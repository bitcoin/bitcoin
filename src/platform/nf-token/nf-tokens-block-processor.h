#ifndef CROWN_PLATFORM_NF_TOKENS_PROCESSOR_H
#define CROWN_PLATFORM_NF_TOKENS_PROCESSOR_H

#include "nf-tokens-manager.h"

class CBlock;
class CBlockIndex;
class CValidationState;

namespace Platform
{
    class NfTokensBlockProcessor
    {
    public:
        bool ProcessBlock(const CBlock & block, const CBlockIndex * pindex, CValidationState & state)
        {

        }

        bool UndoBlock(const CBlock & block, const CBlockIndex * pindex, CValidationState & state)
        {

        }
    };
}

#endif // CROWN_PLATFORM_NF_TOKENS_PROCESSOR_H
