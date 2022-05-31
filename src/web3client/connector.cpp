// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/connector.h"

#include <string>
namespace jsonrpc::ws
{
void WsConnector::connect(
    const std::string& _host,
    uint16_t _port,
    std::function<void(boost::beast::error_code, const std::string& extErrorMsg, WsStreamDelegate::Ptr)> _callback)
{
    std::string endpoint = _host + ":" + std::to_string(_port);
    if (!insert_pending_conns(endpoint)) {
        _callback(boost::beast::error_code(boost::asio::error::would_block), "", nullptr);
        return;
    }

    auto connector = shared_from_this();
    m_resolver->async_resolve(
        _host.c_str(),
        std::to_string(_port).c_str(),
        [_host,
         endpoint,
         connector,
         _callback](boost::beast::error_code _ec, boost::asio::ip::tcp::resolver::results_type _results) {
            if (_ec) {
                _callback(_ec, "", nullptr);
                connector->erase_pending_conns(endpoint);
                return;
            }

            auto raw_stream = std::make_shared<boost::beast::tcp_stream>(boost::asio::make_strand(*(connector->m_ioc)));
            raw_stream->async_connect(
                _results,
                [_host, endpoint, connector, raw_stream, _callback](
                    boost::beast::error_code _ec,
                    boost::asio::ip::tcp::resolver::results_type::endpoint_type _ep) mutable {
                    if (_ec) {
                        _callback(_ec, "", nullptr);
                        connector->erase_pending_conns(endpoint);
                        return;
                    }

                    auto stream_delegate = connector->m_builder->build(raw_stream);
                    stream_delegate->async_handshake([stream_delegate, connector, _host, endpoint, _ep, _callback](
                                                         boost::beast::error_code _ec) {
                        if (_ec) {
                            _callback(_ec, " ssl handshake failed", nullptr);
                            connector->erase_pending_conns(endpoint);
                            return;
                        }

                        std::string temp_endpoint = _host + ":" + std::to_string(_ep.port());
                        stream_delegate->async_handshake(
                            temp_endpoint,
                            "/",
                            [connector, endpoint, _callback, stream_delegate](boost::beast::error_code _ec) mutable {
                                if (_ec) {
                                    _callback(_ec, "", nullptr);
                                    connector->erase_pending_conns(endpoint);
                                    return;
                                }

                                _callback(_ec, "", stream_delegate);
                                connector->erase_pending_conns(endpoint);
                            });
                    });
                });
        });
}
} // namespace jsonrpc::ws
