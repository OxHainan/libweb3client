// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "iostream"
#include "web3client/common/consts.h"
#include "web3client/rpc/message.h"

#include <nlohmann/json.hpp>
#include <string>
namespace jsonrpc::ws
{
struct Response
{
    uint16_t id;
    std::vector<uint8_t> result;
    friend void from_json(const nlohmann::json& j, Response& r);
    friend void to_json(nlohmann::json& j, const Response& r);
};

inline void to_json(nlohmann::json& j, const Response& r)
{
    j[JSON_RPC] = RPC_VERSION;
    j[ID] = r.id;
    // j[RESULT] = r.result;
}

inline void from_json(const nlohmann::json& j, Response& r)
{
    std::string jsonRpc = j[JSON_RPC];
    if (jsonRpc != RPC_VERSION) {
        throw std::runtime_error("Wrong JSON_RPC version: " + j.dump());
    }

    r.id = j[ID];
    // r.result = j[RESULT];
    auto s = j[RESULT].dump();
    r.result = decltype(r.result)(s.begin(), s.end());
}

class ResponseMessage
{
 public:
    using Ptr = std::shared_ptr<ResponseMessage>;
    ResponseMessage(const std::vector<uint8_t>& _resp) : m_resp(nlohmann::json::parse(_resp)) {}

    std::shared_ptr<std::vector<uint8_t>> result()
    {
        return std::make_shared<std::vector<uint8_t>>(m_resp.result);
    }

    uint16_t id()
    {
        return m_resp.id;
    }

    static ResponseMessage::Ptr make_response(const std::vector<uint8_t>& _resp)
    {
        return std::make_shared<ResponseMessage>(_resp);
    }

 private:
    Response m_resp;
};
} // namespace jsonrpc::ws
