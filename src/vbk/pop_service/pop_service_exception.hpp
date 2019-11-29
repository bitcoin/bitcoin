#ifndef INTEGRATION_REFERENCE_BTC_POP_SERVICE_EXCEPTION_H
#define INTEGRATION_REFERENCE_BTC_POP_SERVICE_EXCEPTION_H

#include <exception>

namespace VeriBlock {

class PopServiceException : public std::exception
{
public:
    explicit PopServiceException(const char* message) : _what(message)
    {
    }

    const char*
    what() const noexcept override
    {
        return this->_what;
    }

private:
    const char* _what;
};

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_POP_SERVICE_EXCEPTION_H
