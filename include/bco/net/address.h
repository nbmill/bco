#pragma once
#ifdef _WIN32
#include <WinSock2.h>
#include <in6addr.h>
#include <ws2ipdef.h>
#else
#endif
#include <cstdint>

namespace bco
{

namespace net
{

class Address {
public:
    Address() = default;
    Address(in_addr ip, uint16_t port);
    Address(const in6_addr& ip, uint16_t port);
    explicit Address(const sockaddr_in& addr);
    explicit Address(const sockaddr_in6& addr);

    uint16_t port() const;
    in_addr ipv4() const;
    in6_addr ipv6() const;
    int family() const;

    void set_ip(in_addr ip);
    void set_ip(in6_addr ip);
    void set_port(uint16_t port);

    bool is_private() const;
    bool is_loopback() const;
    bool is_linklocal() const;
    bool is_private_network() const;
    bool is_shared_network() const;

    sockaddr_storage to_storage() const;
    static Address from_storage(const sockaddr_storage& storage);

private:
    int family_;
    uint16_t port_;
    union IP {
        in_addr v4;
        in6_addr v6;
        IP()
            : v6 {}
        {
        }
        IP(in_addr ip)
            : v4(ip)
        {
        }
        IP(in6_addr ip)
            : v6(ip)
        {
        }
    };
    IP ip_;
};

} // namespace net

} // namespace bco
