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
#ifndef __evdns_hpp__
#define __evdns_hpp__

#include <event2/dns.h>

#include <jinx/async/awaitable.hpp>

namespace jinx {
namespace evdns {

enum class ErrorEvdns {
    NoError,
    Failed
};

JINX_ERROR_DEFINE(evdns, ErrorEvdns);

enum class DNSType {
    A,
    AAAA,
    IPv4 = A,
    IPv6 = AAAA,
};

class AsyncResolve : public Awaitable {
    struct evdns_base* _base{};
    struct evdns_request* _request{};
    char _buffer[16];
    bool _done{false};

public:
    AsyncResolve& operator()(evdns_base* base, const char* name, DNSType type) {
        _base = base;
        _request = nullptr;
        _done = false;
        _request = evdns_base_resolve_ipv4(_base, name, 0, &AsyncResolve::callback, this);
        return *this;
    }

    char* get_address() {
        return _buffer;
    }

protected:
    void async_finalize() noexcept override {
        if (_request != nullptr) {
            evdns_cancel_request(_base, _request);
            _request = nullptr;
        }
        Awaitable::async_finalize();
    }

    Async async_poll() override {
        if (_request == nullptr) {
            return this->async_throw(ErrorEvdns::Failed);
        }
        
        if (_done) {
            return async_return();
        }

        return async_suspend();
    }

private:
    static void callback(int result, char type, int count, int ttl, void* addresses, void* arg) {
        auto* self = reinterpret_cast<AsyncResolve*>(arg);

        if (type == EVDNS_TYPE_A) {
            memcpy(self->_buffer, addresses, 4);
        } else if (type == EVDNS_TYPE_AAAA) {
            memcpy(self->_buffer, addresses, 16);
        } else {
            error::fatal("unsupported address type");
        }

        if (self->_request != nullptr) {
            self->async_resume() >> JINX_IGNORE_RESULT;
        }

        self->_done = true;
    }
};

} // namespace evdns
} // namespace jinx

#endif
