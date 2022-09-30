#include <jinx/async.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/libevent.hpp>
#include <jinx/buffer.hpp>

using namespace jinx;
using namespace jinx::buffer;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

typedef BufferAllocator<posix::MemoryProvider, BufferConfigExample> AllocatorType;
typedef AllocatorType::BufferType BufferType;
typedef stream::StreamSocket<asyncio> StreamType;

class Connection : public AsyncRoutine {
    posix::Socket _sock{-1};
    char _buffer[32];

    asyncio::Recv _recv{};
    asyncio::Send _send{};

public:
    Connection& operator ()(posix::Socket&& sock) {
        _sock = std::move(sock);

        async_start(&Connection::recv);
        return *this;
    }

    Async recv() {
        return async_await(_recv(_sock.native_handle(), SliceMutable{_buffer, 5}, 0), &Connection::send);
    }

    Async send() {
        return async_await(_send(_sock.native_handle(), SliceConst{_buffer, (size_t)_recv.get_result()}, 0), &Connection::recv);
    }
};

class StreamTest : public AsyncRoutine {
    AllocatorType* _allocator{};
    stream::Stream* _stream{};
    BufferType _buffer{};

public:
    StreamTest& operator ()(AllocatorType* allocator, StreamType* stream) {
        _allocator = allocator;
        _stream = stream;
        async_start(&StreamTest::foo);
        return *this;
    }

    Async foo() {
        _buffer = _allocator->allocate(BufferConfigExample{});
        if (_buffer == nullptr) {
            return async_throw(posix::ErrorPosix::NotEnoughMemory);
        }
        auto buf = _buffer->slice_for_producer();
        memcpy(buf.data(), "hello", 5);
        _buffer->commit(5) >> JINX_IGNORE_RESULT;
        return *this / _stream->write(&_buffer.get()->view()) / &StreamTest::bar;
    }

    Async bar() {
        _buffer = _allocator->allocate(BufferConfigExample{});
        if (_buffer == nullptr) {
            return async_throw(posix::ErrorPosix::NotEnoughMemory);
        }
        return *this / _stream->read(&_buffer.get()->view()) / &StreamTest::check;
    }

    Async check() {
        auto buf = _buffer->slice_for_consumer();
        jinx_assert(memcmp(buf.data(), "hello", 5) == 0);
        return this->async_throw(ErrorAwaitable::ExitLoop);
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    
    posix::MemoryProvider memory{};
    AllocatorType _allocator{memory};
    StreamType _stream{};

    _allocator.reconfigure<BufferConfigExample>(10, 10);

    int socks[2];
    jinx_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, socks) == 0);

    posix::Socket sock1{socks[0]};
    posix::Socket sock2{socks[1]};

    sock1.set_non_blocking(true);
    sock2.set_non_blocking(true);

    _stream.initialize(std::move(sock1));

    loop.task_new<StreamTest>(&_allocator, &_stream);
    
    loop.task_new<Connection>(std::move(sock2));

    loop.run();
    return 0;
}
