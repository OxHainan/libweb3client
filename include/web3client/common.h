// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/error.h"

#include <boost/beast/websocket.hpp>
#include <functional>
#include <vector>
namespace jsonrpc::ws
{
class ResponseMessage;
class WsSession;
using RespCallback = std::function<void(Error::Ptr, std::shared_ptr<ResponseMessage>, std::shared_ptr<WsSession>)>;
using WsConnectHandler = std::function<void(Error::Ptr, std::shared_ptr<WsSession>)>;
using WsDisconnectHandler = std::function<void(Error::Ptr, std::shared_ptr<WsSession>)>;
using WsRecvMessageHandler = std::function<void(std::shared_ptr<ResponseMessage>, std::shared_ptr<WsSession>)>;

using byte = uint8_t;
using bytes = std::vector<uint8_t>;
struct Options
{
    Options(const uint32_t _timeout) : timeout(_timeout) {}
    Options() : timeout(0) {}
    uint32_t timeout = 0;
};

} // namespace jsonrpc::ws
