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
#ifndef __jinx_eventbackend_hpp__
#define __jinx_eventbackend_hpp__

#include <cstdint>

#include <jinx/slice.hpp>
#include <jinx/error.hpp>

namespace jinx {

enum class ErrorEventEngine {
    NoError,
    FailedToRegisterTimerEvent,
    FailedToRegisterIOEvent,
    FailedToRegisterSignalEvent,
    FailedToRemoveTimerEvent,
    FailedToRemoveIOEvent,
    FailedToRemoveSignalEvent,
};

JINX_ERROR_DEFINE(event_engine, ErrorEventEngine);

typedef void(*EventCallback)(const error::Error&, void*);
typedef void(*EventCallbackIO)(unsigned int flags, const error::Error&, void*);

struct IOResult {
    error::Error _error{}; // TODO error condition
    union {
        long _size{0};
        int _fd;
        int _int;
    };

    IOResult() : _size(0) { } // NOLINT
    IOResult(const error::Error& ec, long size)  // NOLINT
    : _error(ec), _size(size) { }

    explicit operator long() const noexcept {
        return _size;
    }

    explicit operator int() const noexcept {
        return _fd;
    }

    void set_result(int result) noexcept {
        _fd = result;
    }

    void set_result(long result) noexcept {
        _size = result;
    }
};

struct EventEngineAbstract
{
    virtual void wakeup() { }
    virtual void poll() { }
    virtual const void* get_type_tag() = 0;
};

} // namespace jinx

#endif
