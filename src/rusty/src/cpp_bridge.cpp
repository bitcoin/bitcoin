#include <chainparams.h>
#include <validation.h>
#include <shutdown.h>
#include <serialize.h>
#include <consensus/validation.h>

/** A class that deserializes a single thing one time. */
class InputStream
{
public:
    InputStream(int nTypeIn, int nVersionIn, const unsigned char *data, size_t datalen) :
    m_type(nTypeIn),
    m_version(nVersionIn),
    m_data(data),
    m_remaining(datalen)
    {}

    void read(char* pch, size_t nSize)
    {
        if (nSize > m_remaining)
            throw std::ios_base::failure(std::string(__func__) + ": end of data");

        if (pch == nullptr)
            throw std::ios_base::failure(std::string(__func__) + ": bad destination buffer");

        if (m_data == nullptr)
            throw std::ios_base::failure(std::string(__func__) + ": bad source buffer");

        memcpy(pch, m_data, nSize);
        m_remaining -= nSize;
        m_data += nSize;
    }

    template<typename T>
    InputStream& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }

    int GetVersion() const { return m_version; }
    int GetType() const { return m_type; }
private:
    const int m_type;
    const int m_version;
    const unsigned char* m_data;
    size_t m_remaining;
};

extern "C" {

bool rusty_IsInitialBlockDownload() {
    return ::ChainstateActive().IsInitialBlockDownload();
}

bool rusty_ShutdownRequested() {
    return ShutdownRequested();
}

const void* rusty_ConnectHeaders(const uint8_t* headers_data, size_t stride, size_t count) {
    std::vector<CBlockHeader> headers;
    for(size_t i = 0; i < count; i++) {
        CBlockHeader header;
        try {
            InputStream(SER_NETWORK, PROTOCOL_VERSION, headers_data + (stride * i), 80) >> header;
        } catch (...) {}
        headers.push_back(header);
    }
    BlockValidationState state_dummy;
    const CBlockIndex* last_index = nullptr;
    ProcessNewBlockHeaders(headers, state_dummy, ::Params(), &last_index);
    return last_index;
}

const void* rusty_GetChainTip() {
    LOCK(cs_main);
    const CBlockIndex* tip = ::ChainActive().Tip();
    assert(tip != nullptr);
    return tip;
}

int32_t rusty_IndexToHeight(const void* pindexvoid) {
    const CBlockIndex *pindex = (const CBlockIndex*) pindexvoid;
    assert(pindex != nullptr);
    return pindex->nHeight;
}

}
