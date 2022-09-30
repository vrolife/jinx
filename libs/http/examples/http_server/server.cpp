#include <csignal>

#include "server.hpp"

namespace example {

class HandshakeSocket : public WebApp<ExampleAppConfig, AllocatorType> 
{
    typedef WebApp<ExampleAppConfig, AllocatorType> BaseType;

    stream::StreamSocket<asyncio> _stream{};

public:
    HandshakeSocket& operator ()(void* data, posix::Socket&& sock, AllocatorType* allocator) 
    {
        _stream.initialize(std::move(sock));
        BaseType::operator()(&_stream, allocator, data);
        return *this;
    }

protected:
    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
           << "HandshakeSocket{ stream: " << &_stream << " }" << std::endl;
        AsyncRoutine::print_backtrace(output_stream, indent + 1);
    }
};

class Acceptor : public AsyncRoutine {
    void* _data{};
    int _fd{-1};
    AllocatorType* _allocator{};
    asyncio::Accept _accept{};
public:
    Acceptor& operator ()(void* data, int sock, AllocatorType* allocator)
    {
        _data = data;
        _fd = sock;
        _allocator = allocator;
        async_start(&Acceptor::accept);
        return *this;
    }

protected:
    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
            << "Acceptor{ fd: " << _fd << " }" << std::endl;
        AsyncRoutine::print_backtrace(output_stream, indent + 1);
    }

    Async accept() {
        return async_await(_accept(_fd, nullptr, nullptr), &Acceptor::spawn);
    }

    Async spawn() {
        posix::Socket client(_accept.get_result());
        client.set_non_blocking(true);
        this->task_allocate<buffer::TaskBufferredConfig<HandshakeSocket>>(
            _allocator, 
            _data,
            std::move(client), 
            _allocator
        ).abort_on(Faileb, "out of memory");
        return accept();
    }
};

} // namespace example

int main(int argc, const char* argv[])
{
    using namespace example;

    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::Socket sock{};

    sock.create(AF_INET, SOCK_STREAM, 0).abort_on(-1, "failed to create socket");
    posix::SocketAddress listen_addr("127.0.0.1", 1980, posix::SocketAddressAbortOnFailure::SocketAddressAbortOnFailure);
    sock.set_non_blocking(true);
    sock.set_reuse_address(true);
    sock.set_reuse_port(true);
    sock.bind(listen_addr).abort_on(-1, "failed to bind address");

    sock.listen(32).abort_on(-1, "failed to listen socket");

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};

    WebAppData data{};

    loop.task_new<Acceptor>(&data, sock.native_handle(), &allocator);

    libevent::EventEngineLibevent::EventHandleSignalFunctional sigiterm;
    eve.add_signal(sigiterm, SIGTERM, [&](const error::Error& error){
        std::cout << "terminated\n";
        loop.exit();
    });

    libevent::EventEngineLibevent::EventHandleSignalFunctional sigint;
    eve.add_signal(sigint, SIGINT, [&](const error::Error& error){
        std::cout << "interrupted\n";
        loop.exit();
    });

    loop.run();
    return 0;
}
