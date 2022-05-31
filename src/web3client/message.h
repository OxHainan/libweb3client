// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/common.h"

#include <memory>
namespace jsonrpc::ws
{
class WsMessage
{
 public:
    using Ptr = std::shared_ptr<WsMessage>;

    WsMessage() : m_data(std::make_shared<bytes>()) {}

    virtual ~WsMessage() {}

    virtual uint64_t req_id() const
    {
        return m_id;
    }

    virtual std::shared_ptr<bytes> data()
    {
        return m_data;
    }

    virtual void set_data(const bytes& _data)
    {
        m_data = std::make_shared<bytes>(_data);
    }

    virtual void set_id(uint64_t _id)
    {
        m_id = _id;
    }
    bool encode(bytes& _buffer);
    uint64_t decode(const byte* _buffer, size_t _size);

 private:
    uint64_t m_id{0};
    std::shared_ptr<bytes> m_data;
};

class WsMessageFactory
{
 public:
    using Ptr = std::shared_ptr<WsMessageFactory>;

    WsMessageFactory() = default;
    virtual ~WsMessageFactory() {}

    virtual std::shared_ptr<WsMessage> buildMessage()
    {
        return std::make_shared<WsMessage>();
    }

    virtual std::shared_ptr<WsMessage> buildMessage(const bytes& _data)
    {
        auto msg = buildMessage();
        msg->set_data(_data);
        return msg;
    }

    virtual std::shared_ptr<WsMessage> buildRequest() = 0;
};
} // namespace jsonrpc::ws
