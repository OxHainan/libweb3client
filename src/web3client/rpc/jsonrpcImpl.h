// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "message.h"
#include "web3client/rpc/message.h"
namespace jsonrpc::ws
{
using RespFunc = std::function<void(Error::Ptr, std::shared_ptr<bytes>)>;
using JsonRpcSendHandler = std::function<void(WsMessage::Ptr, RespFunc)>;

class JsonRpcImpl
{
 public:
    using Ptr = std::shared_ptr<JsonRpcImpl>;
    JsonRpcImpl(WsMessageFactory::Ptr _factory) : m_factory(_factory) {}

    void set_sender(JsonRpcSendHandler _sender)
    {
        m_sender = _sender;
    }

    WsMessage::Ptr set_handshake_message()
    {
        auto msg = m_factory->buildRequest();
        auto request = EthSyncing::make_request(msg->req_id());
        msg->set_data(EthSyncing::ReqSerialiser::to_serialised(request));
        return msg;
    }

    template <typename TRpcBuilder>
    void send(const typename TRpcBuilder::Params& params, RespFunc _respFunc = RespFunc())
    {
        auto msg = m_factory->buildRequest();
        auto request = TRpcBuilder::make_request(msg->req_id());
        request.params = params;
        msg->set_data(TRpcBuilder::ReqSerialiser::to_serialised(request));
        m_sender(msg, _respFunc);
    }

    template <typename TRpcBuilder>
    void send(RespFunc _respFunc = RespFunc())
    {
        auto msg = m_factory->buildRequest();
        auto request = TRpcBuilder::make_request(msg->req_id());
        msg->set_data(TRpcBuilder::ReqSerialiser::to_serialised(request));
        m_sender(msg, _respFunc);
    }

 private:
    WsMessageFactory::Ptr m_factory;
    JsonRpcSendHandler m_sender;
};
} // namespace jsonrpc::ws
