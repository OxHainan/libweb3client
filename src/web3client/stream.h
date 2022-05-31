// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/common.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/beast/websocket/stream_base.hpp>
#include <boost/system/detail/errc.hpp>
#include <boost/system/detail/error_code.hpp>
#include <functional>
#include <memory>
namespace jsonrpc::ws
{
using WsStreamRWHandler = std::function<void(boost::system::error_code, std::size_t)>;
using WsStreamHandshakeHandler = std::function<void(boost::system::error_code)>;

template <typename STREAM>
class WsStream
{
 public:
    using Ptr = std::shared_ptr<WsStream>;
    using ConstPtr = std::shared_ptr<const WsStream>;

    WsStream(std::shared_ptr<boost::beast::websocket::stream<STREAM>> _stream) : m_stream(_stream)
    {
        boost::beast::websocket::stream_base::timeout opt;
        opt.handshake_timeout = std::chrono::milliseconds(30000);
        opt.idle_timeout = std::chrono::milliseconds(10000);
        opt.keep_alive_pings = true;
        m_stream->set_option(opt);
    }

    virtual ~WsStream()
    {
        close();
    }

    void set_message_max(uint32_t max_size)
    {
        m_stream->read_message_max(max_size);
    }

    bool open()
    {
        return !m_closed.load() && m_stream->is_open();
    }

    boost::beast::tcp_stream& tcp_stream() const
    {
        return boost::beast::get_lowest_layer(*m_stream);
    }

    std::shared_ptr<boost::beast::websocket::stream<STREAM>> stream() const
    {
        return m_stream;
    }

    void async_write(const bytes& _buffer, WsStreamRWHandler _handler)
    {
        m_stream->binary(true);
        m_stream->async_write(boost::asio::buffer(_buffer), _handler);
    }

    void async_read(boost::beast::flat_buffer& _buffer, WsStreamRWHandler _handler)
    {
        m_stream->async_read(_buffer, _handler);
    }

    void init_handshake()
    {
        tcp_stream().expires_never();
        m_stream->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));
        m_stream->set_option(
            boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type& req) {
                req.set(
                    boost::beast::http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
            }));
    }

    void async_handshake(
        const std::string& _endpoint,
        const std::string& _target,
        std::function<void(boost::beast::error_code)> _handler)
    {
        init_handshake();
        m_stream->async_handshake(_endpoint, _target, _handler);
    }

    virtual std::string remote_endpoint()
    {
        try {
            auto& s = tcp_stream();
            auto endpoint = s.socket().remote_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        }
        catch (const std::exception& e) {
            // std::cerr << e.what() << '\n';
        }

        return std::string("");
    }

    void close()
    {
        bool trueValue = true;
        bool falseValue = false;
        if (m_closed.compare_exchange_strong(falseValue, trueValue)) {
            auto& ss = boost::beast::get_lowest_layer(*m_stream);
            try {
                boost::beast::error_code ec;
                auto& socket = ss.socket();
                if (socket.is_open()) {
                    socket.close();
                }
            }
            catch (...) {
            }
        }
    }

 private:
    std::atomic<bool> m_closed{false};
    std::shared_ptr<boost::beast::websocket::stream<STREAM>> m_stream;
};

using RawWsStream = WsStream<boost::beast::tcp_stream>;

class WsStreamDelegate
{
 public:
    using Ptr = std::shared_ptr<WsStreamDelegate>;
    using ConstPtr = std::shared_ptr<const WsStreamDelegate>;

    WsStreamDelegate(RawWsStream::Ptr& _rawStream) : m_rawStream(_rawStream) {}
    void set_message_max(uint32_t max_size)
    {
        m_rawStream->set_message_max(max_size);
    }

    bool open()
    {
        return m_rawStream->open();
    }

    void close()
    {
        m_rawStream->close();
    }

    std::string remote_endpoint()
    {
        return m_rawStream->remote_endpoint();
    }

    void async_write(const bytes& _buffer, WsStreamRWHandler _handler)
    {
        return m_rawStream->async_write(_buffer, _handler);
    }

    void async_read(boost::beast::flat_buffer& _buffer, WsStreamRWHandler _handler)
    {
        return m_rawStream->async_read(_buffer, _handler);
    }

    void async_handshake(
        const std::string& _endpoint,
        const std::string& _target,
        std::function<void(boost::beast::error_code)> _handler)
    {
        return m_rawStream->async_handshake(_endpoint, _target, _handler);
    }

    void async_handshake(std::function<void(boost::beast::error_code)> _handler)
    {
        _handler(make_error_code(boost::system::errc::success));
    }

    boost::beast::tcp_stream& tcp_stream()
    {
        return m_rawStream->tcp_stream();
    }

    auto stream()
    {
        return m_rawStream->stream();
    }

 private:
    RawWsStream::Ptr m_rawStream;
};

class WsStreamDelegateBuilder
{
 public:
    using Ptr = std::shared_ptr<WsStreamDelegateBuilder>;
    using ConstPtr = std::shared_ptr<const WsStreamDelegateBuilder>;

    WsStreamDelegate::Ptr build(std::shared_ptr<boost::beast::tcp_stream> _tcp_stream)
    {
        auto stream =
            std::make_shared<boost::beast::websocket::stream<boost::beast::tcp_stream>>(std::move(*_tcp_stream));
        auto raw_stream = std::make_shared<WsStream<boost::beast::tcp_stream>>(stream);
        return std::make_shared<WsStreamDelegate>(raw_stream);
    }
};
} // namespace jsonrpc::ws
