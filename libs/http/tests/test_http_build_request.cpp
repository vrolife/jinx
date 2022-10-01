#include <jinx/async.hpp>
#include <jinx/stream.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/libevent.hpp>
#include <jinx/http/http.hpp>

using namespace jinx;
using namespace jinx::stream;
using namespace jinx::buffer;
using namespace jinx::http;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

typedef HTTPConfigDefault HTTPConfig;

typedef BufferAllocator<posix::MemoryProvider, typename HTTPConfig::BufferConfig> AllocatorType;
typedef AllocatorType::BufferType BufferType;

typedef StreamSocket<asyncio> StreamType;
typedef HTTPBuilderRequest HTTPRequestType;

std::string request = 
    "GET /get?uid=12 HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: jinx/0.0.1\r\n"
    "Accept: */*\r\n"
    "\r\n";

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};
    HTTPRequestType _request{};

    auto buffer = allocator.allocate(HTTPConfig::BufferConfig{});

    _request.initialize(nullptr, &buffer.get()->view());

    int uid = 12;
    _request.write_request_line("GET") << "/get?uid=" << uid;
    _request.write_header_field("Host") << "example.com";
    _request.write_header_field("User-Agent") << "jinx/0.0.1";
    _request.write_header_field("Accept") << "*/*";
    jinx_assert(_request.write_header_done().is(Successful_));

    auto header = buffer->slice_for_consumer();
    std::string header_str{header.begin(), header.end()};

    std::cout << header_str;

    jinx_assert(header_str == request);

    loop.run();
    return 0;
}
