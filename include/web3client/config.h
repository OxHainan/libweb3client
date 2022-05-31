// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <vector>

#define MIN_THREAD_POOL_SIZE (1)
#define MIN_HEART_BEAT_PERIOD_MS (10000)
#define MIN_RECONNECT_PERIOD_MS (10000)
#define DEFAULT_MESSAGE_TIMEOUT_MS (-1)
#define DEFAULT_MAX_MESSAGE_SIZE (32 * 1024 * 1024)

namespace jsonrpc::ws
{
struct EndPoint
{
    std::string host;
    uint64_t port;

    std::string endpoint() const
    {
        return host + ":" + std::to_string(port);
    }
};

using EndPointPtr = std::shared_ptr<EndPoint>;
using EndPointConstPtr = std::shared_ptr<const EndPoint>;

class WsConfig
{
 public:
    using Ptr = std::shared_ptr<WsConfig>;
    using ConstPtr = std::shared_ptr<const WsConfig>;

    void set_endpoint(const std::string& host, uint64_t port)
    {
        {
            boost::system::error_code ec;
            boost::asio::ip::address::from_string(host, ec);
            if (ec) {
                throw std::invalid_argument("Invalid connected server, ip: " + host);
            }
        }

        {
            if (port < 1024) {
                throw std::invalid_argument("Invalid connected server, port: " + std::to_string(port));
            }
        }

        m_endpoint = std::make_shared<EndPoint>();
        m_endpoint->host = host;
        m_endpoint->port = port;
    }

    EndPointConstPtr remote_endpoint() const
    {
        return m_endpoint;
    }

    void set_reconnect_period(uint32_t _reconnect_period)
    {
        m_reconnect_period = _reconnect_period;
    }

    uint32_t reconnect_period() const
    {
        return m_reconnect_period > MIN_RECONNECT_PERIOD_MS ? m_reconnect_period : MIN_RECONNECT_PERIOD_MS;
    }

    void set_heartbeat_period(uint32_t _heartbeat_period)
    {
        m_heartbeat_period = _heartbeat_period;
    }

    uint32_t heartbeat_period() const
    {
        return m_heartbeat_period > MIN_HEART_BEAT_PERIOD_MS ? m_heartbeat_period : MIN_HEART_BEAT_PERIOD_MS;
    }

    int32_t send_msg_timeout() const
    {
        return m_send_msg_timeout;
    }

    void set_msg_timeout(int32_t timeout)
    {
        m_send_msg_timeout = timeout;
    }

    uint32_t ioc_thread_count() const
    {
        return m_ioc_thread_count;
    }

    void set_ioc_thread_count(uint32_t count)
    {
        m_ioc_thread_count = count;
    }

    uint32_t thread_pool_count() const
    {
        return m_thread_pool_count ? m_thread_pool_count : MIN_THREAD_POOL_SIZE;
    }

    void set_thread_pool_count(uint32_t count)
    {
        m_thread_pool_count = count;
    }

    void set_max_msg_size(uint32_t size)
    {
        m_max_msg_size = size;
    }

    uint32_t max_msg_size() const
    {
        return m_max_msg_size;
    }

 private:
    EndPointPtr m_endpoint;
    uint32_t m_thread_pool_count{MIN_THREAD_POOL_SIZE};
    uint32_t m_ioc_thread_count{2};
    int32_t m_send_msg_timeout{DEFAULT_MAX_MESSAGE_SIZE};
    uint32_t m_reconnect_period{MIN_RECONNECT_PERIOD_MS};
    uint32_t m_heartbeat_period{MIN_HEART_BEAT_PERIOD_MS};
    uint32_t m_max_msg_size{DEFAULT_MAX_MESSAGE_SIZE};
};
} // namespace jsonrpc::ws
