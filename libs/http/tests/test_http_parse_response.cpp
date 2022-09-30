#include <jinx/async.hpp>
#include <jinx/streamsocket.hpp>
#include <jinx/posix.hpp>
#include <jinx/libevent.hpp>
#include <jinx/http/http.hpp>

using namespace jinx;
using namespace jinx::stream;
using namespace jinx::buffer;
using namespace jinx::http;

typedef AsyncEngine<libevent::EventEngineLibevent> async;
typedef posix::AsyncIOPosix<libevent::EventEngineLibevent> asyncio;

struct HTTPConfig {
    struct BufferConfig 
    {
        constexpr static char const* Name = "HTTP";
        static constexpr const size_t Size = 512;
        static constexpr const size_t Reserve = 100;
        static constexpr const long Limit = -1;
        
        struct Information { };
    };

    constexpr static const bool WaitBuffer = true;
};

typedef BufferAllocator<posix::MemoryProvider, typename HTTPConfig::BufferConfig> AllocatorType;
typedef AllocatorType::BufferType BufferType;

class AsyncTest : public AsyncRoutine {
    typedef StreamSocket<asyncio> StreamType;
    typedef HTTPParserResponse HTTPResponseType;

    int _rfd;
    int _wfd;
    AllocatorType* _allocator{nullptr};
    BufferType _buffer{};
    StreamType _client_stream{};
    HTTPResponseType _response{};
    asyncio::Send _send{};

public:
    AsyncTest& operator ()(int rfd, int wfd, AllocatorType* allocator)
    {
        _rfd = rfd;
        _wfd = wfd;
        _allocator = allocator;
        _client_stream.initialize(posix::Socket{_rfd});
        async_start(&AsyncTest::write_multi_line_value);
        return *this;
    }

    // multi line value
    Async write_multi_line_value() {
        const char* http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Server: jinx\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "Test: foo"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"  
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" 
            "\r\n"
            " bar\r\n"
            "\r\n";

        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(
            _send(_wfd, SliceConst{http_response, strlen(http_response)}, 0), 
            &AsyncTest::parse_multi_line_value);
    }

    Async parse_multi_line_value() {
        return this->async_await(_response, &AsyncTest::check_multi_line_value);
    }

    Async check_multi_line_value() {
        switch(_response.get_result()) {
            case HTTPParserState::Header:
            {
                std::string key{_response.name().begin(), _response.name().end()};
                std::string value{_response.value().begin(), _response.value().end()};
                if (key == "Test") {
                    jinx_assert(value == "foo"
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"  
                        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" 
                        "   bar");
                }
            }
                break;
            case HTTPParserState::EntityTooLarge:
            case HTTPParserState::EndOfStream:
            case HTTPParserState::BadRequest:
                jinx_assert(false && "invalid format");
                break;
            case HTTPParserState::Complete:
                return write_SP_before_colon();
            default:
                // ignore
                break;
        }
        return parse_multi_line_value();
    }

    // SP before colon
    Async write_SP_before_colon() {
        const char* http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Test : foo\rbar\r\n"
            "\r\n";
        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(
            _send(_wfd, SliceConst{http_response, strlen(http_response)}, 0), 
            &AsyncTest::parse_SP_before_colon);
    }

    Async parse_SP_before_colon() {
        return this->async_await(_response, &AsyncTest::check_SP_before_colon);
    }

    Async check_SP_before_colon() {
        switch(_response.get_result()) {
            case HTTPParserState::EntityTooLarge:
            case HTTPParserState::EndOfStream:
                jinx_assert(false && "unexpected behavior");
                break;
            case HTTPParserState::BadRequest:
                return write_bare_CR_in_value();
            case HTTPParserState::Complete:
                jinx_assert(false && "unexpected behavior");
            default:
                // ignore
                break;
        }
        return parse_SP_before_colon();
    }
    
    // field value include CR
    Async write_bare_CR_in_value() {
        const char* http_response = 
            "HTTP/1.1 200 OK\r\n"
            "Server: jinx\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "Test: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n"
            "\r\n";
        jinx_assert(strlen(http_response) == 256);
        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(
            _send(_wfd, SliceConst{http_response, strlen(http_response)}, 0), 
            &AsyncTest::parse_bare_CR_in_value);
    }

    Async parse_bare_CR_in_value() {
        return this->async_await(_response, &AsyncTest::check_bare_CR_in_value);
    }

    Async check_bare_CR_in_value() {
        switch(_response.get_result()) {
            case HTTPParserState::Header:
                break;
            case HTTPParserState::EntityTooLarge:
            case HTTPParserState::EndOfStream:
            case HTTPParserState::Complete:
                jinx_assert(false && "invalid format");
                break;
            case HTTPParserState::BadRequest:
                return write_bare_CR_in_reason();
            default:
                // ignore
                break;
        }
        return parse_bare_CR_in_value();
    }

    // reason include CR
    Async write_bare_CR_in_reason() {
        const char* http_response = 
            "HTTP/1.1 200 OK\rOK\r\n"
            "Server: jinx\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "Test: foo\rbar\r\n"
            "\r\n";
        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(
            _send(_wfd, SliceConst{http_response, strlen(http_response)}, 0), 
            &AsyncTest::parse_reason_include_CR);
    }

    Async parse_reason_include_CR() {
        return this->async_await(_response, &AsyncTest::check_reason_include_CR);
    }

    Async check_reason_include_CR() {
        switch(_response.get_result()) {
            case HTTPParserState::StartLine:
            case HTTPParserState::EntityTooLarge:
            case HTTPParserState::EndOfStream:
            case HTTPParserState::Complete:
                jinx_assert(false && "invalid format");
                break;
            case HTTPParserState::BadRequest:
                return write_large_header();
            default:
                // ignore
                break;
        }
        return parse_reason_include_CR();
    }

    // large header
    Async write_large_header() {
        const char* http_response = 
            "HTTP/1.1 200 OKOK\r\n"
            "Server: jinx\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "Test: "
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "\r\n"
            "\r\n";
        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(
            _send(_wfd, SliceConst{http_response, strlen(http_response)}, 0), 
            &AsyncTest::parse_large_header);
    }

    Async parse_large_header() {
        return this->async_await(_response, &AsyncTest::check_large_header);
    }

    Async check_large_header() {
        switch(_response.get_result()) {
            case HTTPParserState::EntityTooLarge:
                return parse_end_of_stream();
            case HTTPParserState::EndOfStream:
            case HTTPParserState::BadRequest:
                jinx_assert(false && "invalid format");
                break;
            case HTTPParserState::Complete:
                jinx_assert(false && "unexpected end-of-header");
            default:
                // ignore
                break;
        }
        return parse_large_header();
    }

    // eof
    Async parse_end_of_stream() {
        char buf[512];
        ::read(_rfd, buf, 512);
        ::close(_wfd);
        _buffer = _allocator->allocate(HTTPConfig::BufferConfig{});
        _response.initialize(&_client_stream, &_buffer.get()->view());
        return this->async_await(_response, &AsyncTest::check_end_of_stream);
    }

    Async check_end_of_stream() {
        switch(_response.get_result()) {
            case HTTPParserState::EndOfStream:
                return finish();
            case HTTPParserState::EntityTooLarge:
            case HTTPParserState::BadRequest:
                jinx_assert(false && "invalid format");
                break;
            case HTTPParserState::Complete:
                jinx_assert(false && "unexpected end-of-header");
            default:
                // ignore
                break;
        }
        return parse_end_of_stream();
    }

    Async finish() {
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

    loop.task_new<AsyncTest>(fd0.release(), fd1.release(), &allocator);

    loop.run();
    return 0;
}
