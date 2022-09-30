/*
Copyright (C) 2022  pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <unordered_map>

#include <jinx/http/http.hpp>
#include <jinx/http/client.hpp>
#include <jinx/http/webapp.hpp>


#define JINX_WEBAPP_BUILTIN_PAGE_PART_1 \
    "<html><head><title>"

#define JINX_WEBAPP_BUILTIN_PAGE_PART_2 \
    "</title></head>" \
    "<body>" \
    "<center><h1>"

#define JINX_WEBAPP_BUILTIN_PAGE_PART_3 \
    "</h1></center><hr><center>jinx</center>" \
    "</body>"\
    "</html>"

namespace jinx {
namespace http {

JINX_ERROR_IMPLEMENT(http, {
    switch(code.value()) {
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Time-out";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Request Entity Too Large";
        case 414: return "Request-URI Too Large";
        case 415: return "Unsupported Media Type";
        case 416: return "Requested range not satisfiable";
        case 417: return "Expectation Failed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Time-out";
        case 505: return "HTTP Version not supported";
        default: break;
    }
    return "Unsupported status code";
});

JINX_ERROR_IMPLEMENT(http_builder, {
    switch(code.as<HTTPBuilderStatus>()) {
        case HTTPBuilderStatus::NoError:
            return "No error";
        case HTTPBuilderStatus::RequestEntityTooLarge:
            return "Request entity too large";
        case HTTPBuilderStatus::ResponseEntityTooLarge:
            return "Response entity too large";
        default: break;
    }
    return "unkonwn error code";
});

JINX_ERROR_IMPLEMENT(http_client, {
    switch(code.as<ErrorHTTPClient>()) {
        case ErrorHTTPClient::NoError:
            return "No error";
        case ErrorHTTPClient::OutOfMemory:
            return "Out of memory";
        case ErrorHTTPClient::RequestHeaderTooLarge:
            return "Request header too large";
        case ErrorHTTPClient::BadResponse:
            return "Bad response";
        case ErrorHTTPClient::EntityTooLarge:
            return "Entity too large";
        case ErrorHTTPClient::EndOfStream:
            return "End of stream";
        default: break;
    }
    return "unkonwn error code";
});

JINX_ERROR_IMPLEMENT(webapp, {
    switch(code.as<ErrorWebApp>()) {
        case ErrorWebApp::NoError:
            return "No error";
        case ErrorWebApp::OutOfMemory:
            return "Out of memory";
        case ErrorWebApp::ResponseHeaderTooLarge:
            return "Response header too large";
        default: break;
    }
    return "unkonwn error code";
});

namespace detail {

#define HTML_400 \
    JINX_WEBAPP_BUILTIN_PAGE_PART_1 \
    "400 Bad Request" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_2 \
    "400 Bad Request" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_3

const buffer::BufferView InternalPages::html_400 = {const_cast<char*>(HTML_400), sizeof(HTML_400) - 1, 0, sizeof(HTML_400) - 1};

#define HTML_404 \
    JINX_WEBAPP_BUILTIN_PAGE_PART_1 \
    "404 Not Found" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_2 \
    "404 Not Found" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_3

const buffer::BufferView InternalPages::html_404 = {const_cast<char*>(HTML_404), sizeof(HTML_404) - 1, 0, sizeof(HTML_404) - 1};

#define HTML_413 \
    JINX_WEBAPP_BUILTIN_PAGE_PART_1 \
    "413 Request Entity Too Large" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_2 \
    "413 Request Entity Too Large" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_3

const buffer::BufferView InternalPages::html_413 = {const_cast<char*>(HTML_413), sizeof(HTML_413) - 1, 0, sizeof(HTML_413) - 1};

#define HTML_500 \
    JINX_WEBAPP_BUILTIN_PAGE_PART_1 \
    "500 Internal Server Error" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_2 \
    "500 Internal Server Error" \
    JINX_WEBAPP_BUILTIN_PAGE_PART_3

const buffer::BufferView InternalPages::html_500 = {const_cast<char*>(HTML_500), sizeof(HTML_500) - 1, 0, sizeof(HTML_500) - 1};

} // namespace detail

} // namespace http
} // namespace jinx
