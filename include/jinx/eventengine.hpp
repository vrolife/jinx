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
#ifndef __jinx_eventbackend_hpp__
#define __jinx_eventbackend_hpp__

#include <cstdint>

#include <jinx/slice.hpp>
#include <jinx/error.hpp>
#include <jinx/assert.hpp>

namespace jinx {

class Awaitable;

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
    EventEngineAbstract() = default;
    ~EventEngineAbstract() = default;
    JINX_NO_COPY_NO_MOVE(EventEngineAbstract);

    template<typename T, typename A>
    static
    inline
    T* get_event_engine(A* awaitable) noexcept {
        auto* event_engine = awaitable->_task->_task_queue->get_event_engine();
        jinx_assert(event_engine->get_type_tag() == T:: type_tag());
        return static_cast<T*>(event_engine);
    }

    virtual void wakeup() { }
    virtual void poll() { }
    virtual const void* get_type_tag() = 0;
};

} // namespace jinx

#endif
