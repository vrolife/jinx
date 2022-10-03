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
#ifndef __jinx_libevent_hpp__
#define __jinx_libevent_hpp__

#include <unistd.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#include <atomic>
#include <functional>

#include <jinx/macros.hpp>
#include <jinx/optional.hpp>
#include <jinx/eventengine.hpp>
#include <jinx/posix.hpp>

namespace jinx {
namespace libevent {

class EventEngineLibevent : public EventEngineAbstract {
    class EventDataIO;
    class EventDataSimple;
    class EventDataSimpleFunctional;

    class EventRead;
    class EventWrite;

    struct event_base *_base;
    int _poll_flags;
    int _additional_flags{0};

public:
    typedef int IOHandleNativeType;
    typedef ::jinx::posix::IOHandle IOHandleType;

    constexpr static const IOHandleNativeType IOHandleNativeInvalidValue = -1;

    typedef Optional<EventDataSimple> EventHandleTimer;
    typedef Optional<EventDataSimple> EventHandleSignal;
    typedef Optional<EventDataSimpleFunctional> EventHandleSignalFunctional;
    typedef Optional<EventDataIO> EventHandleIO;

    constexpr static const int IOFlagRead = EV_READ; 
    constexpr static const int IOFlagWrite = EV_WRITE; 
    constexpr static const int IOFlagReadWrite = EV_READ | EV_WRITE; 
    constexpr static const int IOFlagPersist = EV_PERSIST;
    constexpr static const int IOFlagEdgeTriggered = EV_ET;

    struct IOTypeRead {
        constexpr static unsigned int Flags = IOFlagRead;
    };

    struct IOTypeWrite {
        constexpr static unsigned int Flags = IOFlagWrite;
    };

    struct IOTypeReadWrite {
        constexpr static unsigned int Flags = IOFlagReadWrite;
    };

    explicit EventEngineLibevent(bool thread_safe);
    JINX_NO_COPY_NO_MOVE(EventEngineLibevent);

    ~EventEngineLibevent();

    static const void* type_tag();
    const void* get_type_tag() override { return type_tag(); }

    struct event_base *get_event_base() {
        return _base;
    }

    // poll
    void poll() override;

    void wakeup() override;

    // EVLOOP_NONBLOCK
    void set_non_blocking(bool enable);

    // EVLOOP_NO_EXIT_ON_EMPTY
    void set_no_exit_on_empty(bool enable);

    static void set_debug(bool enable);

    static
    IOHandleNativeType get_native_handle(IOHandleType& handle) {
        return handle.native_handle();
    }

    static
    IOHandleNativeType get_native_handle(IOHandleNativeType handle) {
        return handle;
    }

    // reactor
    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        Awaitable* awaitable,
        const IOTypeRead& /*unused*/, 
        EventHandleIO& event_handle, 
        IOHandleNativeType io_handle, 
        EventCallbackIO callback, 
        void* arg) 
    { 
        return add_io(awaitable, IOTypeRead::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    inline ResultGeneric add_io(
        const IOTypeRead& /*unused*/, 
        EventHandleIO& event_handle, 
        IOHandleNativeType io_handle, 
        EventCallbackIO callback, 
        void* arg) 
    { 
        return add_io(IOTypeRead::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        Awaitable* awaitable,
        const IOTypeWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg) 
    {
        return add_io(awaitable, IOTypeWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    inline ResultGeneric add_io(
        const IOTypeWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg) 
    {
        return add_io(IOTypeWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    inline ResultGeneric add_io(
        Awaitable* awaitable,
        const IOTypeReadWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg)
    { 
        return add_io(awaitable, IOTypeReadWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    inline ResultGeneric add_io(
        const IOTypeReadWrite& /*unused*/,
        EventHandleIO& event_handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg)
    { 
        return add_io(IOTypeReadWrite::Flags, event_handle, io_handle, callback, arg); 
    }

    JINX_NO_DISCARD
    static
    ResultGeneric add_io(
        Awaitable* awaitable,
        unsigned int flags,
        EventHandleIO& handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg);

    JINX_NO_DISCARD
    ResultGeneric add_io(
        unsigned int flags,
        EventHandleIO& handle,
        IOHandleNativeType io_handle,
        EventCallbackIO callback,
        void* arg);

    JINX_NO_DISCARD
    static
    ResultGeneric remove_io(Awaitable* awaitable, EventHandleIO& handle);

    JINX_NO_DISCARD
    ResultGeneric remove_io(EventHandleIO& handle);

    // signal

    JINX_NO_DISCARD
    static
    ResultGeneric add_signal(Awaitable* awaitable, EventHandleSignal& signal, int sig, EventCallback, void* arg);

    JINX_NO_DISCARD
    ResultGeneric add_signal(EventHandleSignal& signal, int sig, EventCallback, void* arg);

    JINX_NO_DISCARD
    static
    ResultGeneric remove_signal(Awaitable* awaitable, EventHandleSignal& signal);

    JINX_NO_DISCARD
    ResultGeneric remove_signal(EventHandleSignal& signal);


    JINX_NO_DISCARD
    ResultGeneric add_signal(EventHandleSignalFunctional& signal, int sig, std::function<void(const error::Error&)>&&);

    JINX_NO_DISCARD
    ResultGeneric remove_signal(EventHandleSignalFunctional& signal);

    // timer

    JINX_NO_DISCARD
    static
    ResultGeneric add_timer(Awaitable* awaitable, EventHandleTimer& timer, struct timeval* timeval, EventCallback, void* arg);

    JINX_NO_DISCARD
    ResultGeneric add_timer(EventHandleTimer& timer, struct timeval* timeval, EventCallback, void* arg);

    JINX_NO_DISCARD
    static
    ResultGeneric remove_timer(Awaitable* awaitable, EventHandleTimer& timer);

    JINX_NO_DISCARD
    ResultGeneric remove_timer(EventHandleTimer& timer);

    // posix io
    inline static IOResult connect(IOHandleNativeType io_handle, const struct sockaddr* addr, socklen_t len) noexcept {
        IOResult result;
        int err = 0;
        do {
            result._int = ::connect(io_handle, addr, len);
            if (JINX_UNLIKELY(result._int == -1)) {
                err = errno;
                if (err == EINPROGRESS) {
                    result._error = error::Error(err, posix::category_again());
                } else {
                    result._error = error::Error(err, posix::category_posix());
                }
            }
        } while(err == EINTR);
        return result;
    }

    inline static IOResult accept(IOHandleNativeType io_handle, struct sockaddr* addr, socklen_t* addrlen) noexcept {
        return convert_to_asyncio_error_code(::accept(io_handle, addr, addrlen));
    }

    inline static IOResult read(IOHandleNativeType io_handle, SliceMutable& buffer) noexcept {
        return convert_to_asyncio_error_code(::read(io_handle, buffer.data(), buffer.size()));
    }

    inline static IOResult write(IOHandleNativeType io_handle, SliceConst& buffer) noexcept {
        return convert_to_asyncio_error_code(::write(io_handle, buffer.data(), buffer.size()));
    }

    inline static IOResult recv(IOHandleNativeType io_handle, SliceMutable& buffer, int flags) noexcept {
        return convert_to_asyncio_error_code(::recv(io_handle, buffer.data(), buffer.size(), flags));
    }

    inline static IOResult send(IOHandleNativeType io_handle, SliceConst& buffer, int flags) noexcept {
        return convert_to_asyncio_error_code(::send(io_handle, buffer.data(), buffer.size(), flags));
    }

    inline static IOResult recvfrom(IOHandleNativeType io_handle, SliceMutable& buffer, int flags,
        struct sockaddr* address, socklen_t* address_len) 
    {
        return convert_to_asyncio_error_code(::recvfrom(io_handle, buffer.data(), buffer.size(), flags,
            address, address_len));
    }

    inline static IOResult sendto(IOHandleNativeType io_handle, SliceConst& buffer, int flags,
        const struct sockaddr* address, socklen_t address_len) noexcept
    {
        return convert_to_asyncio_error_code(::sendto(io_handle, buffer.data(), buffer.size(), flags,
            address, address_len));
    }

    inline static IOResult readv(IOHandleNativeType io_handle, SliceMutable* buffers, int count) noexcept {
        return convert_to_asyncio_error_code(::readv(io_handle, reinterpret_cast<struct iovec*>(buffers), count));
    }

    inline static IOResult writev(IOHandleNativeType io_handle, SliceConst* buffers, int count) noexcept {
        return convert_to_asyncio_error_code(::writev(io_handle, reinterpret_cast<struct iovec*>(buffers), count));
    }
    
    inline static IOResult recvmsg(IOHandleNativeType io_handle, struct msghdr* msg, int flags) noexcept {
        return convert_to_asyncio_error_code(::recvmsg(io_handle, msg, flags));
    }

    inline static IOResult sendmsg(IOHandleNativeType io_handle, struct msghdr* msg, int flags) noexcept {
        return convert_to_asyncio_error_code(::sendmsg(io_handle, msg, flags));
    }

    inline static IOResult shutdown(IOHandleNativeType io_handle, int how) noexcept {
        return convert_to_asyncio_error_code(::shutdown(io_handle, how));
    }

private:

    template<typename T>
    inline static IOResult convert_to_asyncio_error_code(T res) noexcept {
        IOResult result;
        int err = 0;
        do {
            result.set_result(res);
            if (JINX_UNLIKELY(res == -1)) {
                err = errno;
                if (err == EAGAIN or err == EWOULDBLOCK) {
                    result._error = error::Error(err, posix::category_again());
                } else {
                    result._error = error::Error(err, posix::category_posix());
                }
            }
        } while(err == EINTR);
        return result;
    }
};

class EventEngineLibevent::EventDataSimpleFunctional {
    friend class EventEngineLibevent;

    struct event _event{};
    int _state;
    std::function<void(const error::Error&)> _callback;

    static void callback(int io_handle, short event, void* arg);
public:
    explicit EventDataSimpleFunctional(std::function<void(const error::Error&)>&& callback);
    JINX_NO_COPY_NO_MOVE(EventDataSimpleFunctional);
    ~EventDataSimpleFunctional();
};

class EventEngineLibevent::EventDataSimple {
    friend class EventEngineLibevent;

    struct event _event{};
    int _state;
    EventCallback _callback;
    void* _arg;

    static void callback(int io_handle, short event, void* arg);
public:
    EventDataSimple(EventCallback callback, void* arg);
    JINX_NO_COPY_NO_MOVE(EventDataSimple);
    ~EventDataSimple();
};

class EventEngineLibevent::EventDataIO {
    friend class EventEngineLibevent;

    struct event _event{};
    int _state;
    EventCallbackIO _callback;
    void* _arg;

    static void callback(int io_handle, short event, void* arg);
public:
    EventDataIO(EventCallbackIO callback, void* arg);
    JINX_NO_COPY_NO_MOVE(EventDataIO);
    ~EventDataIO();
};

struct EventEngineLibevent::EventRead : public EventEngineLibevent::EventDataIO {
    using EventEngineLibevent::EventDataIO::EventDataIO;
};
struct EventEngineLibevent::EventWrite : public EventEngineLibevent::EventDataIO {
    using EventEngineLibevent::EventDataIO::EventDataIO;
};

} // namespace libevent
} // namespace jinx

#endif
