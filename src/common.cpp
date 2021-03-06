#ifdef _WIN32
//#include <WinSock2.h>
//#include <WS2tcpip.h>
#else
#include <sys/socket.h>
#include <sys/uio.h>
#endif // _WIN32

#include "common.h"

namespace bco {

int syscall_sendv(int s, bco::Buffer buff)
{
    const auto slices = buff.data();
#ifdef _WIN32
    std::vector<WSABUF> wsabuf(slices.size());
    for (size_t i = 0; i < wsabuf.size(); i++) {
        wsabuf[i].buf = reinterpret_cast<CHAR*>(slices[i].data());
        wsabuf[i].len = static_cast<ULONG>(slices[i].size());
    }
    DWORD bytes_sent = 0;
    int ret = ::WSASend(s, wsabuf.data(), static_cast<DWORD>(wsabuf.size()), &bytes_sent, 0, nullptr, nullptr);
    if (ret != 0) {
        return SOCKET_ERROR;
    } else {
        return bytes_sent;
    }
#else
    std::vector<::iovec> iovecs(slices.size());
    for (size_t i = 0; i < iovecs.size(); i++) {
        iovecs[i] = ::iovec { slices[i].data(), slices[i].size() };
    }
    return ::writev(s, iovecs.data(), iovecs.size());
#endif // _WIN32
}

int syscall_recvv(int s, bco::Buffer buff)
{
    auto slices = buff.data();
#ifdef _WIN32
    std::vector<WSABUF> wsabuf(slices.size());
    for (size_t i = 0; i < wsabuf.size(); i++) {
        wsabuf[i].buf = reinterpret_cast<CHAR*>(slices[i].data());
        wsabuf[i].len = static_cast<ULONG>(slices[i].size());
    }
    DWORD bytes_read = 0;
    DWORD flag = 0;
    int ret = ::WSARecv(s, wsabuf.data(), static_cast<DWORD>(wsabuf.size()), &bytes_read, &flag, nullptr, nullptr);
    if (ret != 0) {
        return SOCKET_ERROR;
    } else {
        return bytes_read;
    }
#else
    std::vector<::iovec> iovecs(slices.size());
    for (size_t i = 0; i < iovecs.size(); i++) {
        iovecs[i] = ::iovec { slices[i].data(), slices[i].size() };
    }
    return ::readv(s, iovecs.data(), iovecs.size());
#endif // _WIN32
}

int syscall_sendmsg(int s, bco::Buffer buff, const sockaddr_storage& addr, void* sendmsg_func)
{
    const auto slices = buff.data();
#ifdef _WIN32
    std::vector<WSABUF> wsabuf(slices.size());
    for (size_t i = 0; i < wsabuf.size(); i++) {
        wsabuf[i].buf = reinterpret_cast<CHAR*>(slices[i].data());
        wsabuf[i].len = static_cast<ULONG>(slices[i].size());
    }
    WSAMSG wsamsg {
        .name = const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(&addr)),
        .namelen = addr.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6),
        .lpBuffers = wsabuf.data(),
        .dwBufferCount = static_cast<ULONG>(wsabuf.size()),
        .Control = WSABUF {},
        .dwFlags = 0,
    };
    DWORD bytes_sent = 0;
    auto WSASendMsg = static_cast<LPFN_WSASENDMSG>(sendmsg_func);
    int ret = WSASendMsg(s, &wsamsg, 0, &bytes_sent, nullptr, nullptr);
    if (ret != 0) {
        return SOCKET_ERROR;
    } else {
        return bytes_sent;
    }
#else
    (void)sendmsg_func;
    std::vector<::iovec> iovecs(slices.size());
    for (size_t i = 0; i < iovecs.size(); i++) {
        iovecs[i] = ::iovec { slices[i].data(), slices[i].size() };
    }
    ::msghdr hdr {
        .msg_name = const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(&addr)),
        .msg_namelen = sizeof(addr),
        .msg_iov = iovecs.data(),
        .msg_iovlen = iovecs.size(),
        .msg_control = nullptr,
        .msg_controllen = 0,
        .msg_flags = 0,
    };
    return ::sendmsg(s, &hdr, 0);
#endif // _WIN32
}

int syscall_recvmsg(int s, bco::Buffer buff, sockaddr_storage& addr, void* recvmsg_func)
{
    auto slices = buff.data();
#ifdef _WIN32
    std::vector<WSABUF> wsabuf(slices.size());
    for (size_t i = 0; i < wsabuf.size(); i++) {
        wsabuf[i].buf = reinterpret_cast<CHAR*>(slices[i].data());
        wsabuf[i].len = static_cast<ULONG>(slices[i].size());
    }
    WSAMSG wsamsg {
        .name = reinterpret_cast<sockaddr*>(&addr),
        .namelen = sizeof(addr),
        .lpBuffers = wsabuf.data(),
        .dwBufferCount = static_cast<ULONG>(wsabuf.size()),
        .Control = WSABUF {},
        .dwFlags = 0,
    };
    DWORD bytes_read = 0;
    auto WSARecvMsg = static_cast<LPFN_WSARECVMSG>(recvmsg_func);
    int ret = WSARecvMsg(s, &wsamsg, &bytes_read, nullptr, nullptr);
    if (ret != 0) {
        return SOCKET_ERROR;
    } else {
        return bytes_read;
    }
#else
    (void)func_addr;
    std::vector<::iovec> iovecs(slices.size());
    for (size_t i = 0; i < iovecs.size(); i++) {
        iovecs[i] = ::iovec { slices[i].data(), slices[i].size() };
    }
    ::msghdr hdr {
        .msg_name = const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(&addr)),
        .msg_namelen = sizeof(addr),
        .msg_iov = iovecs.data(),
        .msg_iovlen = iovecs.size(),
        .msg_control = nullptr,
        .msg_controllen = 0,
        .msg_flags = 0,
    };
    return ::recvmsg(s, &hdr, 0);
#endif // _WIN32
}

} // namespace bco