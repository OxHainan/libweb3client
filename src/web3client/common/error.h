// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include <string>
namespace jsonrpc::ws
{
#define XX_STANDARD_ERROR_CODES \
    XX(PARSE_ERROR, -32700) \
    XX(INVALID_REQUEST, -32600) \
    XX(METHOD_NOT_FOUND, -32601) \
    XX(INVALID_PARAMS, -32602) \
    XX(INTERNAL_ERROR, -32603) \
    XX(SERVER_ERROR_START, -32000) \
    XX(SERVER_ERROR_END, -32099)
using ErrorBaseType = int;
enum class StandardErrorCodes : ErrorBaseType
{
#define XX(Name, Value) Name = Value,
    XX_STANDARD_ERROR_CODES
#undef XX
};

inline std::string get_error_prefix(StandardErrorCodes ec)
{
#define XX(Name, Value) \
    case (StandardErrorCodes::Name): \
        return "[" #Name "]: ";

    switch (ec) {
        XX_STANDARD_ERROR_CODES
    }
#undef XX

    return "";
}

enum WsError
{
    AcceptError = -4000,
    ReadError = -4001,
    WriteError = -4002,
    PingError = -4003,
    PongError = -4004,
    PacketError = -4005,
    SessionDisconnect = -4006,
    UserDisconnect = -4007,
    TimeOut = -4008,
    NoActiveCons = -4009,
    EndPointNotExist = -4010,
    MessageOverflow = -4011,
    UndefinedException = -4099
};

inline bool notRetryAgain(int _wsError)
{
    return (_wsError == WsError::MessageOverflow);
}

}