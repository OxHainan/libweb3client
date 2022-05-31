// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include <boost/throw_exception.hpp>
#include <exception>
namespace jsonrpc::ws
{
struct Exception : virtual std::exception, virtual boost::exception
{
    Exception(const std::string& _msg = std::string()) : m_message(std::move(_msg)) {}
    const char* what() const noexcept override
    {
        return m_message.empty() ? std::exception::what() : m_message.c_str();
    }

 private:
    std::string m_message;
};
} // namespace jsonrpc::ws
