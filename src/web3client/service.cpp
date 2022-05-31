// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/service.h"

#include "iostream"
#include "web3client/common/error.h"
#include "web3client/rpc/message.h"
namespace jsonrpc::ws
{
WsService::WsService(WsConfig::ConstPtr _config) :
  m_config(_config),
  m_iocThreadCount(_config->thread_pool_count()),
  m_ioc(std::make_shared<boost::asio::io_context>(m_iocThreadCount)),
  m_connector(std::make_shared<WsConnector>(m_ioc)),
  //   m_messageFactory(std::make_shared<RpcMessageFactory>()),
  m_threadPool(std::make_shared<ThreadPool>("t_ws_pool", m_iocThreadCount))
{}

WsService::~WsService()
{
    stop();
}

void WsService::start()
{
    if (m_running)
        return;

    m_running = true;
    start_ioc_thread();

    if (auto endpoint = m_config->remote_endpoint(); endpoint) {
        connect_to_service(endpoint);
    }

    reconnect();
    heartbeat();
}

void WsService::stop()
{
    if (!m_running) {
        return;
    }

    m_running = false;
    if (m_reconnect)
        m_reconnect->cancel();

    if (m_heartbeat)
        m_heartbeat->cancel();

    stop_ioc_thread();
}

void WsService::start_ioc_thread()
{
    m_iocThreads = std::make_shared<std::vector<std::thread>>();
    m_iocThreads->reserve(m_iocThreadCount);
    for (size_t i = 0; i < m_iocThreadCount; ++i) {
        m_iocThreads->emplace_back([&, i] {
            pthread_setThreadName("t_ws_ioc_" + std::to_string(i));
            while (m_running) {
                try {
                    m_ioc->run();
                }
                catch (const std::exception& e) {
                    // std::cerr << e.what() << '\n';
                }

                if (m_running)
                    m_ioc->restart();
            }
        });
    }
}
void WsService::stop_ioc_thread()
{
    if (m_iocThreads && !m_iocThreads->empty()) {
        for (auto& t : *m_iocThreads) {
            if (t.get_id() != std::this_thread::get_id()) {
                t.join();
            } else {
                t.detach();
            }
        }
    }
}

void WsService::heartbeat()
{
    m_heartbeat = std::make_shared<boost::asio::deadline_timer>(
        boost::asio::make_strand(*m_ioc), boost::posix_time::milliseconds(m_config->heartbeat_period()));
    auto self = std::weak_ptr<WsService>(shared_from_this());
    m_heartbeat->async_wait([self](const boost::system::error_code&) {
        auto service = self.lock();
        if (!service) {
            return;
        }
        service->heartbeat();
    });
}

void WsService::connect_to_service(EndPointConstPtr _endpoint)
{
    std::string errorMsg;
    size_t sucCount = 0;
    if (auto promise = async_connect_service(_endpoint); promise) {
        auto fut = promise->get_future();
        auto status = fut.wait_for(std::chrono::milliseconds(m_waitConnectFinishTimeout));
        switch (status) {
            case std::future_status::deferred:
                break;
            case std::future_status::timeout:
                errorMsg = "connect timeout, endpoint: " + _endpoint->endpoint();
                break;
            case std::future_status::ready:
                try {
                    auto result = fut.get();
                    if (result.first) {
                        errorMsg = (result.second.empty() ? result.first.message() :
                                                            result.second + " " + result.first.message()) +
                            ", endpoint: " + _endpoint->endpoint();
                    } else {
                        sucCount++;
                    }
                }
                catch (const std::exception& e) {
                    // std::cerr << e.what() << '\n';
                }
                break;
        }
    }

    if (sucCount == 0) {
        stop();
        BOOST_THROW_EXCEPTION(std::runtime_error(errorMsg));
        return;
    }
}

std::shared_ptr<std::promise<std::pair<boost::beast::error_code, std::string>>> WsService::async_connect_service(
    EndPointConstPtr _endpoint)
{
    auto promise = std::make_shared<std::promise<std::pair<boost::beast::error_code, std::string>>>();
    if (_endpoint) {
        auto self = std::weak_ptr<WsService>(shared_from_this());
        m_connector->connect(
            _endpoint->host,
            _endpoint->port,
            [promise, self, _endpoint](
                boost::beast::error_code _ec,
                const std::string& _extErrorMessage,
                WsStreamDelegate::Ptr _streamDelegate) {
                auto service = self.lock();
                if (!service)
                    return;

                auto futResult = std::make_pair(_ec, _extErrorMessage);
                promise->set_value(futResult);
                if (_ec)
                    return;

                auto session = service->new_session(_streamDelegate);
                session->set_connectd_endpoint(_endpoint->endpoint());
                session->start();
            });
    }
    return promise;
}

void WsService::reconnect()
{
    auto self = std::weak_ptr<WsService>(shared_from_this());
    m_reconnect = std::make_shared<boost::asio::deadline_timer>(
        boost::asio::make_strand(*m_ioc), boost::posix_time::milliseconds(m_config->reconnect_period()));

    m_reconnect->async_wait([self, this](const boost::system::error_code&) {
        auto service = self.lock();
        if (!service)
            return;

        if (auto endpoint = m_config->remote_endpoint(); endpoint) {
            if (!m_session) {
                std::cout << "***    reconnect service  ****** " << std::endl;
                async_connect_service(endpoint);
            }
        }
        service->reconnect();
    });
}

WsSession::Ptr WsService::new_session(WsStreamDelegate::Ptr _streamDelegate)
{
    _streamDelegate->set_message_max(m_config->max_msg_size());
    auto session = std::make_shared<WsSession>(m_config, m_ioc, _streamDelegate, m_threadPool, m_messageFactory);
    auto self = std::weak_ptr<WsService>(shared_from_this());
    session->set_connectHandler([self](Error::Ptr _error, WsSession::Ptr _session) {
        auto service = self.lock();
        if (service) {
            service->on_connect(_error, _session);
        }
    });

    session->set_disconnectHandler([self](Error::Ptr _error, WsSession::Ptr _session) {
        auto service = self.lock();
        if (service) {
            service->on_disconnect(_error, _session);
        }
    });

    return session;
}

void WsService::add_session(WsSession::Ptr _session)
{
    boost::unique_lock<boost::shared_mutex> lock(x_mutex);

    m_session = _session;
    if (m_connectHandler)
        m_connectHandler(_session);
}

void WsService::remove_session()
{
    boost::unique_lock<boost::shared_mutex> lock(x_mutex);
    std::cout << "remove session, at endpoint " << m_session->endpoint() << std::endl;

    m_session.reset();
}

void WsService::on_connect(Error::Ptr _error, WsSession::Ptr _session)
{
    std::ignore = _error;
    if (!_session) {
        throw std::runtime_error("session not found");
    }

    add_session(_session);
}

void WsService::on_disconnect(Error::Ptr _error, WsSession::Ptr _session)
{
    std::ignore = _error;
    remove_session();
    if (m_disconnectHandler)
        m_disconnectHandler(_session);
}

void WsService::async_send_message(WsMessage::Ptr _message, Options _options, RespCallback _respFunc)
{
    if (!m_session) {
        auto error =
            std::make_shared<Error>(WsError::EndPointNotExist, "there has no connection of the endpoint exist");
        _respFunc(error, nullptr, nullptr);
        return;
    }

    m_session->async_send_message(_message, _options, _respFunc);
}

} // namespace jsonrpc::ws
