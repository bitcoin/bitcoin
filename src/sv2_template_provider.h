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

/**
 * The first message sent by the client to the server to establish a connection
 * and specifies the subprotocol (Template Provider).
 */
class SetupConnection : Sv2Msg
{
public:
    /**
     * Specifies the subprotocol for the new conncetion. It will always be TemplateDistribution
     * (0x02).
     */
    uint8_t m_protocol;

    /**
     * The minimum protocol version the client supports (currently must be 2).
     */
    uint16_t m_min_version;

    /**
     * The maximum protocol version the client supports (currently must be 2).
     */
    uint16_t m_max_version;

    /**
     * Flags indicating optional protocol features the client supports. Each protocol 
     * from protocol field has its own values/flags.
     */
    uint32_t m_flags;

    /**
     * ASCII text indicating the hostname or IP address.
     */
    std::string m_endpoint_host;

    /**
     * Connecting port value.
     */
    uint16_t m_endpoint_port;

    /**
     * Vendor name of the connecting device.
     */
    std::string m_vendor;

    /**
     * Hardware version of the connecting device.
     */
    std::string m_hardware_version;

    /**
     * Firmware of the connecting device.
     */
    std::string m_firmware;

    /**
     * Unique identifier of the device as defined by the vendor.
     */
    std::string m_device_id;

    template <typename Stream>
    void Unserialize(Stream& s) {
        s >> m_protocol
          >> m_min_version
          >> m_max_version
          >> m_flags;

        ReadSTR0_255(s, m_endpoint_host);
        s >> m_endpoint_port;
        ReadSTR0_255(s, m_vendor);
        ReadSTR0_255(s, m_hardware_version);
        ReadSTR0_255(s, m_firmware);
        ReadSTR0_255(s, m_device_id);
    }
};

#endif // SV2_TEMPLATE_PROVIDER_H
