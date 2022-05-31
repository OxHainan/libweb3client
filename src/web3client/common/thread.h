// Copyright (c) Oxford-Hainan Blockchain Research Institute. All rights reserved.
// Licensed under the Apache 2.0 License.
#pragma once
#include "string"

#include <pthread.h>
inline void pthread_setThreadName(std::string const& _n)
{
#if defined(__GLIBC__)
    pthread_setname_np(pthread_self(), _n.c_str());
#elif defined(__APPLE__)
    pthread_setname_np(_n.c_str());
#endif
}