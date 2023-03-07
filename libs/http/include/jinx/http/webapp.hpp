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
#ifndef __jinx_libs_http_webapp_hpp__
#define __jinx_libs_http_webapp_hpp__

#include <algorithm>
#include <tuple>

#include <jinx/assert.hpp>
#include <jinx/async.hpp>
#include <jinx/hash.hpp>
#include <jinx/slice.hpp>
#include <jinx/pointer.hpp>
#include <jinx/http/http.hpp>

namespace jinx {
namespace http {

class WebPage;

struct AppInterface {
    HTTPParserRequest _parser{};
    HTTPBuilderResponse _builder{};
    HTTPConnectionState _connection_state{HTTPConnectionState::Close};
    
    std::pair<uintptr_t, SliceConst>* _slots{};
    size_t _slot_count{};

    SliceConst _query_string{};

    HTTPStatusCode _error_code{};
    void* _app_data{};
    uint32_t _flag_broken_stream:1;

    virtual void redirect(const SliceConst& path) = 0;
};

typedef WebPage* (*SpawnPage)(buffer::BufferView& view, AppInterface* app_interface);

namespace detail {

template<HTTPStatusCode StatusCode>
class ErrorPage;

template<typename RootNode>
struct GetErrorPage {
    template<typename Root, typename ErrorPage=typename Root::ErrorPage>
    static
    ErrorPage find_error_page(int ignored);

    template<typename Root>
    static
    ErrorPage<HTTPStatusCode::Undefined> find_error_page(...);

    typedef decltype(find_error_page<RootNode>(0)) type;
};

// calculate buffer size
constexpr static size_t max(size_t num1, size_t num2) { return num1 > num2 ? num1 : num2; }

template<typename... ChildNodes>
struct CalculateBufferSizeChildNodes;

template<typename Node, typename... Others>
struct CalculateBufferSizeNode;

template<>
struct CalculateBufferSizeNode<void> {
    constexpr static const size_t Size = 0;
    constexpr static const size_t Align = 0;
};

template<typename Node, typename... Others>
struct CalculateBufferSizeNode : CalculateBufferSizeNode<Others...> {
    constexpr static const size_t Size = max(
        max(CalculateBufferSizeNode<Others...>::Size, sizeof(typename Node::Index)), 
        CalculateBufferSizeChildNodes<typename Node::ChildNodes>::Size
    );
    constexpr static const size_t Align = max(
        max(CalculateBufferSizeNode<Others...>::Align, alignof(typename Node::Index)), 
        CalculateBufferSizeChildNodes<typename Node::ChildNodes>::Align
    );
};

template<typename... ChildNodes>
struct CalculateBufferSizeChildNodes<std::tuple<ChildNodes...>> 
: CalculateBufferSizeNode<ChildNodes..., void> 
{ };

template<typename RootNode>
struct CalculateBufferSize {
    constexpr static const size_t Size = 
        sizeof(typename GetErrorPage<RootNode>::type) +
        CalculateBufferSizeChildNodes<typename RootNode::ChildNodes>::Size + 
        CalculateBufferSizeChildNodes<typename RootNode::ChildNodes>::Align;
};

// calculate slot count
template<typename... ChildNodes>
struct CalculateSlotCountChildNodes;

template<typename Node, typename... Others>
struct CalculateSlotCountNode;

template<>
struct CalculateSlotCountNode<void> {
    constexpr static const size_t Depth = 0;
    constexpr static const size_t Count = 0;
};

template<typename Node, typename... Others>
struct CalculateSlotCountNode : CalculateSlotCountNode<Others...> {
    constexpr static const size_t Depth = CalculateSlotCountChildNodes<typename Node::ChildNodes>::Depth + int(Node::Name[0] == ':');
    constexpr static const size_t Count = max(Depth, CalculateSlotCountNode<Others...>::Depth);
};

template<typename... ChildNodes>
struct CalculateSlotCountChildNodes<std::tuple<ChildNodes...>> 
: CalculateSlotCountNode<ChildNodes..., void> 
{ };

// route

template<typename WebApp, typename Node, size_t Slot>
struct Folder;

template<typename WebRoute, size_t Slot, typename Node, typename... Others>
struct ParseNode;

template<typename WebRoute, size_t Slot>
struct ParseNode<WebRoute, Slot, void> {
    static SpawnPage parse(WebRoute& context, const char* begin, const char* end) {
        return nullptr;
    }
};

template<typename WebRoute, size_t Slot, typename Node, typename... Others>
struct ParseNode : ParseNode<WebRoute, Slot, Others...> {
    inline
    static SpawnPage parse(WebRoute& route, const char* begin, const char* end) 
    {
        jinx_assert(begin != end);
        jinx_assert(begin[0] != '/');

        auto* next = std::find_if(begin, end, [](char cha){
            return cha == '/' or cha == '?' or cha == '#';
        });

        if (Node::Name[0] == ':') {
            route.set_slot(Slot, hash::hash_string(Node::Name+1), SliceConst{ begin, static_cast<size_t>(next - begin) });
            return Folder<WebRoute, Node, Slot + 1>::parse(route, next, end);
        }

        if (hash::hash_data(begin, next) == hash::hash_string(Node::Name)) {
            return Folder<WebRoute, Node, Slot>::parse(route, next, end);
        }
        return ParseNode<WebRoute, Slot, Others...>::parse(route, begin, end);
    }
};

template<typename WebRoute, size_t Slot, typename... ChildNodes>
struct ParseChildNodes;

template<typename WebRoute, size_t Slot, typename... ChildNodes>
struct ParseChildNodes<WebRoute, Slot, std::tuple<ChildNodes...>> 
: ParseNode<WebRoute, Slot, ChildNodes..., void> 
{ };

template<typename WebRoute, typename Node, size_t Slot>
struct Folder : ParseChildNodes<WebRoute, Slot, typename Node::ChildNodes> {
    inline
    static SpawnPage parse(WebRoute& route, const char* begin, const char* end) 
    {
        if (*begin == '/') {
            begin += 1;
        }

        const bool is_leaf = begin == end or *begin == '?' or *begin == '#';

        if (is_leaf) {
            route.set_query_string({begin, static_cast<size_t>(end - begin)});
            return &WebRoute::template spawn<typename Node::Index, Slot>;
        }
        return ParseChildNodes<WebRoute, Slot, typename Node::ChildNodes>::parse(route, begin, end);
    }
};

} // namespace detail

enum class ErrorWebApp {
    NoError,
    OutOfMemory,
    ResponseHeaderTooLarge
};

JINX_ERROR_DEFINE(webapp, ErrorWebApp);

class WebPage : public AsyncRoutine {
    AppInterface* _interface{};

public:
    WebPage& operator ()(AppInterface* interface) {
        _interface = interface;
        async_start(&WebPage::init);
        return *this;
    }

protected:
    void* get_app_data() {
        return _interface->_app_data;
    }

    JINX_NO_DISCARD
    ResultGeneric get_slot_by_hash(uintptr_t hash, SliceConst& slot) {
        auto* end = _interface->_slots + _interface->_slot_count;
        auto* iter = std::find_if(_interface->_slots, end, [=](const std::pair<uintptr_t, SliceConst>& pair){
            return pair.first == hash;
        });
        if (iter == end) {
            return Failed_;
        }
        slot = iter->second;
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric get_slot_by_index(size_t index, SliceConst& slot) {
        if (index < _interface->_slot_count) {
            slot = _interface->_slots[index].second;
            return Successful_;
        }
        return Failed_;
    }

    SliceConst get_query_string() {
        return _interface->_query_string;
    }

    HTTPStatusCode get_error_code() const noexcept {
        return _interface->_error_code;
    }

    const SliceConst& method() const noexcept {
        return _interface->_parser.method();
    }

    const SliceConst& path() const noexcept {
        return _interface->_parser.path();
    }

    const SliceConst& version() const noexcept {
        return _interface->_parser.version();
    }

    std::pair<stream::Stream*, buffer::BufferView*> get_stream() noexcept {
        return {_interface->_parser.stream(), _interface->_parser.buffer()};
    }

    void async_finalize() noexcept override {
        AsyncRoutine::async_finalize();
    }

    virtual Async http_start_line() {
        if (_interface->_flag_broken_stream) {
            return http_header_done();
        }
        return parse_header();
    }

    virtual void http_header_field(const SliceConst& name, const SliceConst& value) { }

    virtual Async http_header_done() {
        if (_interface->_flag_broken_stream != 0) {
            return http_handle_request();
        }
        return http_handle_request();
    }

    virtual Async http_handle_request() = 0;

    template<typename... TArgs>
    HTTPBuilderResponse::ResponseLine write_response_line(unsigned int status, TArgs&&... args) {
        return _interface->_builder.write_response_line(status, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    HTTPBuilderResponse::HeaderLine write_response_field(TArgs&&... args) {
        return _interface->_builder.write_header_field(std::forward<TArgs>(args)...);
    }

    template<typename T>
    Async send_response(Async(T::*callback)()) {
        if (_interface->_connection_state == HTTPConnectionState::Unknown) {
            _interface->_connection_state = HTTPConnectionState::Close;
        }

        if (_interface->_builder.connection_state() != HTTPConnectionState::Unknown) {
            _interface->_connection_state = _interface->_builder.connection_state();
        }
        
        if (_interface->_builder.write_header_done().is(Failed_)) {
            return this->async_throw(ErrorWebApp::ResponseHeaderTooLarge);
        }

        _interface->_flag_broken_stream = 1;
        return this->async_await(_interface->_builder.send(), callback);
    }

    template<size_t N>
    Async http_redirect(const char(&path)[N]) {
        return http_redirect({&path[0], N - 1});
    }

    Async http_redirect(const SliceConst& path) {
        _interface->redirect(path);
        return this->async_return();
    }

    // TODO Async http_upgrade() { }

private:
    Async init() {
        return http_start_line();
    }

    Async parse_header() {
        return *this / _interface->_parser / &WebPage::handle_parser_result;
    }

    Async handle_parser_result() {
        switch(_interface->_parser.get_result()) {
            case HTTPParserState::BadRequest:
            case HTTPParserState::StartLine:
            case HTTPParserState::EndOfStream:
                return this->async_throw(HTTPStatusCode::BadRequest);
            case HTTPParserState::Uninitialized:
                return this->async_throw(HTTPStatusCode::InternalServerError);
            case HTTPParserState::EntityTooLarge:
                return this->async_throw(HTTPStatusCode::RequestEntityTooLarge);
            case HTTPParserState::Header:
            {
                const auto& name = _interface->_parser.name();
                const auto& value = _interface->_parser.value();

                if (JINX_UNLIKELY(name.size() == 10 and ::strncasecmp(name.begin(), "connection", 10) == 0)) {
                    auto val_hash = jinx::hash::hash_data(value.begin(), value.end());
                    using namespace jinx::hash;

                    if (val_hash == "close"_hash) {
                        _interface->_connection_state = HTTPConnectionState::Close;

                    } else if (val_hash == "keep-alive"_hash) {
                        _interface->_connection_state = HTTPConnectionState::KeepAlive;

                    } else if (val_hash == "upgrade"_hash) {
                        _interface->_connection_state = HTTPConnectionState::Upgrade;
                    } else {
                        _interface->_connection_state = HTTPConnectionState::Unknown;
                    }
                }
                http_header_field(name, value);
                return parse_header();
            }
            case HTTPParserState::Complete:
                return http_header_done();
        }
        return this->async_return();
    }
};

namespace detail {

struct InternalPages {
    static const buffer::BufferView html_400;
    static const buffer::BufferView html_404;
    static const buffer::BufferView html_413;
    static const buffer::BufferView html_500;
};

template<HTTPStatusCode StatusCode>
class ErrorPage : public WebPage {
    buffer::BufferView _buffer{};    
protected:
    Async http_handle_request() override 
    {
        switch(StatusCode == HTTPStatusCode::Undefined ? get_error_code() : StatusCode) {
            case HTTPStatusCode::BadRequest:
                _buffer = InternalPages::html_400;
                break;
            case HTTPStatusCode::NotFound:
                _buffer = InternalPages::html_404;
                break;
            case HTTPStatusCode::RequestEntityTooLarge:
                _buffer = InternalPages::html_413;
                break;
            default:
                _buffer = InternalPages::html_500;
                break;
        }
        auto error = make_error(get_error_code());
        write_response_line(error.value()) << error.message();
        write_response_field("Connection") << "close";
        write_response_field("Content-Type") << "text/html; charset=UTF-8";
        write_response_field("Content-Length") << _buffer.size();
        return send_response(&ErrorPage::send_body);
    }

    Async send_body() {
        auto pair = get_stream();
        return *this / pair.first->write(&_buffer) / &ErrorPage::async_return;
    }
};

template<typename WebConfig>
struct WebRoute {
    typedef typename WebConfig::RootNode RootNode;
    typedef detail::Folder<WebRoute<WebConfig>, RootNode, 0> Route;

    typedef std::array<std::pair<uintptr_t, SliceConst>, 
        detail::CalculateSlotCountChildNodes<typename RootNode::ChildNodes>::Count> SlotArray;

private:
    SlotArray _slots{};
    SliceConst _query_string{};
    bool _update_slot{true};

public:
    WebRoute() = default;

    SpawnPage route(SliceConst URL) {
        return Route::parse(*this, URL.begin(), URL.end());
    }

    void freeze() {
        _update_slot = false;
    }

    void set_slot(size_t index, uintptr_t name_hash, const SliceConst& value) {
        if (_update_slot) {
            _slots[index] = {name_hash, value};
        }
    }

    std::pair<uintptr_t, SliceConst>* get_slots() {
        return _slots.data();
    }

    void set_query_string(const SliceConst& query_string) {
        if (_update_slot) {
            _query_string = query_string;
        }
    }

    SliceConst get_query_string() {
        return _query_string;
    }

    template<typename Index, size_t SlotCount>
    static WebPage* spawn(buffer::BufferView& view, AppInterface* app_interface) {
        auto address = reinterpret_cast<uintptr_t>(view.memory());
        uintptr_t offset = address % alignof(Index);
        if (offset != 0) {
            view.cut(offset).abort_on(Failed_, "buffer overflow");
        }

        jinx_assert(std::tuple_size<SlotArray>::value >= SlotCount && "slot count out of range");

        const auto size = sizeof(Index);
        jinx_assert(view.memory_size() >= size);
        auto* page = new(view.memory()) Index();
        app_interface->_slot_count = SlotCount;
        (*page)(app_interface);
        return page;
    }

    static SpawnPage get_error_page() {
        return spawn<typename GetErrorPage<RootNode>::type, 0>;
    }
};

} // namespace detail

template<typename WebConfig, typename Allocator>
class WebApp : public AsyncRoutine, private AppInterface
{
    typedef AsyncRoutine BaseType;
    typedef typename Allocator::BufferType BufferType; 
    typedef detail::WebRoute<WebConfig> RouteType;

    stream::Stream* _stream{};
    Allocator* _allocator{};

    RouteType _route{};
    SpawnPage _spawn_page{nullptr};

    BufferType _buffer_page{};
    BufferType _buffer_request{};
    BufferType _buffer_response{};

    pointer::PointerAutoDestructor<WebPage> _page{nullptr};

    typename Allocator::Allocate _allocate{};

public:
    WebApp& operator ()(stream::Stream* stream, Allocator* allocator, void* app_data) {
        _stream = stream;
        _allocator = allocator;
        _flag_broken_stream = 1;
        _app_data = app_data;

        if (WebConfig::HTTPConfig::WaitBuffer) {
            async_start(&WebApp::allocate_buffer_no_error);
        } else {
            async_start(&WebApp::allocate_buffer);
        }
        return *this;
    }

private:
    Async allocate_buffer() {
        _buffer_request = _allocator->allocate(typename WebConfig::HTTPConfig::BufferConfig{});
        if (_buffer_request == nullptr) {
            return this->async_throw(ErrorWebApp::OutOfMemory);
        }

        _buffer_response = _allocator->allocate(typename WebConfig::HTTPConfig::BufferConfig{});
        if (_buffer_response == nullptr) {
            return this->async_throw(ErrorWebApp::OutOfMemory);
        }
                
        _buffer_page = _allocator->allocate(typename WebConfig::BufferConfig{});
        if (_buffer_page == nullptr) {
            return this->async_throw(ErrorWebApp::OutOfMemory);
        }
        return init();
    }

    Async allocate_buffer_no_error() {
        if (_buffer_request == nullptr) {
            if (_allocate.empty()) {
                _buffer_request = _allocator->allocate(typename WebConfig::HTTPConfig::BufferConfig{});
                if (_buffer_request == nullptr) {
                    return *this 
                        / _allocate(_allocator, typename WebConfig::HTTPConfig::BufferConfig{}) 
                        / &WebApp::allocate_buffer_no_error;
                }
            } else {
                _buffer_request = _allocate.release();
            }
        }

        if (_buffer_response == nullptr) {
            if (_allocate.empty()) {
                _buffer_response = _allocator->allocate(typename WebConfig::HTTPConfig::BufferConfig{});
                if (_buffer_response == nullptr) {
                    return *this 
                        / _allocate(_allocator, typename WebConfig::HTTPConfig::BufferConfig{}) 
                        / &WebApp::allocate_buffer_no_error;
                }
            } else {
                _buffer_response = _allocate.release();
            }
        }

        if (_buffer_page == nullptr) {
            if (_allocate.empty()) {
                _buffer_page = _allocator->allocate(typename WebConfig::BufferConfig{});
                if (_buffer_page == nullptr) {
                    return *this 
                        / _allocate(_allocator, typename WebConfig::BufferConfig{}) 
                        / &WebApp::allocate_page_buffer;
                }
            } else {
                _buffer_page = _allocate.release();
            }
        }

        jinx_assert(_buffer_request != nullptr);
        jinx_assert(_buffer_response != nullptr);
        jinx_assert(_buffer_page != nullptr);
        return init();
    }

    Async init() {
        this->_parser.initialize(_stream, &_buffer_request.get()->view());
        this->_builder.initialize(_stream, &_buffer_response.get()->view());
        return *this / this->_parser / &WebApp::pre_route;
    }

    Async pre_route() {
        _flag_broken_stream = 0;
        if (this->_parser.get_result() != HTTPParserState::StartLine) {
            return send_error_page(HTTPStatusCode::BadRequest);
        }

        _spawn_page = route(this->_parser.method(), this->_parser.path(), this->_parser.version());
        if (_spawn_page == nullptr) {
            _error_code = HTTPStatusCode::NotFound;
            _spawn_page = RouteType::get_error_page();
        }
        _slots = _route.get_slots();
        _query_string = _route.get_query_string();
        _route.freeze();
        return spawn_page();
    }

    Async allocate_page_buffer() {
        _buffer_page = _allocate.release();
        return spawn_page();
    }

    Async spawn_page() {
        jinx_assert(_buffer_page != nullptr);
        _buffer_page->reset_full();
        _page.reset(_spawn_page(_buffer_page.get()->view(), static_cast<AppInterface*>(this)));
        _spawn_page = nullptr;
        return *this / *_page / &WebApp::finish;
    }

    void redirect(const SliceConst& path) override {
        _spawn_page = route(this->_parser.method(), path, this->_parser.version());
    }

    void reset() noexcept {
        _spawn_page = nullptr;
        _page.reset();
        _buffer_page.reset();
        _buffer_request.reset();
        _buffer_response.reset();
    }

protected:
    Async send_error_page(HTTPStatusCode status_code) {
        if (_flag_broken_stream != 0) {
            return *this / _stream->shutdown() / &WebApp::async_return;
        }
        _flag_broken_stream = 1;
        _error_code = status_code;
        _spawn_page = RouteType::get_error_page();
        return spawn_page();
    }

    Async handle_error(const error::Error& error) override {
        auto state = BaseType::handle_error(error);
        if (state != ControlState::Raise) {
            return state;
        }

        if (error.category() == category_webapp()) {
            switch(static_cast<ErrorWebApp>(error.value())) {
                case ErrorWebApp::NoError:
                case ErrorWebApp::OutOfMemory:
                    return *this / _stream->shutdown() / &WebApp::async_return;
                case ErrorWebApp::ResponseHeaderTooLarge:
                    break;
            }
        } else if (error.category() == category_http()) {
            if (error.value() >= 400) {
                return send_error_page(static_cast<HTTPStatusCode>(error.value()));
            }
        }
        return send_error_page(HTTPStatusCode::InternalServerError);
    }

    virtual SpawnPage route(const SliceConst& method, const SliceConst& path, const SliceConst& version) 
    {
        return _route.route(path);
    }

    void async_finalize() noexcept override {
        reset();
        AsyncRoutine::async_finalize();
    }

    Async finish() {
        jinx_assert(_page);
        jinx_assert(_buffer_page != nullptr);

        // redirect
        if (_spawn_page != nullptr) {
            return spawn_page();
        }

        switch (this->_connection_state) {
            case HTTPConnectionState::Unknown:
            case HTTPConnectionState::Upgrade:
            case HTTPConnectionState::Close:
                return *this / _stream->shutdown() / &WebApp::async_return;
            case HTTPConnectionState::KeepAlive:
                reset();
                return init();
        }
        return this->async_return();
    }
};

typedef detail::ErrorPage<HTTPStatusCode::NotFound> ErrorPageNotFound;

template<typename Root>
struct WebConfig {
    typedef Root RootNode;

    struct BufferConfig {
        constexpr static char const* Name = "PageBuffer";
        static constexpr const size_t Size = detail::CalculateBufferSize<Root>::Size;
        static constexpr const size_t Reserve = 100;
        static constexpr const long Limit = -1;
        struct Information { };
        static_assert(Size > 0, "");
    };

    typedef HTTPConfigDefault HTTPConfig;
};

} // namespace http
} // namespace jinx

#endif
