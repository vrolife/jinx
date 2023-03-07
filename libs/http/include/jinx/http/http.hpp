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
#ifndef __jinx_libs_http_http_hpp__
#define __jinx_libs_http_http_hpp__

#include <cstring>
#include <cctype>
#include <algorithm>
#include <array>

#include <jinx/async.hpp>
#include <jinx/buffer.hpp>
#include <jinx/stream.hpp>
#include <jinx/variant.hpp>
#include <jinx/hash.hpp>
#include <jinx/error.hpp>

namespace jinx {
namespace http {

enum class HTTPStatusCode
{
    Undefined = 0,
    Continue = 100,
    SwitchingProtocols = 101,
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NonAuthoritativeInformation = 203,
    NoContent = 204,
    ResetContent = 205,
    PartialContent = 206,
    MultipleChoices = 300,
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303,
    NotModified = 304,
    UseProxy = 305,
    TemporaryRedirect = 307,
    BadRequest = 400,
    Unauthorized = 401,
    PaymentRequired = 402,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    NotAcceptable = 406,
    ProxyAuthenticationRequired = 407,
    RequestTimeOut = 408,
    Conflict = 409,
    Gone = 410,
    LengthRequired = 411,
    PreconditionFailed = 412,
    RequestEntityTooLarge = 413,
    RequestUriTooLarge = 414,
    UnsupportedMediaType = 415,
    RequestedRangeNotSatisfiable = 416,
    ExpectationFailed = 417,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
    GatewayTimeOut = 504,
    HttpVersionNotSupported = 505
};

JINX_ERROR_DEFINE(http, HTTPStatusCode);

enum class HTTPBuilderStatus {
    NoError = 0,
    RequestEntityTooLarge,
    ResponseEntityTooLarge
};

JINX_ERROR_DEFINE(http_builder, HTTPBuilderStatus);

enum class HTTPParserState {
    Uninitialized = 0,
    EndOfStream,
    EntityTooLarge,
    BadRequest,
    BadResponse = BadRequest,
    StartLine,
    Header,
    Complete
};

enum class HTTPConnectionState {
    Unknown = 0,
    Close,
    KeepAlive,
    Upgrade
};

class HTTPBuilder : 
    public AsyncRoutine,
    protected std::streambuf
{
protected:
    stream::Stream* _stream{nullptr};
    buffer::BufferView* _buffer{};
    std::ostream _output_stream;

    unsigned int _overflow:1;
    unsigned int _detect_connection:1;
    unsigned int _use_custom_server_name:1;
    unsigned int _use_custom_user_agent:1;

    HTTPConnectionState _connection_state{HTTPConnectionState::Unknown};
    SliceConst _connection_string{};

public:
    HTTPBuilder() 
    : _output_stream(this), _overflow(0), _detect_connection(0), _use_custom_server_name(0), _use_custom_user_agent(0)
    { }

    template<typename T>
    typename std::enable_if<not std::is_same<typename std::decay<T>::type, SliceConst>::value, HTTPBuilder&>::
    type operator <<(T&& val) {
        this->_output_stream << std::forward<T>(val);
        return *this;
    }

    HTTPBuilder& operator <<(const SliceConst& blob) {
        this->_output_stream.write(blob.begin(), blob.size());
        return *this;
    }

    HTTPConnectionState connection_state() const noexcept {
        return _connection_state;
    }

    class HeaderLine {
        HTTPBuilder* _builder;

    public:
        explicit HeaderLine(HTTPBuilder* builder)
        :_builder(builder)
        { }

        JINX_NO_COPY(HeaderLine);

        HeaderLine(HeaderLine&& other) noexcept {
            _builder = other._builder;
            other._builder = nullptr;
        }

        HeaderLine& operator=(HeaderLine&& other) = delete;

        ~HeaderLine() {
            if (_builder != nullptr) {
                _builder->field_end();
                *_builder << "\r\n";
            }
            _builder = nullptr;
        }

        template<typename T>
        HeaderLine operator <<(T&& str) {
            *_builder << std::forward<T>(str);
            return std::move(*this);
        }
    };
    
protected:
    void async_finalize() noexcept override {
        _stream = nullptr;
        _overflow = 0;
        _detect_connection = 0;
        _use_custom_server_name = 0;
        _use_custom_user_agent = 0;
        _connection_state = HTTPConnectionState::Unknown;
        _buffer = nullptr;
        AsyncRoutine::async_finalize();
    }

    HeaderLine write_header_field(const SliceConst& name) {
        *this << name << ": ";
        this->field_begin(name);
        return HeaderLine{this};
    }

    template<size_t N>
    HeaderLine write_header_field(const char(&name_array)[N]) {
        SliceConst name{&name_array[0], N-1};
        *this << name << ": ";
        this->field_begin(name);
        return HeaderLine{this};
    }

    ResultGeneric write_header_done() {
        this->_output_stream << "\r\n";
        return _overflow == 0 ? Successful_ : Failed_;
    }

    int_type overflow(int_type cha) override {
        if (cha != std::streambuf::traits_type::eof()) {
            auto dst = _buffer->slice_for_producer();
            if (dst.size() == 0) {
                _overflow = 1;
                return std::streambuf::traits_type::eof();
            }
            *dst.begin() = static_cast<char>(cha);
            if (JINX_UNLIKELY(_buffer->commit(1).is(Failed_))) {
                error::fatal("HTTPBuilder memory overflow");
            }
            return cha;
        }
        return cha;
    }

    std::streamsize xsputn(const char* str, std::streamsize n) override
    {
        auto dst = _buffer->slice_for_producer();

        auto size = std::min(n, static_cast<std::streamsize>(dst.size()));

        memcpy(dst.begin(), str, size);

        if (JINX_UNLIKELY(_buffer->commit(size).is(Failed_))) {
            error::fatal("HTTPBuilder memory overflow");
        }

        if (size < n) {
            _overflow = 1;
        }

        return size;
    }

    void field_begin(const SliceConst& name) {
        if (JINX_UNLIKELY(name.size() == 10 and strncasecmp(name.begin(), "connection", 10) == 0)) {
            _detect_connection = 1;
            _connection_string._data = this->current_position();
        } if (JINX_UNLIKELY(name.size() == 6 and strncasecmp(name.begin(), "server", 6) == 0)) {
            _use_custom_server_name = 1;
        } if (JINX_UNLIKELY(name.size() == 10 and strncasecmp(name.begin(), "user-agent", 10) == 0)) {
            _use_custom_user_agent = 1;
        }
    }

    void field_end() {
        if (JINX_UNLIKELY(_detect_connection)) {
            _connection_string._size = this->current_position() - _connection_string.begin();
            _detect_connection = 0;

            auto hash = hash::hash_data(_connection_string.begin(), _connection_string.end());

            using namespace ::jinx::hash;

            if (hash == "close"_hash) {
                _connection_state = HTTPConnectionState::Close;

            } else if (hash == "keep-alive"_hash) {
                _connection_state = HTTPConnectionState::KeepAlive;

            } else if (hash == "upgrade"_hash) {
                _connection_state = HTTPConnectionState::Upgrade;

            } else {
                _connection_state = HTTPConnectionState::Unknown;
            }
        }
    }

    const char* current_position() {
        return _buffer->end();
    }

    Async _start_transfer() {
        if (_buffer->size() == 0) {
            return async_return();
        }
        return this->async_await(_stream->write(_buffer), &HTTPBuilder::_start_transfer);
    }

public:
    void initialize(stream::Stream* stream, buffer::BufferView* view)
    {
        _stream = stream;
        _buffer = view;
    }

    Awaitable& send() {
        this->async_start(&HTTPBuilder::_start_transfer);
        return *this;
    }
};

struct HTTPVersionDefault { };
struct HTTPVersionCustom {
    const char* _version;
    explicit HTTPVersionCustom(const char* version) : _version(version) { }
};

struct HTTPEndOfHeader {};

class HTTPBuilderRequest 
: public HTTPBuilder
{
    typedef HTTPBuilder BaseType;

public:
    template<typename V>
    class RequestLine {
        HTTPBuilderRequest* _builder;
        V _version{};
    public:

        template<typename T>
        explicit RequestLine(HTTPBuilderRequest* builder, T&& version)
        :_builder(builder), _version(std::forward<T>(version))
        { }

        JINX_NO_COPY(RequestLine);

        RequestLine(RequestLine&& other) noexcept {
            _builder = other._builder;
            _version = std::move(other._version);
            other._builder = nullptr;
        }

        RequestLine& operator=(RequestLine&& other) = delete;

        ~RequestLine() {
            if (_builder != nullptr) {
                *_builder << ' ' << _version << "\r\n";
            }
            _builder = nullptr;
        }

        template<typename T>
        RequestLine<V> operator <<(T&& str) {
            *_builder << std::forward<T>(str);
            return std::move(*this);
        }
    };

    template<typename M, typename V=const char*>
    RequestLine<V> write_request_line(M&& method, V&& version="HTTP/1.1") {
        *this << std::forward<M>(method) << ' ';
        return RequestLine<V>{this, std::forward<V>(version)};
    }

    using BaseType::write_header_field;

    ResultGeneric write_header_done() {
        if (not _use_custom_user_agent) {
            this->_output_stream << "User-Agent: jinx\r\n";
        }
        return BaseType::write_header_done();
    }
};

class HTTPBuilderResponse 
: public HTTPBuilder
{
    typedef HTTPBuilder BaseType;
public:

    class ResponseLine {
        HTTPBuilderResponse* _builder;

    public:
        explicit ResponseLine(HTTPBuilderResponse* builder)
        :_builder(builder)
        { }

        JINX_NO_COPY(ResponseLine);

        ResponseLine(ResponseLine&& other) noexcept {
            _builder = other._builder;
            other._builder = nullptr;
        }

        ResponseLine& operator=(ResponseLine&& other) = delete;

        ~ResponseLine() {
            if (_builder != nullptr) {
                *_builder << "\r\n";
            }
            _builder = nullptr;
        }

        template<typename T>
        ResponseLine operator <<(T&& str) {
            *_builder << std::forward<T>(str);
            return std::move(*this);
        }
    };

    template<typename V=const char*>
    ResponseLine write_response_line(unsigned int status, V&& version="HTTP/1.1") {
        *this << version << ' ' << status << ' ';
        return ResponseLine{this};
    }

    using BaseType::write_header_field;

    ResultGeneric write_header_done() {
        if (not _use_custom_server_name) {
            this->_output_stream << "Server: jinx\r\n";
        }
        return BaseType::write_header_done();
    }
};

class HTTPParser 
: public AsyncRoutinePausable, public MixinResult<HTTPParserState>
{
    typedef AsyncRoutinePausable BaseType;

    buffer::BufferView* _buffer{};
    
    stream::Stream* _stream{nullptr};

    HTTPParserState _state{HTTPParserState::Uninitialized};

    // start line
    SliceConst _start_line_first{};
    SliceConst _start_line_second{};
    SliceConst _start_line_third{};

    SliceConst _field_name{};
    SliceConst _field_value{};

public:
    HTTPParser() = default;

    void initialize(stream::Stream* stream, buffer::BufferView* view) 
    {
        async_finalize();
        this->reset();
        
        _state = HTTPParserState::StartLine;
        _stream = stream;
        _buffer = view;

        if (view->size() == 0) {
            this->async_start(&HTTPParser::read_more);
        } else {
            this->async_start(&HTTPParser::parse);
        }
    }

    stream::Stream* stream() noexcept {
        return _stream;
    }

    buffer::BufferView* buffer() noexcept {
        return _buffer;
    }

    const SliceConst& name() noexcept {
        return _field_name;
    }

    const SliceConst& value() noexcept {
        return _field_value;
    }

protected:
    inline
    const SliceConst& start_line_first_part() const noexcept {
        return _start_line_first;
    }
    
    inline
    const SliceConst& start_line_second_part() const noexcept {
        return _start_line_second;
    }

    inline
    const SliceConst& start_line_third_part() const noexcept {
        return _start_line_third;
    }

    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
            << "HTTPParser{ state: " << static_cast<int>(_state) << " }" << std::endl;
        BaseType::print_backtrace(output_stream, indent + 1);
    }

    Async handle_error(const error::Error& error) override {
        auto state = BaseType::handle_error(error);
        if (state != ControlState::Raise) {
            return state;
        }

        if (error.category() == stream::category_stream()) {
            switch(static_cast<stream::ErrorStream>(error.value())) {
                case jinx::stream::ErrorStream::EndOfStream:
                    _state = HTTPParserState::EndOfStream;
                    this->emplace_result(HTTPParserState::EndOfStream);
                    return this->nop();
                default:
                    return async_throw(error);
            }
        }
        return state;
    }

    void async_finalize() noexcept override {
        _state = HTTPParserState::Uninitialized;
        _field_name = { nullptr, 0 };
        _field_value = { nullptr, 0 };
        BaseType::async_finalize();
    }

    Async read_more() {
        if (_buffer->capacity() == 0) {
            this->emplace_result(HTTPParserState::EntityTooLarge);
            return this->async_return();
        }
        return this->async_await(_stream->read(_buffer), &HTTPParser::parse);
    }

    Async parse() {
        switch(_state) {
            case HTTPParserState::StartLine:
                return parse_start_line();
            case HTTPParserState::Header:
                return parse_header();
            case HTTPParserState::Uninitialized:
                return async_throw(ErrorAwaitable::Uninitialized);
            default:
                return this->async_return();
        }
    }

    // optimistic parser
    Async parse_start_line() {
        // split in space
        char* begin_of_first_part = _buffer->begin();
        char* end_of_first_part = std::find(_buffer->begin(), _buffer->end(), ' ');
        
        char* begin_of_second_part = std::find_if_not(end_of_first_part, _buffer->end(), [](char cha) { return cha == ' '; });
        char* end_of_second_part = std::find(begin_of_second_part, _buffer->end(), ' ');

        char* begin_of_third_part = std::find_if_not(end_of_second_part, _buffer->end(), [](char cha) { return cha == ' '; });
        char* end_of_third_part = std::find(begin_of_third_part, _buffer->end(), '\r');

        constexpr const size_t Prefetch = 3; // CR + LF + one-byte-of-first-field

        if (JINX_UNLIKELY((end_of_third_part + Prefetch) > _buffer->end())) {
            return read_more();
        }

        char* lf_char = end_of_third_part + 1;

        if (*lf_char != '\n') {
            // bare CR detected
            // https://www.rfc-editor.org/rfc/rfc9112#name-message-parsing
            this->emplace_result(HTTPParserState::BadRequest);
            return async_return();
        }

        // consume CRLF
        char* end = lf_char + 1;

        if (std::isspace(*end) != 0) {
            // whitespace between the start-line and the first header field
            // https://www.rfc-editor.org/rfc/rfc9112#name-message-parsing
            this->emplace_result(HTTPParserState::BadRequest);
            return async_return();
        }

        auto start_line_length = end - _buffer->begin(); // include CRLF
        if (JINX_UNLIKELY(_buffer->consume(start_line_length).is(Failed_))) {
            error::fatal("HTTP parser memory overflow");
        }
        
        _start_line_first = { begin_of_first_part, static_cast<size_t>(end_of_first_part - begin_of_first_part) };
        _start_line_second = { begin_of_second_part, static_cast<size_t>(end_of_second_part - begin_of_second_part) };
        _start_line_third = { begin_of_third_part, static_cast<size_t>(end_of_third_part - begin_of_third_part) };

        if (_start_line_first.size() >= 2 and memcmp(_start_line_first.data(), "\r\n", 2) == 0) 
        {
            // https://www.rfc-editor.org/rfc/rfc7230.html#section-3.5
            _start_line_first._data = _start_line_first.begin() + 2;
            _start_line_first._size -= 2;
        }

        this->emplace_result(HTTPParserState::StartLine);
        _state = HTTPParserState::Header;
        return async_pause();
    }

    Async parse_header() { // NOLINT
        if (JINX_UNLIKELY(_buffer->size() < 2)) {
            return read_more();
        }

        char* begin_of_field_name = _buffer->begin();

        if (JINX_UNLIKELY(begin_of_field_name[0] == '\r' and begin_of_field_name[1] == '\n')) {
            // end of header
            if (JINX_UNLIKELY(_buffer->consume(2).is(Failed_))) {
                error::fatal("HTTP parser memory overflow");
            }

            // cut buffer view to protect header content
            auto header_size = reinterpret_cast<uintptr_t>(_buffer->begin()) - reinterpret_cast<uintptr_t>(_buffer->memory());
            if(JINX_UNLIKELY(_buffer->cut(header_size).is(Failed_))) {
                error::fatal("HTTP parser memory overflow");
            }

            this->emplace_result(HTTPParserState::Complete);
            return async_return();
        }

        char* end_of_field_name = std::find(_buffer->begin(), _buffer->end(), ':');

        char* begin_of_value = end_of_field_name;
        char* end_of_value = begin_of_value;

        char* end = nullptr;

        for(;;) {
            end_of_value = std::find(end_of_value, _buffer->end(), '\r');

            char* next_char = end_of_value + 1;

            if (JINX_UNLIKELY(end_of_value == _buffer->end() or next_char == _buffer->end())) {
                return read_more();
            }

            if (*next_char != '\n') {
                // bare CR detected
                // https://www.rfc-editor.org/rfc/rfc9112#name-message-parsing
                this->emplace_result(HTTPParserState::BadRequest);
                return async_return();
            }

            // consume CRLF
            end = next_char + 1;

            // for line-folding
            if (end == _buffer->end()) {
                return read_more();
            }

            // https://www.rfc-editor.org/rfc/rfc9112#name-obsolete-line-folding
            if (*end == ' ' or *end == '\t') {
                end_of_value[0] = ' ';
                end_of_value[1] = ' ';
                end_of_value[2] = ' ';
                end_of_value = end + 1;
                continue;
            }
            break;
        }
        
        // skip colon
        if (end_of_value > begin_of_value) {
            begin_of_value += 1;
        }

        // skip leading whitespace
        while (begin_of_value != end_of_value) {
            if (std::isspace(*begin_of_value) != 0) {
                begin_of_value += 1;
            } else {
                break;
            }
        }

        // skip tailing whitespace
        while (end_of_value > begin_of_value) {
            if (std::isspace((end_of_value - 1)[0]) != 0) {
                end_of_value -= 1;
            } else {
                break;
            }
        }      

        auto header_length = end - _buffer->begin();
        if (JINX_UNLIKELY(_buffer->consume(header_length).is(Failed_))) {
            error::fatal("HTTP parser memory overflow");
        }
        
        _field_name = { begin_of_field_name, static_cast<size_t>(end_of_field_name - begin_of_field_name) };
        _field_value = { begin_of_value, static_cast<size_t>(end_of_value - begin_of_value) };

        if (_field_name.size() > 0 and std::isspace((_field_name.end() - 1)[0]) != 0) {
            // whitespace between the field name and colon
            // https://www.rfc-editor.org/rfc/rfc9112#name-field-line-parsing
            this->emplace_result(HTTPParserState::BadRequest);
            return async_return();
        }
        
        this->emplace_result(HTTPParserState::Header);
        return this->async_pause();
    }

    Async nop() {
        this->async_start(&HTTPParser::nop);
        return this->async_return();
    }
};

struct HTTPParserResponse : public HTTPParser
{
    const SliceConst& version() const noexcept {
        return this->start_line_first_part();
    }
    
    const SliceConst& status() const noexcept {
        return this->start_line_second_part();
    }

    const SliceConst& reason() const noexcept {
        return this->start_line_third_part();
    }
};

struct HTTPParserRequest : public HTTPParser
{
    const SliceConst& method() const noexcept {
        return this->start_line_first_part();
    }
    
    const SliceConst& path() const noexcept {
        return this->start_line_second_part();
    }

    const SliceConst& version() const noexcept {
        return this->start_line_third_part();
    }
};

struct BufferConfigHTTPDefault
{
    constexpr static char const* Name = "HTTP";
    static constexpr const size_t Size = 0x2000; // 8 KBytes
    static constexpr const size_t Reserve = 100;
    static constexpr const long Limit = -1;
    
    struct Information { };
};

struct HTTPConfigDefault {
    typedef BufferConfigHTTPDefault BufferConfig;

    constexpr static const bool WaitBuffer = false;
};

} // namespace http
} // namespace jinx

#endif
