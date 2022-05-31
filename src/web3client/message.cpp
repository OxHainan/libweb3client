// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/message.h"

#include "web3client/response.h"

namespace jsonrpc::ws
{
bool WsMessage::encode(bytes& _buffer)
{
    _buffer.clear();
    _buffer.insert(_buffer.end(), m_data->begin(), m_data->end());
    return true;
}

uint64_t WsMessage::decode(const byte* _buffer, size_t _size)
{
    m_data->clear();
    m_data->insert(m_data->begin(), _buffer, _buffer + _size);
    return _size;
}
} // namespace jsonrpc::ws
