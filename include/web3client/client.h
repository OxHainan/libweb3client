// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.

#pragma once
#include "iostream"
#include "web3client/common/error.h"
#include "web3client/config.h"
#include "web3client/rpc/jsonrpcImpl.h"
#include "web3client/rpc/message.h"
#include "web3client/service.h"
#include "web3client/session.h"
namespace jsonrpc::ws
{
using WsHandshakeSucHandler = std::function<void(WsSession::Ptr)>;
class Client : public WsService
{
 public:
    ~Client() {}

    static std::shared_ptr<Client> get_instance(WsConfig::ConstPtr _config = nullptr);

    void init_jsonrpc();

    JsonRpcImpl::Ptr jsonrpc();

    virtual void start() override;
    virtual void stop() override;

 private:
    Client() = delete;
    Client(WsConfig::ConstPtr _config);

    virtual void on_connect(Error::Ptr _error, WsSession::Ptr _session) override;
    virtual void on_disconnect(Error::Ptr _error, WsSession::Ptr _session) override;

    void wait_for_connectionEstablish();
    void start_handshake(WsSession::Ptr _session);
    bool check_handshake(WsSession::Ptr _session);
    uint32_t handshakeSucCount() const
    {
        return m_handshakeSucCount.load();
    }
    void increase_handshakeSucCount()
    {
        m_handshakeSucCount++;
    }
    void call_handshakeSucHandler(WsSession::Ptr _session)
    {
        if (m_handshakeSucHandler)
            m_handshakeSucHandler(_session);
    }

    uint32_t m_handshakeTimeout = 10000;
    std::atomic<uint32_t> m_handshakeSucCount{0};
    WsHandshakeSucHandler m_handshakeSucHandler;
    JsonRpcImpl::Ptr m_jsonrpc;
};
} // namespace jsonrpc::ws
