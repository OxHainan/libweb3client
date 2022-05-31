// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/common.h"
#include "web3client/common/consts.h"
#include "web3client/message.h"

#include <intx/intx.hpp>
#include <nlohmann/json.hpp>

namespace jsonrpc::ws
{

using uint256_t = intx::uint256;
using uint256 = intx::uint256;
// to/from json converters

struct ProcedureCallBase
{
    std::string method;
    uint16_t id{0};
};

inline void to_json(nlohmann::json& j, const ProcedureCallBase& pc)
{
    j[JSON_RPC] = RPC_VERSION;
    j[ID] = pc.id;
    j[METHOD] = pc.method;
}

inline void from_json(const nlohmann::json& j, ProcedureCallBase& pc)
{
    std::string jsonRpc = j[JSON_RPC];
    if (jsonRpc != RPC_VERSION)
        throw std::runtime_error("Wrong JSON_RPC version: " + j.dump());

    pc.id = j[ID];
    pc.method = j[METHOD].get<std::string>();
}

template <typename T>
struct ProcedureCall : public ProcedureCallBase
{
    T params;
};

template <typename T>
struct JsonSerialiser
{
    static bytes to_serialised(const T& t)
    {
        static_assert(std::is_convertible_v<T, nlohmann::json>, "Cannot convert this type to JSON");
        const nlohmann::json j = t;
        const auto dumped = j.dump();
        return bytes(dumped.begin(), dumped.end());
    }

    static T from_serialised(const bytes& rep)
    {
        const auto j = nlohmann::json::parse(rep.begin(), rep.end());
        return j.get<T>();
    }
};

template <typename T>
void to_json(nlohmann::json& j, const ProcedureCall<T>& pc)
{
    to_json(j, dynamic_cast<const ProcedureCallBase&>(pc));
    j[PARAMS] = pc.params;
}

template <typename T>
void from_json(const nlohmann::json& j, ProcedureCall<T>& pc)
{
    from_json(j, dynamic_cast<ProcedureCallBase&>(pc));
    pc.params = j[PARAMS];
}
template <>
struct ProcedureCall<void> : public ProcedureCallBase
{};

template <>
inline void to_json(nlohmann::json& j, const ProcedureCall<void>& pc)
{
    to_json(j, dynamic_cast<const ProcedureCallBase&>(pc));
    j[PARAMS] = nlohmann::json::array();
}

template <>
inline void from_json(const nlohmann::json& j, ProcedureCall<void>& pc)
{
    from_json(j, dynamic_cast<ProcedureCallBase&>(pc));
}

template <class TTag, typename TParams, typename TResult>
struct RpcBuilder
{
    using Tag = TTag;
    using Params = TParams;

    using Request = ProcedureCall<TParams>;
    using ReqSerialiser = JsonSerialiser<Request>;
    using ResultSerialiser = JsonSerialiser<TResult>;
    using Result = TResult;
    static constexpr auto name = TTag::name;
    static Request make_request(uint16_t n = 0)
    {
        Request req;
        req.id = n;
        req.method = TTag::name;
        return req;
    }
};

using BlockID = std::string;
constexpr auto DefaultBlockID = "latest";
struct AddressWithBlock
{
    std::string address = {};
    BlockID block_id = DefaultBlockID;
};
//
inline void to_json(nlohmann::json& j, const AddressWithBlock& s)
{
    j = nlohmann::json::array();
    j.push_back(s.address);
    j.push_back(s.block_id);
}

inline void from_json(const nlohmann::json& j, AddressWithBlock& s)
{
    s.address = j[0];
    s.block_id = j[1];
}

struct EthSyncingTag
{
    static constexpr auto name = "eth_syncing";
};
using EthSyncing = RpcBuilder<EthSyncingTag, void, bool>;

struct EthBalanceTag
{
    static constexpr auto name = "eth_getBalance";
};

using EthBalance = RpcBuilder<EthBalanceTag, AddressWithBlock, void>;

class RpcMessageFactory : public WsMessageFactory
{
 public:
    RpcMessageFactory() {}
    ~RpcMessageFactory() {}

    WsMessage::Ptr buildRequest() override
    {
        auto req = std::make_shared<WsMessage>();
        req->set_id(next_id());
        return req;
    }

    uint16_t next_id()
    {
        int16_t _id = ++id;
        return _id;
    }

 private:
    std::atomic<uint16_t> id{0};
};
} // namespace jsonrpc::ws
