// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once

#include "web3client/common.h"
#include "web3client/common/thread_pool.h"
#include "web3client/config.h"
#include "web3client/connector.h"
#include "web3client/message.h"
#include "web3client/session.h"
namespace jsonrpc::ws
{
using MsgHandler = std::function<void(std::shared_ptr<WsMessage>, std::shared_ptr<WsSession>)>;
using ConnectHandler = std::function<void(std::shared_ptr<WsSession>)>;
using DisconnectHandler = std::function<void(std::shared_ptr<WsSession>)>;
using HandshakeHandler = std::function<void(Error::Ptr, std::shared_ptr<WsMessage>, std::shared_ptr<WsSession>)>;

class WsService : public std::enable_shared_from_this<WsService>
{
 public:
    using Ptr = std::shared_ptr<WsService>;
    WsService(std::shared_ptr<const WsConfig> _config);
    virtual ~WsService();
    virtual void start();
    virtual void stop();
    virtual void reconnect();
    virtual void heartbeat();
    void connect_to_service(EndPointConstPtr _endpoint);

    std::shared_ptr<std::promise<std::pair<boost::beast::error_code, std::string>>> async_connect_service(
        EndPointConstPtr _endpoint);
    WsSession::Ptr new_session(WsStreamDelegate::Ptr _streamDelegate);
    void async_send_message(WsMessage::Ptr _message, Options _options, RespCallback _respFunc);

 protected:
    WsMessage::Ptr build_message()
    {
        return m_messageFactory->buildRequest();
    }

    void set_messageFactory(WsMessageFactory::Ptr _messageFactory)
    {
        m_messageFactory = _messageFactory;
    }

    WsMessageFactory::Ptr messageFactory()
    {
        return m_messageFactory;
    }
    WsMessageFactory::Ptr m_messageFactory;

    virtual void on_connect(Error::Ptr _error, WsSession::Ptr _session);
    virtual void on_disconnect(Error::Ptr _error, WsSession::Ptr _session);
    int32_t m_waitConnectFinishTimeout = 30000;

 private:
    void start_ioc_thread();
    void stop_ioc_thread();
    void add_session(WsSession::Ptr _session);

    void remove_session();

    bool m_running{false};

    size_t m_iocThreadCount;
    WsConfig::ConstPtr m_config;
    std::shared_ptr<boost::asio::io_context> m_ioc;

    WsConnector::Ptr m_connector;
    ThreadPool::Ptr m_threadPool;
    std::shared_ptr<std::vector<std::thread>> m_iocThreads;
    std::shared_ptr<boost::asio::deadline_timer> m_reconnect;
    std::shared_ptr<boost::asio::deadline_timer> m_heartbeat;

    mutable boost::shared_mutex x_mutex;
    WsSession::Ptr m_session;
    ConnectHandler m_connectHandler;
    DisconnectHandler m_disconnectHandler;
    HandshakeHandler m_handshakeHandler;
};
} // namespace jsonrpc::ws
