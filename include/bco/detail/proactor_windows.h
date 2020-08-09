﻿#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <Windows.h>

#include <bco/buffer.h>

namespace bco {

class Proactor {
public:
    Proactor(Context& executor);
    int read(SOCKET s, Buffer buff, std::function<void(size_t length)>&& cb);
    int write(SOCKET s, Buffer buff, std::function<void(size_t length)>&& cb);
    int accept(SOCKET s, std::function<void()>&& cb);
    bool connect(SOCKADDR_IN& addr, std::function<void()>&& cb);

private:
    ::HANDLE complete_port_;
    Context& executor_;
};

}