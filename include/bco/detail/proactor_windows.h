﻿#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <Windows.h>

namespace bco {
class Executor;
class Buffer;

class Proactor {
public:
    Proactor();
    int read(int s, Buffer buff, std::function<void(int length)>&& cb);
    int write(int s, Buffer buff, std::function<void(int length)>&& cb);
    int accept(int s, std::function<void(int s)>&& cb);
    bool connect(sockaddr_in addr, std::function<void(int)>&& cb);
    std::vector<std::function<void()>> drain(uint32_t timeout_ms);
    void attach(int fd);

private:
    void set_executor(Executor* executor);

private:
    ::HANDLE complete_port_;
};

}