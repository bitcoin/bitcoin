#include "nf-token-protocol.h"

namespace Platform
{
    std::string NftRegSignToString(NftRegSign nftRegSign)
    {
        switch (nftRegSign)
        {
            case SelfSign:
                return "SelfSign";
            case SignByCreator:
                return "SignByCreator";
            case SignPayer:
                return "SignPayer";
            default:
                return "UnknownRegSign";
        }
    }
}