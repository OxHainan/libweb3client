// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#include "web3client/client.h"

#include "web3client/config.h"

#include <iostream>
using namespace jsonrpc::ws;
int main()
{
    auto config = std::make_shared<WsConfig>();
    config->set_endpoint("127.0.0.1", 8546);
    auto service = std::make_shared<Client>(config);
    service->init_jsonrpc();
    service->start();
    std::cout << "Starting client..." << std::endl;

    while (true) {
        std::promise<bool> p;
        auto f = p.get_future();

        service->jsonrpc()->send<EthSyncing>([&p](Error::Ptr _error, std::shared_ptr<bytes> _result) {
            if (_error && _error->errorCode() != 0) {
                std::cout << "errorCode: " << _error->errorCode() << "  errorMessage : " << _error->errorMessage()
                          << std::endl;

                return;
            }

            auto result = EthSyncing::ResultSerialiser::from_serialised(*_result);
            p.set_value(true);
            std::cout << result << std::endl;
        });
        f.get();
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}
