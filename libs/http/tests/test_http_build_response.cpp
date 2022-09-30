#include <jinx/assert.hpp>

#include <jinx/async.hpp>
#include <jinx/stream.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/libevent.hpp>
#include <jinx/http/http.hpp>

using namespace jinx;
using namespace jinx::stream;
using namespace jinx::buffer;
using namespace jinx::http;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

typedef HTTPConfigDefault HTTPConfig;

typedef BufferAllocator<posix::MemoryProvider, typename HTTPConfig::BufferConfig> AllocatorType;
typedef AllocatorType::BufferType BufferType;

typedef StreamSocket<asyncio> StreamType;
typedef HTTPBuilderResponse HTTPResponseType;

std::string response = 
    "HTTP/1.1 200 OK\r\n"
    "Server: jinx\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 0\r\n"
    "Connection: close\r\n"
    "\r\n";

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};
    HTTPResponseType _response{};

    auto buffer = allocator.allocate(HTTPConfig::BufferConfig{});

    _response.initialize(nullptr, &buffer.get()->view());

    _response.write_response_line(200) << "OK";
    _response.write_header_field("Server") << "jinx";
    _response.write_header_field("Content-Type") << "text/plain";
    _response.write_header_field("Content-Length") << "0";
    _response.write_header_field("Connection") << "close";
    jinx_assert(_response.write_header_done().is(Successfu1));

    auto header = buffer->slice_for_consumer();
    std::string header_str{header.begin(), header.end()};

    std::cout << header_str;

    jinx_assert(header_str == response);

    loop.run();
    return 0;
}
