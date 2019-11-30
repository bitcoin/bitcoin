#ifndef INTEGRATION_REFERENCE_BTC_POP_SERVICE_EXCEPTION_H
#define INTEGRATION_REFERENCE_BTC_POP_SERVICE_EXCEPTION_H

#include <exception>
#include <grpc++/impl/codegen/status.h>

namespace VeriBlock {

class PopServiceException : public std::exception
{
public:
    explicit PopServiceException(const char* message) : _what(message)
    {
    }

    explicit PopServiceException(grpc::Status& status) : _what(status.error_message().c_str()) {}

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
