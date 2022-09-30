#include "static.hpp"

#include "index.h"
#include "app.h"

namespace example {

Async PageIndex::http_handle_request()
{
    _buffer = buffer::BufferView {
        const_cast<char*>(reinterpret_cast<const char*>(INDEX_HTML)),
        INDEX_HTML_LEN,
        0,
        INDEX_HTML_LEN
    };
    write_response_line(200) << "Ok";
    write_response_field("Connection") << "close";
    write_response_field("Content-Type") << "text/html; charset=UTF-8";
    write_response_field("Content-Length") << _buffer.size();
    return send_response(&PageIndex::send_body);
}

Async FileAppJs::http_handle_request()
{
    _buffer = buffer::BufferView {
        const_cast<char*>(reinterpret_cast<const char*>(APP_JS)),
        APP_JS_LEN,
        0,
        APP_JS_LEN
    };
    write_response_line(200) << "Ok";
    write_response_field("Connection") << "close";
    write_response_field("Content-Type") << "application/javascript; charset=utf-8";
    write_response_field("Content-Length") << _buffer.size();
    return send_response(&FileAppJs::send_body);
}

} // namespace example
