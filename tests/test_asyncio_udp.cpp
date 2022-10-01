#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/posix.hpp>

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

class Writer : public AsyncRoutine {
    int _wfd{-1};
    asyncio::SendTo _sendto{};
    posix::SocketAddress _addr;

public:
    Writer() = default;

    Writer& operator ()(int wfd, posix::SocketAddress& addr)
    {
        async_start(&Writer::write);
        _wfd = wfd;
        _addr = addr;
        return *this;
    }

    Async write() {
        return async_await(_sendto(_wfd, SliceConst{"hello", 5}, 0, _addr.data(), _addr.size()), &Writer::async_return);
    }

    Async finish() {
        return async_return();
    }
};

class Reader : public AsyncRoutine {
    int _rfd{-1};
    asyncio::RecvFrom _recvfrom{};
    char _buf[6]{};
    posix::SocketAddress _addr;
    posix::SocketAddress _addr2;
    socklen_t _len{};

public:
    Reader() = default;

    Reader& operator ()(int rfd, posix::SocketAddress& addr) {
        async_start(&Reader::read);
        _rfd = rfd;
        _addr = addr;
        return *this;
    };

    Async read() {
        return async_await(_recvfrom(_rfd, SliceMutable{_buf, 6}, 0, _addr2), &Reader::finish);
    }

    Async finish() {
        jinx_assert(posix::AddressINet::compare(_addr, _addr2));
        jinx_assert(memcmp("hello", &_buf[0], 5) == 0);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::SocketAddress addr_a("127.0.0.1", 23642, posix::SocketAddressAbortOnFailure);
    posix::SocketAddress addr_b("127.0.0.1", 23643, posix::SocketAddressAbortOnFailure);

    posix::Socket sock_a{::socket(AF_INET, SOCK_DGRAM, 0)};
    posix::Socket sock_b{::socket(AF_INET, SOCK_DGRAM, 0)};

    jinx_assert(sock_a.bind(addr_a).unwrap() == 0);
    jinx_assert(sock_b.bind(addr_b).unwrap() == 0);

    sock_a.set_non_blocking(true);
    sock_b.set_non_blocking(true);

    loop.task_new<Writer>(sock_a.native_handle(), addr_b);
    loop.task_new<Reader>(sock_b.native_handle(), addr_a);

    loop.run();
    return 0;
}
