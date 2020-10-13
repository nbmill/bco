#include <WinSock2.h>
#include <MSWSock.h>
#include <cstdint>
#include <chrono>
#include <vector>
#include <functional>
#include <bco/detail/proactor_windows.h>
#include <bco/executor.h>

namespace bco {

struct OverlapInfo {
    WSAOVERLAPPED overlapped;
    SOCKET sock;
    std::function<void(size_t)> cb;
};

Proactor::Proactor(Executor& executor)
    : executor_(executor)
{
    this->complete_port_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 1);
}

int Proactor::read(SOCKET s, Buffer buff, std::function<void(int length)>&& cb)
{
    OverlapInfo* overlap_info = new OverlapInfo;
    overlap_info->cb = std::move(cb);
    overlap_info->sock = s;
    ::SecureZeroMemory((PVOID)&overlap_info->overlapped, sizeof(WSAOVERLAPPED));
    WSABUF wsabuf {buff.length(), static_cast<char*>(buff.data())};
    DWORD flags = 0;
    DWORD bytes_transferred;
    int ret = ::WSARecv(s, &wsabuf, 1, &bytes_transferred, &flags, &overlap_info->overlapped, nullptr);
    if (ret == 0) {
        delete overlap_info;
        return bytes_transferred;
    }
    if (ret == SOCKET_ERROR && ::WSAGetLastError() == WSA_IO_PENDING) {
        return 0;
    }
    return -1;
}

int Proactor::write(SOCKET s, Buffer buff, std::function<void(int length)>&& cb)
{
    OverlapInfo* overlap_info = new OverlapInfo;
    overlap_info->cb = std::move(cb);
    overlap_info->sock = s;
    ::SecureZeroMemory((PVOID)&overlap_info->overlapped, sizeof(WSAOVERLAPPED));
    WSABUF wsabuf {buff.length(), static_cast<char*>(buff.data())};
    DWORD flags = 0;
    DWORD bytes_transferred;
    int ret = ::WSASend(s, &wsabuf, 1, &bytes_transferred, flags, &overlap_info->overlapped, nullptr);
    if (ret == 0) {
        delete overlap_info;
        return bytes_transferred;
    }
    if (ret == SOCKET_ERROR && ::WSAGetLastError() == WSA_IO_PENDING) {
        return 0;
    }
    return -1;
}

int Proactor::accept(SOCKET s, std::function<void(SOCKET s)>&& cb)
{
    OverlapInfo* overlap_info = new OverlapInfo;
    overlap_info->cb = std::move(cb);
    overlap_info->sock = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (overlap_info->sock == INVALID_SOCKET) {
        delete overlap_info;
        return -1;
    }
    ::SecureZeroMemory((PVOID)&overlap_info->overlapped, sizeof(WSAOVERLAPPED));
    DWORD bytes;
    bool success = ::AcceptEx(s, overlap_info->sock, buff, 0, len, len, &bytes, &overlap_info->overlapped);
    if (success) {
        SOCKET sock = overlap_info->sock;
        delete overlap_info;
        if (CreateIoCompletionPort(sock, complete_port_, nullptr, 0) == complete_port_)
            return sock;
        else
            return -1;
    }
    if (WSAGetLastError() == WSA_IO_PENDING) {
        return 0;
    }
    return -1;
}

LPFN_CONNECTEX GetConnectEx(SOCKET so)
{
    GUID guid = WSAID_CONNECTEX;
    LPFN_CONNECTEX fnConnectEx = NULL;
    DWORD nBytes = 0;
    if (SOCKET_ERROR == WSAIoctl(so, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &fnConnectEx, sizeof(fnConnectEx), &nBytes, NULL, NULL)) {
        return NULL;
    }

    return fnConnectEx;
}

bool Proactor::connect(SOCKADDR_IN& addr, std::function<void()>&& cb)
{
    OverlapInfo* overlap_info = new OverlapInfo;
    overlap_info->cb = std::move(cb);
    overlap_info->sock = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (overlap_info->sock == INVALID_SOCKET) {
        delete overlap_info;
        return -1;
    }
    ::SecureZeroMemory((PVOID)&overlap_info->overlapped, sizeof(WSAOVERLAPPED));
    LPFN_CONNECTEX ConnectEx = GetConnectEx(overlap_info->sock);
    bool success = ConnectEx(s, (SOCKADDR*)&addr, sizeof(addr), nullptr, 0, nullptr, &overlap_info->overlapped);
    if (success) {
        return success;
    }
    delete overlap_info;
    return false;
}

std::vector<std::function<void()>> Proactor::drain(uint32_t timeout_ms)
{
    DWORD bytes;
    LPOVERLAPPED overlapped;
    ULONG_PTR completion_key;
    DWORD remain_ms;
    if (timeout_ms == std::numeric_limits<uint32_t>::max()) {
        remain_ms = INFINITE;
    } else {
        remain_ms = timeout_ms;
    }
    std::vector<std::function<void()>> cbs;
    while (true) {
        int ret = ::GetQueuedCompletionStatus(complete_port_, &bytes, &completion_key, &overlapped, remain_ms);
        if (ret == 0 && overlapped == 0) {
            auto err = ::GetLastError();
            switch (err) {
            case WAIT_TIMEOUT:
                std::this_thread::yield();
                break;
            default:
                break;
            }
        } else if (ret == 0 && overlappped != 0) {
            auto err = ::GetLastError();
            break;
        } else if (ret != 0 && overlapped == 0) {
            assert(false);
        } else if (ret != 0 && overlapped != 0) {
            //成功
            OverlapInfo* overlap_info = reinterpret_cast<OverlapInfo*>(overlapped);
            cbs.push_back(std::move(overlap_info->cb));
            delete overlap_info;
        } else {
            assert(false);
        }
    }
    return std::move(cbs);
}

}
