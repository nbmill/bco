#include <bco/net/tcp.h>
#include <bco/net/proactor/select.h>
#ifdef _WIN32
#include <bco/net/proactor/iocp.h>
#else
#include <bco/net/proactor/epoll.h>
#endif // _WIN32
#include "../common.h"

namespace bco {

namespace net {

template class TcpSocket<Select>;
#ifdef _WIN32
template class TcpSocket<IOCP>;
#else
template class TcpSocket<Epoll>;
#endif // _WIN32

template <SocketProactor P>
std::tuple<TcpSocket<P>, int> TcpSocket<P>::create(P* proactor, int family)
{
    int fd = proactor->create(family, SOCK_STREAM);
    if (fd < 0)
        return { TcpSocket {}, -1 };
    else
        return { TcpSocket { proactor, family, fd }, 0 };
}

template <SocketProactor P>
TcpSocket<P>::TcpSocket(P* proactor, int family, int fd)
    : proactor_(proactor)
    , family_(family)
    , socket_(fd)
{
}

template <SocketProactor P>
Task<int> TcpSocket<P>::recv(bco::Buffer buffer)
{
    Task<int> task;
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
Task<int> TcpSocket<P>::send(bco::Buffer buffer)
{
    Task<int> task;
    int size = proactor_->send(socket_, buffer, [task](int length) mutable {
        if (task.await_ready())
            return;
        task.set_result(std::forward<int>(length));
        task.resume();
    });
    if (size != 0) {
        task.set_result(std::forward<int>(size));
    }
    return task;
}

template <SocketProactor P>
Task<std::tuple<TcpSocket<P>, Address>> TcpSocket<P>::accept()
{
    Task<std::tuple<TcpSocket<P>, Address>> task;
    auto proactor = proactor_;
    int ret = proactor_->accept(socket_, [task, proactor](int fd, const ::sockaddr_storage& address) mutable {
        if (task.await_ready())
            return;
        TcpSocket s { proactor, address.ss_family, fd };
        task.set_result(std::make_tuple(s, Address::from_storage(address )));
        task.resume();
    });
    if (ret < 0) {
        TcpSocket s{};
        task.set_result(std::make_tuple(s, Address {}));
    }
    return task;
}

template <SocketProactor P>
Task<int> TcpSocket<P>::connect(const Address& addr)
{
    Task<int> task;
    sockaddr_storage storage {};
    int ret = proactor_->connect(socket_, addr.to_storage(storage), [task](int ret) mutable {
        if (ret == 0) {
            task.set_result(0);
        } else if (ret < 0) {
            task.set_result(-errno);
        } else {
            assert(false);
        }
        task.resume();
    });
    if (ret < 0) {
        task.set_result(std::move(ret));
    }
    return task;
}

template <SocketProactor P>
int TcpSocket<P>::listen(int backlog)
{
    return listen_socket(socket_, backlog);
}

template <SocketProactor P>
int TcpSocket<P>::bind(const Address& addr)
{
    sockaddr_storage storage {};
    return bind_socket(socket_, addr.to_storage(storage));
}

template <SocketProactor P>
void TcpSocket<P>::shutdown(Shutdown how)
{
    shutdown_socket(socket_, static_cast<int>(how));
}

template <SocketProactor P>
void TcpSocket<P>::close()
{
    close_socket(socket_);
}

} // namespace net

} // namespace bco
