#ifndef __status_hpp__
#define __status_hpp__

#include <jinx/http/webapp.hpp>

#include "appdata.hpp"

namespace example {

using namespace jinx;
using namespace jinx::http;

struct PageStatus : WebPage {
    typedef WebPage BaseType;

    buffer::BufferView _buffer{};
    char _array[100];

    Async http_handle_request() override 
    {
        auto* data = reinterpret_cast<WebAppData*>(get_app_data());

        snprintf(_array, 100, "{\"count\":\"%d\"}", ++data->counter);
        auto size = strlen(_array);

        _buffer = buffer::BufferView {
            _array,
            size,
            0,
            size
        };
        write_response_line(200) << "Ok";
        write_response_field("Connection") << "close";
        write_response_field("Content-Type") << "text/html; charset=UTF-8";
        write_response_field("Content-Length") << _buffer.size();
        return send_response(&PageStatus::send_body);
    }

    Async send_body() {
        auto pair = get_stream();
        return *this / pair.first->write(&_buffer) / &PageStatus::async_return;
    }
};

} // namespace example

#endif
