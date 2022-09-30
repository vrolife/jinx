#include <jinx/assert.hpp>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/posix.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

int counter = 0;

class Connection : public AsyncRoutine {
    posix::Socket _sock{-1};
    char _buffer[32];

    asyncio::Recv _recv{};
    asyncio::Send _send{};

public:
    Connection& operator ()(posix::Socket&& sock) {
        _sock = std::move(sock);
        _sock.set_non_blocking(true);

        async_start(&Connection::recv);
        return *this;
    }

    Async recv() {
        return async_await(_recv(_sock.native_handle(), SliceMutable{_buffer, 32}, 0), &Connection::send);
    }

    Async send() {
        counter += 1;
        return async_await(_send(_sock.native_handle(), SliceConst{_buffer, (size_t)_recv.get_result()}, 0), &Connection::recv);
    }
};

class Acceptor : public AsyncRoutine {
    posix::Socket _sock{};
    asyncio::Accept _accept{};

public:
    Acceptor& operator ()(posix::Socket&& sock) {
        _sock = std::move(sock);
        _sock.set_non_blocking(true);
        async_start(&Acceptor::accept);
        return *this;
    }

    Async accept() {
        return async_await(_accept(_sock.native_handle(), nullptr, nullptr), &Acceptor::spawn);
    }

    Async spawn() {
        posix::Socket sock(_accept.get_result());

        posix::SocketAddress peer{};
        jinx_assert(sock.get_peer_name(peer).unwrap() == 0);

        std::cout << "peer: " << peer << std::endl;

        this->task_new<Connection>(std::move(sock));
        return accept();
    }
};

class Client : public AsyncRoutine {
    posix::Socket _sock{};
    posix::SocketAddress _addr{};
    char _buffer[32];

    asyncio::Connect _connect{};
    asyncio::Send _send{};
    asyncio::Recv _recv{};

public:
    Client& operator ()(const posix::SocketAddress& addr) {
        if (_sock.create(AF_INET, SOCK_STREAM, 0).unwrap() < 0) {
            async_start(&Client::bad_fd);
        } else {
            _sock.set_non_blocking(true);
            _addr = addr;
            async_start(&Client::__yield);
        }
        return *this;
    }

    Async bad_fd() {
        return async_throw(posix::ErrorPosix::BadFileDescriptor);
    }

    Async __yield() {
        return this->async_yield(&Client::connect);
    }

    Async connect() {
        return async_await(_connect(_sock.native_handle(), _addr.data(), _addr.size()), &Client::send);
    }

    Async send() {
        return async_await(_send(_sock.native_handle(), SliceConst{"hello", 5}, 0), &Client::recv);
    }

    Async recv() {
        return async_await(_recv(_sock.native_handle(), SliceMutable{_buffer, 32}, 0), &Client::check);
    }

     Async check() {
         throw counter; // NOLINT
         return async_return();
     }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::SocketAddress addr{"127.0.0.1", 0, posix::SocketAddressAbortOnFailure};

    posix::Socket sock{::socket(AF_INET, SOCK_STREAM, 0)};
    sock.set_receive_buffer_size(4096);
    sock.set_send_buffer_size(4096);
    sock.set_reuse_address(true);
    sock.set_reuse_port(true);
    jinx_assert(sock.bind(addr).unwrap() == 0);
    jinx_assert(sock.listen(32).unwrap() == 0);
    jinx_assert(sock.get_socket_name(addr).unwrap() == 0);

    jinx_assert(sock.get_receive_buffer_size() >= 4096);
    jinx_assert(sock.get_send_buffer_size() >= 4096);

    std::cout << "listen: " << addr << std::endl;

    loop.task_new<Acceptor>(std::move(sock));
    loop.task_new<Client>(addr);

    try {
        loop.run();
    } catch(int err) {
        jinx_assert(err == 1);
        loop.exit();
    }
    return 0;
}
