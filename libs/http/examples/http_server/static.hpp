#ifndef __static_hpp__
#define __static_hpp__

#include <jinx/http/webapp.hpp>

namespace example {

using namespace jinx;
using namespace jinx::http;

struct PageIndex : WebPage {
    typedef WebPage BaseType;

    buffer::BufferView _buffer{};

    Async http_handle_request() override;

    Async send_body() {
        auto pair = get_stream();
        return *this / pair.first->write(&_buffer) / &PageIndex::async_return;
    }
};

struct FileAppJs : WebPage {
    typedef WebPage BaseType;

    buffer::BufferView _buffer{};

    Async http_handle_request() override;

    Async send_body() {
        auto pair = get_stream();
        return *this / pair.first->write(&_buffer) / &FileAppJs::async_return;
    }
};

} // namespace example

#endif
