// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/stream.h"

#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <functional>
#include <mutex>
#include <set>

namespace jsonrpc::ws
{
class WsConnector : public std::enable_shared_from_this<WsConnector>
{
 public:
    using Ptr = std::shared_ptr<WsConnector>;
    using ConstPtr = std::shared_ptr<const WsConnector>;

    WsConnector(std::shared_ptr<boost::asio::io_context> _ioc)
    {
        m_ioc = _ioc;
        m_resolver = std::make_shared<boost::asio::ip::tcp::resolver>(boost::asio::make_strand(*_ioc));
        m_builder = std::make_shared<WsStreamDelegateBuilder>();
    }

    void connect(
        const std::string& _host,
        uint16_t _port,
        std::function<void(boost::beast::error_code, const std::string& extErrorMsg, WsStreamDelegate::Ptr)> _callback);

    bool erase_pending_conns(const std::string& _endpoint)
    {
        std::lock_guard<std::mutex> lock(x_pendingConns);
        return m_pendingConns.erase(_endpoint);
    }

    bool insert_pending_conns(const std::string& _endpoint)
    {
        std::lock_guard<std::mutex> lock(x_pendingConns);
        auto p = m_pendingConns.insert(_endpoint);
        return p.second;
    }

 private:
    std::shared_ptr<WsStreamDelegateBuilder> m_builder;
    std::shared_ptr<boost::asio::ip::tcp::resolver> m_resolver;
    std::shared_ptr<boost::asio::io_context> m_ioc;

    mutable std::mutex x_pendingConns;
    std::set<std::string> m_pendingConns;
};
} // namespace jsonrpc::ws
