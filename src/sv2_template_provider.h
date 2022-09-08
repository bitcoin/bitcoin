#ifndef SV2_TEMPLATE_PROVIDER_H
#define SV2_TEMPLATE_PROVIDER_H

#include <streams.h>

/**
 * Base class for all stratum v2 messages.
 */
class Sv2Msg
{
public:
    void ReadSTR0_255(CDataStream& stream, std::string& output)
    {
        uint8_t len;
        stream >> len;

        for (auto i = 0; i < len; ++i) {
            uint8_t b;
            stream >> b;

            output.push_back(b);
        }
    }
};

#endif // SV2_TEMPLATE_PROVIDER_H
