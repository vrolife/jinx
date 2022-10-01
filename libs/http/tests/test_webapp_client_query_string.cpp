#include <iostream>

#include <jinx/async.hpp>
#include <jinx/buffer.hpp>
#include <jinx/posix.hpp>
#include <jinx/libevent.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/http/webapp.hpp>
#include <jinx/http/client.hpp>

using namespace jinx;
using namespace jinx::http;
using namespace jinx::stream;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

struct PageIndex : WebPage {
    typedef WebPage BaseType;

    buffer::BufferView _buffer{};

    Async http_handle_request() override 
    {
        auto qstr = get_query_string();

        _buffer = buffer::BufferView {
            const_cast<char*>(qstr.begin()),
            qstr.size(),
            0,
            qstr.size()
        };

        write_response_line(200) << "Ok";
        write_response_field("Connection") << "close";
        write_response_field("Content-Type") << "text/html; charset=UTF-8";
        write_response_field("Content-Length") << _buffer.size();
        return send_response(&PageIndex::send_body);
    }

    Async send_body() {
        auto pair = get_stream();
        return *this / pair.first->write(&_buffer) / &PageIndex::async_return;
    }
};

struct Root {
    typedef PageIndex Index;
    typedef std::tuple<> ChildNodes;
};

typedef WebConfig<Root> AppConfig;

typedef buffer::BufferAllocator
<
    posix::MemoryProvider, 
    AppConfig::HTTPConfig::BufferConfig, 
    AppConfig::BufferConfig
> AllocatorType;

typedef AllocatorType::BufferType BufferType;

class AsyncHandshake : public WebApp<AppConfig, AllocatorType> {
    typedef WebApp<AppConfig, AllocatorType> BaseType;
    StreamSocket<asyncio> _stream{};

public:
    AsyncHandshake& operator ()(posix::Socket&& sock, AllocatorType* allocator) {
        _stream.initialize(std::move(sock));
        BaseType::operator()(&_stream, allocator, nullptr);
        return *this;
    }
};

class AsyncTest : public AsyncRoutine {
    typedef AsyncRoutine BaseType;

    StreamSocket<asyncio> _stream{};
    AllocatorType* _allocator{};

    HTTPClient<AppConfig::HTTPConfig, AllocatorType> _client{};

public:
    AsyncTest& operator ()(posix::Socket&& sock, AllocatorType* allocator) {
        _stream.initialize(std::move(sock));
        _allocator = allocator;
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        return *this / _client.initialize(&_stream, _allocator) / &AsyncTest::ready;
    }

    Async ready() {
        _client.write_request_line("GET") << "/?hello-world";
        _client.write_request_field("User-Agent") << "curl/7.81.0";
        return *this / _client.send_request() / &AsyncTest::recv_response;
    }

    Async recv_response() {
        return *this / _client.receive_response() / &AsyncTest::recv_body;
    }

    Async recv_body() {
        auto* _buffer = _client.get_stream().second;
        if (_buffer->size() == 0) {
            return *this / _stream.read(_buffer) / &AsyncTest::recv_body;
        }

#define CONTENT "?hello-world"
#define CONTENT_LENGTH (sizeof(CONTENT) - 1)

        jinx_assert(_buffer->size() == CONTENT_LENGTH);
        jinx_assert(memcmp(_buffer->begin(), CONTENT, CONTENT_LENGTH) == 0);

        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    int fds[2]{-1, -1};
    jinx_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);

    posix::Socket server{fds[0]};
    server.set_non_blocking(true);

    posix::Socket client{fds[1]};
    client.set_non_blocking(true);

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};

    loop.task_new<AsyncTest>(std::move(client), &allocator);
    loop.task_new<AsyncHandshake>(std::move(server), &allocator);

    loop.run();
    return 0;
}
