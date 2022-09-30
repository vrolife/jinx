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
