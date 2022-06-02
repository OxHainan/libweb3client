// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/common.h"
#include "web3client/common/thread_pool.h"
#include "web3client/config.h"
#include "web3client/message.h"
#include "web3client/stream.h"

#include <atomic>
#include <unordered_map>

namespace jsonrpc::ws
{
class WsSession : public std::enable_shared_from_this<WsSession>
{
 public:
    using Ptr = std::shared_ptr<WsSession>;

    WsSession(
        WsConfig::ConstPtr _config,
        std::shared_ptr<boost::asio::io_context> _ioc,
        WsStreamDelegate::Ptr _streamDelegate,
        ThreadPool::Ptr _threadPool,
        WsMessageFactory::Ptr _messageFactory);

    virtual ~WsSession() {}
    void drop(uint32_t reason);
    void start();
    void on_accept(boost::beast::error_code _ec);
    void on_read_packet(boost::beast::flat_buffer& _buffer);
    void async_read();
    void on_read(boost::system::error_code _ec, size_t);
    void on_write_packet();
    void async_write();
    void on_write(std::shared_ptr<bytes> _buffer);
    void async_send_message(std::shared_ptr<WsMessage> _msg, Options _options, RespCallback _respFunc);
    virtual bool is_connected()
    {
        return !m_isDrop && m_streamDelegate && m_streamDelegate->open();
    }
    void on_response_timeout(const boost::system::error_code& _error, const uint64_t& _req);
    std::string endpoint() const
    {
        return m_endpoint;
    }
    void set_connectd_endpoint(const std::string& _endpoint)
    {
        m_endpoint = _endpoint;
    }
    void set_connectHandler(WsConnectHandler _connectHandler)
    {
        m_connectHandler = _connectHandler;
    }

    WsConnectHandler connectHandler()
    {
        return m_connectHandler;
    }

    void set_disconnectHandler(WsDisconnectHandler _disconnectHandler)
    {
        m_disconnectHandler = _disconnectHandler;
    }
    WsDisconnectHandler disconnectHandler()
    {
        return m_disconnectHandler;
    }

 private:
    std::atomic_bool m_isDrop{false};
    boost::beast::flat_buffer m_buffer;
    std::string m_endpoint;
    int32_t m_sendMsgTimeout = -1;
    int32_t m_maxMessageCount = -1;

    WsStreamDelegate::Ptr m_streamDelegate;
    struct CallBack
    {
        using Ptr = std::shared_ptr<CallBack>;
        RespCallback respCallback;
        std::shared_ptr<boost::asio::deadline_timer> timer;
    };
    void add_resp_callback(const uint64_t& _req, CallBack::Ptr _callback);
    CallBack::Ptr response_callback(const uint64_t _req, bool _remove = true);
    boost::shared_mutex x_callBack;
    std::unordered_map<uint16_t, CallBack::Ptr> m_callBacks;

    WsConnectHandler m_connectHandler;
    WsDisconnectHandler m_disconnectHandler;
    WsRecvMessageHandler m_recvMessageHandler;
    std::shared_ptr<boost::asio::io_context> m_ioc;
    std::shared_ptr<ThreadPool> m_threadPool;
    std::shared_ptr<WsMessageFactory> m_messageFactory;

    struct Message
    {
        std::shared_ptr<bytes> buffer;
        std::chrono::time_point<std::chrono::high_resolution_clock> incomeTimePoint;
    };

    // send message queue
    mutable boost::shared_mutex x_queue;
    std::vector<std::shared_ptr<Message>> m_queue;

    // for send performance statistics
    std::atomic<uint32_t> m_msgDelayCount{0};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_msgDelayReportMS =
        std::chrono::high_resolution_clock::now();
};

} // namespace jsonrpc::ws
