// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/session.h"

#include "common/error.h"
#include "iostream"
#include "web3client/config.h"
#include "web3client/response.h"
#include "web3client/stream.h"
namespace jsonrpc::ws
{
#define MESSAGE_SEND_DELAY_REPORT_MS (5000)
#define MAX_MESSAGE_SEND_DELAY_MS (5000)
WsSession::WsSession(
    WsConfig::ConstPtr _config,
    std::shared_ptr<boost::asio::io_context> _ioc,
    WsStreamDelegate::Ptr _streamDelegate,
    ThreadPool::Ptr _threadPool,
    WsMessageFactory::Ptr _messageFactory) :
  m_endpoint(_streamDelegate->remote_endpoint()),
  m_sendMsgTimeout(_config->send_msg_timeout()),
  m_maxMessageCount(_config->max_msg_size()),
  m_streamDelegate(_streamDelegate),
  m_ioc(_ioc),
  m_threadPool(_threadPool),
  m_messageFactory(_messageFactory)
{}

void WsSession::drop(uint32_t reason)
{
    if (m_isDrop) {
        return;
    }

    m_isDrop = true;
    auto self = std::weak_ptr<WsSession>(shared_from_this());
    {
        auto error = std::make_shared<Error>(WsError::SessionDisconnect, "the session has been disconnected");
        boost::shared_lock<boost::shared_mutex> lock(x_callBack);
        for (auto& cb_entry : m_callBacks) {
            auto callback = cb_entry.second;
            if (callback->timer) {
                callback->timer->cancel();
            }

            m_threadPool->enqueue([callback, error]() { callback->respCallback(error, nullptr, nullptr); });
        }
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(x_callBack);
        m_callBacks.clear();
    }

    if (m_streamDelegate) {
        m_streamDelegate->close();
    }

    m_threadPool->enqueue([self]() {
        auto session = self.lock();
        if (session) {
            session->disconnectHandler()(nullptr, session);
        }
    });
}

void WsSession::start()
{
    if (m_connectHandler) {
        auto session = shared_from_this();
        m_connectHandler(nullptr, session);
    }

    async_read();
}

void WsSession::on_accept(boost::beast::error_code _ec)
{
    if (_ec) {
        return drop(WsError::AcceptError);
    }

    if (m_connectHandler) {
        m_connectHandler(nullptr, shared_from_this());
    }

    async_read();
}

void WsSession::on_read_packet(boost::beast::flat_buffer& _buffer)
{
    auto data = boost::asio::buffer_cast<byte*>(boost::beast::buffers_front(_buffer.data()));
    auto size = boost::asio::buffer_size(_buffer.data());
    auto message = m_messageFactory->buildMessage();
    if (message->decode(data, size) < 0) {
        return drop(WsError::PacketError);
    }

    auto response = ResponseMessage::make_response({data, data + size});
    _buffer.consume(_buffer.size());

    auto self = std::weak_ptr<WsSession>(shared_from_this());
    auto callback = response_callback(response->id());
    m_threadPool->enqueue([response, self, callback]() {
        auto session = self.lock();
        if (!session) {
            return;
        }
        if (callback) {
            if (callback->timer)
                callback->timer->cancel();

            callback->respCallback(nullptr, response, session);
        } else {
            session->m_recvMessageHandler(response, session);
        }
    });
}

void WsSession::async_read()
{
    if (!is_connected()) {
        return;
    }

    try {
        m_streamDelegate->async_read(
            m_buffer, std::bind(&WsSession::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
    }
    catch (const std::exception& e) {
        drop(WsError::ReadError);
    }
}

void WsSession::on_read(boost::system::error_code _ec, size_t)
{
    if (_ec) {
        return drop(WsError::ReadError);
    }

    on_read_packet(m_buffer);
    async_read();
}

void WsSession::on_write_packet()
{
    std::shared_ptr<Message> msg = nullptr;
    size_t nMsgQueueSize = 0;
    {
        boost::unique_lock<boost::shared_mutex> lock(x_queue);
        msg = m_queue.front();
        m_queue.erase(m_queue.begin());
        nMsgQueueSize = m_queue.size();

        if (!m_queue.empty()) {
            async_write();
        }
    }
#if 1
    auto now = std::chrono::high_resolution_clock::now();
    auto delayMS = std::chrono::duration_cast<std::chrono::milliseconds>(now - msg->incomeTimePoint).count();

    if (delayMS >= MAX_MESSAGE_SEND_DELAY_MS) {
        m_msgDelayCount++;
    }

    auto reportMS = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_msgDelayReportMS).count();
    if (reportMS >= MESSAGE_SEND_DELAY_REPORT_MS) {
        m_msgDelayCount = 0;
        m_msgDelayReportMS = now;
    }
#endif
}

void WsSession::async_write()
{
    if (!is_connected()) {
        return;
    }

    try {
        auto session = shared_from_this();
        m_streamDelegate->async_write(*(m_queue.front()->buffer), [session](boost::beast::error_code _ec, size_t) {
            if (_ec) {
                return session->drop(WsError::WriteError);
            }

            session->on_write_packet();
        });
    }
    catch (const std::exception& e) {
        drop(WsError::WriteError);
    }
}

void WsSession::on_write(std::shared_ptr<bytes> _buffer)
{
    auto msg = std::make_shared<Message>();
    msg->buffer = _buffer;
    msg->incomeTimePoint = std::chrono::high_resolution_clock::now();

    std::unique_lock<boost::shared_mutex> lock(x_queue);
    auto isEmpty = m_queue.empty();
    m_queue.push_back(msg);
    if (isEmpty) {
        async_write();
    }
}

void WsSession::async_send_message(std::shared_ptr<WsMessage> _msg, Options _options, RespCallback _respFunc)
{
    if (!is_connected()) {
        if (_respFunc) {
            auto error = std::make_shared<Error>(WsError::SessionDisconnect, "the session has been disconnected");
            _respFunc(error, nullptr, nullptr);
        }
        return;
    }

    if ((int64_t)_msg->data()->size() > (int64_t)m_maxMessageCount) {
        if (_respFunc) {
            auto error = std::make_shared<Error>(WsError::MessageOverflow, "Message size overflow");
            _respFunc(error, nullptr, nullptr);
        }
        return;
    }

    auto buffer = std::make_shared<bytes>();
    _msg->encode(*buffer);
    auto req = _msg->req_id();
    if (_respFunc) {
        auto callback = std::make_shared<CallBack>();
        callback->respCallback = _respFunc;

        if (auto timeout = _options.timeout > 0 ? _options.timeout : m_sendMsgTimeout; timeout > 0) {
            auto timer = std::make_shared<
                boost::asio::
                    deadline_timer>(boost::asio::make_strand(*m_ioc), boost::posix_time::milliseconds(timeout));

            callback->timer = timer;
            auto self = std::weak_ptr<WsSession>(shared_from_this());
            timer->async_wait([self, req](const boost::system::error_code& e) {
                auto session = self.lock();
                if (session) {
                    session->on_response_timeout(e, req);
                }
            });
        }
        add_resp_callback(req, callback);
    }

    {
        boost::asio::post(
            m_streamDelegate->tcp_stream().get_executor(),
            boost::beast::bind_front_handler(&WsSession::on_write, shared_from_this(), buffer));
    }
}

void WsSession::add_resp_callback(const uint64_t& _req, CallBack::Ptr _callback)
{
    std::unique_lock<boost::shared_mutex> lock(x_callBack);
    m_callBacks[_req] = _callback;
}

WsSession::CallBack::Ptr WsSession::response_callback(const uint64_t _req, bool _remove)
{
    CallBack::Ptr callback = nullptr;
    {
        boost::unique_lock<boost::shared_mutex> lock(x_callBack);
        auto it = m_callBacks.find(_req);
        if (it != m_callBacks.end()) {
            callback = it->second;
            if (_remove) {
                m_callBacks.erase(it);
            }
        }
    }

    return callback;
}

void WsSession::on_response_timeout(const boost::system::error_code& _error, const uint64_t& _req)
{
    if (_error) {
        return;
    }

    auto callback = response_callback(_req);
    if (callback) {
        return;
    }

    auto error = std::make_shared<Error>(WsError::TimeOut, "waiting for message response timed out");
    m_threadPool->enqueue([callback, error]() { callback->respCallback(error, nullptr, nullptr); });
}
} // namespace jsonrpc::ws
