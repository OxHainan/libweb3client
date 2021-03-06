// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/client.h"

#include "iostream"
#include "web3client/response.h"
#include "web3client/rpc/message.h"
namespace jsonrpc::ws
{
Client::Client(WsConfig::ConstPtr _config) : WsService(_config)
{
    set_messageFactory(std::make_shared<RpcMessageFactory>());
}

void Client::start()
{
    WsService::start();
    wait_for_connectionEstablish();
}

void Client::stop()
{
    WsService::stop();
}

std::shared_ptr<Client> Client::get_instance(WsConfig::ConstPtr _config)
{
    static std::shared_ptr<Client> client;
    if (client == nullptr) {
        if (_config == nullptr) {
            throw std::invalid_argument("websocket config is null");
        }

        client = std::shared_ptr<Client>(new Client(_config));
    }

    return client;
}

JsonRpcImpl::Ptr Client::jsonrpc()
{
    if (m_jsonrpc == nullptr) {
        throw std::runtime_error("JSON_RPC cannot to be initialized");
    }

    return m_jsonrpc;
}

void Client::wait_for_connectionEstablish()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto timeout = start + std::chrono::milliseconds(m_waitConnectFinishTimeout);
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (handshakeSucCount()) {
            break;
        }

        if (std::chrono::high_resolution_clock::now() < timeout) {
            continue;
        } else {
            stop();
            BOOST_THROW_EXCEPTION(std::runtime_error("The websocket connection handshake timeout"));
            return;
        }
    }
}

void Client::on_connect(Error::Ptr _error, WsSession::Ptr _session)
{
    WsService::on_connect(_error, _session);
    start_handshake(_session);
}

void Client::on_disconnect(Error::Ptr _error, WsSession::Ptr _session)
{
    WsService::on_disconnect(_error, _session);
}

bool Client::check_handshake(WsSession::Ptr _session)
{
    return true;
}

void Client::start_handshake(WsSession::Ptr _session)
{
    auto msg = m_jsonrpc->set_handshake_message();
    auto session = _session;
    auto client = std::dynamic_pointer_cast<Client>(shared_from_this());

    _session->async_send_message(
        msg,
        Options(m_handshakeTimeout),
        [session, client](Error::Ptr _error, ResponseMessage::Ptr _msg, WsSession::Ptr _session) {
            if (_error && _error->errorCode() != 0) {
                // std::cout << "errorMessage" << session->endpoint() << std::endl;
                std::cout << "endpoint" << _error->errorMessage() << std::endl;

                session->drop(WsError::UserDisconnect);
                return;
            }

            client->increase_handshakeSucCount();
            client->call_handshakeSucHandler(_session);
        });
}

void Client::init_jsonrpc()
{
    auto client = std::dynamic_pointer_cast<Client>(shared_from_this());
    m_jsonrpc = std::make_shared<JsonRpcImpl>(messageFactory());
    m_jsonrpc->set_sender([client](WsMessage::Ptr _message, RespFunc _respFunc) {
        client->async_send_message(
            _message, Options(), [_respFunc](Error::Ptr _error, ResponseMessage::Ptr _msg, WsSession::Ptr _session) {
                (void)_session;
                if (auto err = _msg->error(); err) {
                    if (!_error)
                        _error = std::make_shared<Error>();
                    _error->setErrorCode(WsError::UndefinedException);
                    _error->setErrorMessage(std::string(err->begin(), err->end()));
                }
                _respFunc(_error, _msg ? _msg->result() : nullptr);
            });
    });
}
}