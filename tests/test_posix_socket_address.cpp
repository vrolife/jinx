#include <unistd.h>
#include <fcntl.h> 

#include <sys/socket.h>

#include <cstring>
#include <jinx/assert.hpp>
#include <array>
#include <iostream>
#include <sstream>

#include <jinx/posix.hpp>
#include <jinx/slice.hpp>

using namespace jinx::posix;

int main(int argc, const char* argv[]) // NOLINT
{
    try {
        SocketAddress addr{"1.2.3.4", 90, SocketAddressAbortOnFailure};
        uint16_t port = 0;
        jinx_assert(AddressINet::get_port(addr, port).unwrap());
        jinx_assert(port == 90);

        char buf[INET6_ADDRSTRLEN];
        jinx_assert(AddressINet::get_address_string(addr, buf).unwrap() == AddressStatus::Ok);
        jinx_assert(strcmp(buf, "1.2.3.4") == 0);
    } catch(...) {
        jinx_assert(false);
    }

    try {
        SocketAddress addr{"1::1", 90, SocketAddressAbortOnFailure};
        uint16_t port = 0;
        jinx_assert(AddressINet::get_port(addr, port).unwrap());
        jinx_assert(port == 90);

        char buf[INET6_ADDRSTRLEN];
        jinx_assert(AddressINet::get_address_string(addr, buf).unwrap() == AddressStatus::Ok);
        jinx_assert(strcmp(buf, "1::1") == 0);
    } catch(...) {
        jinx_assert(false);
    }

    // try {
    //     SocketAddress addr{"-=-=", 90, SocketAddressAbortOnFailure};
    //     jinx_assert(false);
    // } catch(...) { }

    try {
        SocketAddress addr{};
        jinx_assert(AddressINet::set_address_port(addr, "1.2.3.4", 90).unwrap() == AddressStatus::Ok);
        jinx_assert(AddressINet::set_address_port(addr, "1::1", 90).unwrap() == AddressStatus::Ok);
        jinx_assert(AddressINet::set_address_port(addr, "1.2.3-=-=4", 90).unwrap() == AddressStatus::InvalidAddress);
    } catch(...) {
        jinx_assert(false);
    }

    try {
        SocketAddress addr{};
        uint32_t addr4 = inet_addr("1.2.3.4");
        char buf[INET6_ADDRSTRLEN];
        jinx_assert(AddressINet::set_raw(addr, AF_INET, jinx::SliceConst{&addr4, 3}, 90).unwrap() == AddressStatus::InvalidAddress);
        jinx_assert(AddressINet::set_raw(addr, AF_UNSPEC, jinx::SliceConst{&addr4, 4}, 90).unwrap() == AddressStatus::UnsupportedAddressFamily);
        jinx_assert(AddressINet::set_raw(addr, AF_INET, jinx::SliceConst{&addr4, 4}, 90).unwrap() == AddressStatus::Ok);
        jinx_assert(AddressINet::get_address_string(addr, buf).unwrap() == AddressStatus::Ok);
        jinx_assert(strcmp(buf, "1.2.3.4") == 0);
    } catch(...) {
        jinx_assert(false);
    }

    return 0;
}
