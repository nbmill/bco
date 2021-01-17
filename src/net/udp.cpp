#include <bco/net/udp.h>
#include <bco/net/proactor/select.h>
#ifdef _WIN32
#include <bco/net/proactor/iocp.h>
#else
#include <bco/net/proactor/epoll.h>
#endif // _WIN32

namespace bco {

namespace net {

template class UdpSocket<Select>;
#ifdef _WIN32
template class UdpSocket<IOCP>;
#else
template class UdpSocket<Epoll>;
#endif // _WIN32

template <SocketProactor P>
std::tuple<UdpSocket<P>, int> UdpSocket<P>::create(P* proactor, int family)
{
    int fd = proactor->create(family, SOCK_DGRAM);
    if (fd < 0)
        return { UdpSocket {}, -1 };
    else
        return { UdpSocket { proactor, family, fd }, 0 };
}

template <SocketProactor P>
UdpSocket<P>::UdpSocket(P* proactor, int family, int fd)
    : proactor_(proactor)
    , family_(family)
    , socket_(fd)
{
}

template <SocketProactor P>
ProactorTask<int> UdpSocket<P>::recv(std::span<std::byte> buffer)
{
    ProactorTask<int> task;
    int error = proactor_->recv(socket_, buffer, [task](int length) mutable {
        if (task.await_ready())
            return;
        task.set_result(std::forward<int>(length));
        task.resume();
    });
    if (error < 0) {
        task.set_result(std::forward<int>(error));
    }
    return task;
}

template <SocketProactor P>
ProactorTask<std::tuple<int, Address>> UdpSocket<P>::recvfrom(std::span<std::byte> buffer)
{
    ProactorTask<std::tuple<int, Address>> task;
    auto error = proactor_->recvfrom(socket_, buffer, [task](int length, const sockaddr_storage& remote_addr) mutable {
        if (task.await_ready())
            return;
        task.set_result(std::make_tuple(length, Address::from_storage(remote_addr)));
        task.resume();
    });
    if (error < 0) {
        task.set_result(std::make_tuple(error, Address{}));
    }
    return task;
}

template <SocketProactor P>
int UdpSocket<P>::send(std::span<std::byte> buffer)
{
    return proactor_->send(socket_, buffer);
}

template <SocketProactor P>
int UdpSocket<P>::sendto(std::span<std::byte> buffer, const Address& addr)
{
    return proactor_->sendto(socket_, buffer, addr.to_storage());
}

template <SocketProactor P>
int UdpSocket<P>::bind(const Address& addr)
{
    auto storage = addr.to_storage();
    return proactor_->bind(socket_, storage);
}

template <SocketProactor P>
int UdpSocket<P>::connect(const Address& addr)
{
    auto storage = addr.to_storage();
    return proactor_->connect(socket_, storage);
}

}

}