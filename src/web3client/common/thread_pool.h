// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "web3client/common/thread.h"

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <memory>
// #include <pthread.h>
namespace jsonrpc::ws
{
class ThreadPool
{
 public:
    using Ptr = std::shared_ptr<ThreadPool>;

    explicit ThreadPool(const std::string& name, size_t size) : m_name(name), m_work(m_service)
    {
        for (auto i = 0; i < size; i++) {
            m_workers.create_thread([this] {
                pthread_setThreadName(m_name);
                m_service.run();
            });
        }
    }

    void stop()
    {
        m_service.stop();
        if (!m_workers.is_this_thread_in()) {
            m_workers.join_all();
        }
    }

    ~ThreadPool()
    {
        stop();
    }

    template <class F>
    void enqueue(F f)
    {
        m_service.post(f);
    }

 private:
    std::string m_name;
    boost::thread_group m_workers;
    boost::asio::io_service m_service;
    boost::asio::io_service::work m_work;
};
} // namespace jsonrpc::ws
