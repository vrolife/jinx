/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __jinx_libs_http_client_hpp__
#define __jinx_libs_http_client_hpp__

#include <jinx/http/http.hpp>

namespace jinx {
namespace http {

/*
    GetStream
    GetTCP
    GetTLS
*/

enum class ErrorHTTPClient {
    NoError,
    Uninitialized,
    OutOfMemory,
    RequestHeaderTooLarge,
    BadResponse,
    EntityTooLarge,
    EndOfStream
};

JINX_ERROR_DEFINE(http_client, ErrorHTTPClient);

template<typename HTTPConfig, typename Allocator>
class HTTPClient : private AsyncRoutinePausable {
    typedef typename Allocator::BufferType BufferType;

    friend class AsyncRoutinePausable::Pack;
    friend class MixinRoutine<AwaitablePausable>;

    stream::Stream* _stream{};
    Allocator* _allocator{};

    HTTPConnectionState _connection_state{HTTPConnectionState::Unknown};

    BufferType _buffer_request{};
    BufferType _buffer_response{};

    // TODO save memory. Optinal<HTTPBuilderRequest, HTTPParserResponse>
    HTTPBuilderRequest _builder{};
    HTTPParserResponse _parser{};

    typename Allocator::Allocate _allocate{};

public:
    Awaitable& initialize(stream::Stream* stream,  Allocator* allocator) {
        _stream = stream;
        _allocator = allocator;
        if (HTTPConfig::WaitBuffer) {
            async_start(&HTTPClient::allocate_buffer_no_error);
        } else {
            async_start(&HTTPClient::allocate_buffer);
        }
        return *this;
    }

    std::pair<stream::Stream*, buffer::BufferView*> get_stream() noexcept {
        return {_parser.stream(), _parser.buffer()};
    }

    HTTPConnectionState connection_state() const {
        return _connection_state;
    }

    const SliceConst& version() const noexcept {
        return _parser.version();
    }

    const SliceConst& status() const noexcept {
        return _parser.status();
    }

    const SliceConst& reason() const noexcept {
        return _parser.reason();
    }

    template<typename M, typename V=const char*>
    HTTPBuilderRequest::RequestLine<V> write_request_line(M&& method, V&& version="HTTP/1.1") {
        return _builder.write_request_line(std::forward<M>(method), std::forward<V>(version));
    }

    template<typename... TArgs>
    HTTPBuilderRequest::HeaderLine write_request_field(TArgs&&... args) {
        return _builder.write_header_field(std::forward<TArgs>(args)...);
    }

    Awaitable& send_request() {
        if (_connection_state == HTTPConnectionState::Unknown) {
            _connection_state = HTTPConnectionState::Close;
        }

        if (_builder.connection_state() != HTTPConnectionState::Unknown) {
            _connection_state = _builder.connection_state();
        }
        
        if (_builder.write_header_done().is(Failed_)) {
            this->async_throw(ErrorHTTPClient::RequestHeaderTooLarge);
            return *this;
        }
        async_start(&HTTPClient::_send_request);
        return *this;
    }

    Awaitable& receive_response () {
        async_start(&HTTPClient::recv_response);
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        _stream = nullptr;
        _allocator = nullptr;
        _connection_state = HTTPConnectionState::Unknown;
        _buffer_request.reset();
        _buffer_response.reset();
        AsyncRoutinePausable::async_finalize();
    }

    Async allocate_buffer() {
        _buffer_request = _allocator->allocate(typename HTTPConfig::BufferConfig{});
        if (_buffer_request == nullptr) {
            return this->async_throw(ErrorHTTPClient::OutOfMemory);
        }

        _buffer_response = _allocator->allocate(typename HTTPConfig::BufferConfig{});
        if (_buffer_response == nullptr) {
            return this->async_throw(ErrorHTTPClient::OutOfMemory);
        }

        jinx_assert(_buffer_request != nullptr);
        jinx_assert(_buffer_response != nullptr);
        return init();
    }

    Async allocate_buffer_no_error() {
        if (_buffer_request == nullptr) {
            if (_allocate.empty()) {
                _buffer_request = _allocator->allocate(typename HTTPConfig::BufferConfig{});
                if (_buffer_request == nullptr) {
                    return *this 
                        / _allocate(_allocator, typename HTTPConfig::BufferConfig{}) 
                        / &HTTPClient::allocate_buffer_no_error;
                }
            } else {
                _buffer_request = _allocate.release();
            }
        }

        if (_buffer_response == nullptr) {
            if (_allocate.empty()) {
                _buffer_response = _allocator->allocate(typename HTTPConfig::BufferConfig{});
                if (_buffer_response == nullptr) {
                    return *this 
                        / _allocate(_allocator, typename HTTPConfig::BufferConfig{}) 
                        / &HTTPClient::allocate_buffer_no_error;
                }
            } else {
                _buffer_response = _allocate.release();
            }
        }
        jinx_assert(_buffer_request != nullptr);
        jinx_assert(_buffer_response != nullptr);
        return init();
    }

    Async init() {
        _parser.initialize(_stream, &_buffer_response.get()->view());
        _builder.initialize(_stream, &_buffer_request.get()->view());
        return async_pause();
    }

    Async _send_request() {
        return *this / _builder.send() / &HTTPClient::async_pause;
    }

    Async recv_response() {
        return *this / _parser / &HTTPClient::parse_header;
    }

    Async parse_header() {
        switch(_parser.get_result()) {
            case HTTPParserState::Uninitialized:
                return async_throw(ErrorHTTPClient::Uninitialized);
            case HTTPParserState::EntityTooLarge:
                return async_throw(ErrorHTTPClient::EntityTooLarge);
            case HTTPParserState::EndOfStream:
                return async_throw(ErrorHTTPClient::EndOfStream);
            case HTTPParserState::BadResponse:
                return async_throw(ErrorHTTPClient::BadResponse);
            case HTTPParserState::StartLine:
                return http_start_line();
            case HTTPParserState::Header:
            {
                const auto& name = _parser.name();
                const auto& value = _parser.value();

                if (JINX_UNLIKELY(name.size() == 10 and ::strncasecmp(name.begin(), "connection", 10) == 0)) {
                    auto val_hash = jinx::hash::hash_data(value.begin(), value.end());
                    using namespace jinx::hash;

                    if (val_hash == "close"_hash) {
                        _connection_state = HTTPConnectionState::Close;

                    } else if (val_hash == "keep-alive"_hash) {
                        _connection_state = HTTPConnectionState::KeepAlive;

                    } else if (val_hash == "upgrade"_hash) {
                        _connection_state = HTTPConnectionState::Upgrade;
                    } else {
                        _connection_state = HTTPConnectionState::Unknown;
                    }
                }
                http_header_field(name, value);
                return recv_response();
            }
            case HTTPParserState::Complete:
                return async_pause();
            default:
                break;
        }
        return recv_response();
    }

    virtual Async http_start_line() {
        return recv_response();
    }

    virtual void http_header_field(const SliceConst& name, const SliceConst& value) { }
};

} // namespace http
} // namespace jinx

#endif
