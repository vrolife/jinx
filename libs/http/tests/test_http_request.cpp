#include <sys/socket.h>

#include <regex>

#include <jinx/async.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/http/http.hpp>
#include <jinx/posix.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;
using namespace jinx::stream;
using namespace jinx::buffer;
using namespace jinx::http;

typedef AsyncImplement<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

typedef HTTPConfigDefault HTTPConfig;

typedef BufferAllocator<posix::MemoryProvider, typename HTTPConfig::BufferConfig> AllocatorType;
typedef AllocatorType::BufferType BufferType;

const char* http_request =
    "GET /get?uid=12 HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: jinx/0.0.1\r\n"
    "Accept: */*\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";
const char* http_response = 
    "HTTP/1.1 200 OK\r\n"
    "Server: jinx\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 6\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "uid=13";

class HTTPServerTest : public AsyncRoutine {
    int _fd{-1};
    asyncio::Recv _recv{};
    asyncio::Send _send{};
    char _buf[4096];

public:
    HTTPServerTest& operator()(int srv_fd) {
        _fd = srv_fd;
        this->async_start(&HTTPServerTest::read);
        return *this;
    }
protected:
    Async read() {
        return this->async_await(_recv(_fd, SliceMutable{_buf, 4096}, 0), &HTTPServerTest::handle_request);
    }

    Async handle_request() {
        jinx_assert(memcmp(http_request, _buf, strlen(http_request)) == 0);
        return send_response();
    }

    Async send_response() {
        return this->async_await(_send(_fd, SliceConst{http_response, strlen(http_response)}, 0), &HTTPServerTest::finish);
    }

    Async finish() {
        return this->async_return();
    }
};

template<typename StreamType>
class Parser : public AsyncFunction<int> {
    std::regex _regexp{"uid=(\\d+)"};

    StreamType* _stream{nullptr};
    AllocatorType* _allocator{nullptr};
    BufferType _read_buffer{};

public:
    Parser& operator ()(StreamType* stream, AllocatorType* allocator, BufferType& prefetched_buffer)
    {
        this->reset();
        _stream = stream;
        _allocator = allocator;
        _read_buffer = prefetched_buffer;
        this->async_start(&Parser::parse);
        return *this;
    }

    Async read_more() {
        if (_read_buffer == nullptr or _read_buffer->capacity() == 0) {
            _read_buffer = _allocator->allocate(typename HTTPConfig::BufferConfig{});
            jinx_assert(_read_buffer != nullptr);
        }
        return this->async_await(_stream->read(&_read_buffer.get()->view()), &Parser::parse);
    }

    Async parse() {
        auto& buf = _read_buffer;
        if (buf->size() != 6) {
            return read_more();
        }
        std::match_results<char*> match;
        if (std::regex_match(buf->begin(), buf->end(), match, _regexp)) {
            this->emplace_result(std::stoi(match[1].str()));
        } else {
            // LCOV_EXCL_START
            this->emplace_result(-1);
            // LCOV_EXCL_STOP
        }
        return this->async_return();
    }
};

class HTTPClientTest : public AsyncRoutine {
    typedef StreamSocket<asyncio> StreamType;
    typedef HTTPBuilderRequest HTTPRequestType;
    typedef HTTPParserResponse HTTPResponseParserType;

    StreamType _stream{};
    HTTPRequestType _request{};
    HTTPResponseParserType _response{};
    AllocatorType* _allocator{nullptr};
    BufferType _request_buffer{};
    BufferType _response_buffer{};
    int _header_index{0};
    std::string _response_body{};

    Parser<StreamType> _parse{};

public:
    HTTPClientTest& operator ()(int cli_fd, AllocatorType* allocator)
    {
        _stream.initialize(posix::Socket{cli_fd});
        _allocator = allocator;
        this->async_start(&HTTPClientTest::write_header);
        return *this;
    }

    Async write_header() {
        int uid = 12;

        _request_buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        if (_request_buffer == nullptr) {
            return this->async_throw(posix::ErrorPosix::NotEnoughMemory);
        }

        _response_buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        if (_response_buffer == nullptr) {
            return this->async_throw(posix::ErrorPosix::NotEnoughMemory);
        }

        _request.initialize(&_stream, &_request_buffer.get()->view());

        _request.write_request_line("GET") << "/get?uid=" << uid;
        _request.write_header_field("Host") << "example.com";
        _request.write_header_field("User-Agent") << "jinx/0.0.1";
        _request.write_header_field("Accept") << "*/*";
        _request.write_header_field("Connection") << "keep-alive";
        if (_request.write_header_done().is(Failed_)) {
            return this->async_throw(HTTPBuilderStatus::RequestEntityTooLarge);
        }

        _response.initialize(&_stream, &_response_buffer.get()->view());
        return this->async_await(_request.send(), &HTTPClientTest::read_response);
    }

    Async read_response() {
        return this->async_await(_response, &HTTPClientTest::parse_response);
    }

    Async parse_response() { // NOLINT
        switch(_response.get_result()) {
            case HTTPParserState::StartLine:
                jinx_assert(std::string("HTTP/1.1") == std::string(_response.version().begin(), _response.version().end()));
                jinx_assert(std::string("200") == std::string(_response.status().begin(), _response.status().end()));
                jinx_assert(std::string("OK") == std::string(_response.reason().begin(), _response.reason().end()));
                break;
            case HTTPParserState::Header:
                switch(_header_index) {
                    case 0:
                        jinx_assert(std::string("Server") == std::string(_response.name().begin(), _response.name().end()));
                        jinx_assert(std::string("jinx") == std::string(_response.value().begin(), _response.value().end()));
                        break;
                    case 1:
                        jinx_assert(std::string("Content-Type") == std::string(_response.name().begin(), _response.name().end()));
                        jinx_assert(std::string("text/plain") == std::string(_response.value().begin(), _response.value().end()));
                        break;
                    case 2:
                        jinx_assert(std::string("Content-Length") == std::string(_response.name().begin(), _response.name().end()));
                        jinx_assert(std::string("6") == std::string(_response.value().begin(), _response.value().end()));
                        break;
                    case 3:
                        jinx_assert(std::string("Connection") == std::string(_response.name().begin(), _response.name().end()));
                        jinx_assert(std::string("keep-alive") == std::string(_response.value().begin(), _response.value().end()));
                        break;
                }
                _header_index += 1;
                break;
            case HTTPParserState::Complete:
                return read_body();
            case HTTPParserState::BadRequest:
                ::abort();
                break;
            default:
                ::abort();
                return finish();
        }
        return read_response();
    }

    Async read_body() {
        return this->async_await(_parse(&_stream, _allocator, _response_buffer), &HTTPClientTest::finish);
    }

    Async finish() {
        jinx_assert(_parse.get_result() == 13);
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    posix::MemoryProvider memory{};
    AllocatorType allocator{memory};
    allocator.reconfigure<typename HTTPConfig::BufferConfig>(100, 200);

    int fds[2];
    jinx_assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != -1);

    asyncio::IOHandleType fd0(fds[0]);
    asyncio::IOHandleType fd1(fds[1]);
    fd0.set_non_blocking(true);
    fd1.set_non_blocking(true);
    
    loop.task_new<HTTPClientTest>(fd0.release(), &allocator);
    loop.task_new<HTTPServerTest>(fd1.release());

    loop.run();
    return 0;
}
