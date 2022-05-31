// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/exceptions.h"

#include <memory>
namespace jsonrpc::ws
{
class Error : public Exception
{
 public:
    using Ptr = std::shared_ptr<Error>;
    using ConstPtr = std::shared_ptr<const Error>;
    Error() = default;
    Error(int64_t _errorCode, const std::string& _errorMessage) :
      Exception(_errorMessage),
      m_errorCode(_errorCode),
      m_errorMessage(_errorMessage)
    {}

    virtual ~Error() {}
    virtual std::string const& errorMessage() const
    {
        return m_errorMessage;
    }

    virtual int64_t errorCode() const
    {
        return m_errorCode;
    }

    virtual void setErrorCode(int64_t _errorCode)
    {
        m_errorCode = _errorCode;
    }
    virtual void setErrorMessage(std::string _errorMessage)
    {
        m_errorMessage = _errorMessage;
    }

 private:
    int64_t m_errorCode;
    std::string m_errorMessage;
};
}