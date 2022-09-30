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
#include <cstddef>
#include <cstdio>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <event2/event.h>

#include <jinx/logging.hpp>
#include <jinx/macros.hpp>
#include <jinx/libevent.hpp>

namespace jinx {
namespace libevent {

enum EventState {
    Pending,
    Running,
    Deleted
};

static const char libevent_type_tag = 0;

// EventDataIO

void EventEngineLibevent::EventDataIO::callback(int io_handle, short event, void* args) {
    auto* handle = static_cast<Optional<EventDataIO>*>(args);
    auto& self = handle->get();
    self._state = Running;
    error::Error error;
    if ((event & EV_TIMEOUT) != 0) {
        error = make_error(posix::ErrorPosix::TimedOut);
    }
    self._callback(event, error, self._arg);
    if (self._state == Deleted) {
        handle->reset();
    } else {
        self._state = Pending;
    }
}

EventEngineLibevent::EventDataIO::EventDataIO(
    EventEngineLibevent::IOCallback callback,
    void* arg)
: _state(Pending), _callback(callback), _arg(arg)
{}

EventEngineLibevent::EventDataIO::~EventDataIO() {
    event_del(&_event);
}

// EventDataSimple

void EventEngineLibevent::EventDataSimple::callback(int io_handle, short event, void* args) {
    auto* handle = static_cast<Optional<EventDataSimple>*>(args);
    auto& self = handle->get();
    self._state = Running;
    error::Error error;
    if ((event & EV_TIMEOUT) != 0) {
        error = make_error(posix::ErrorPosix::TimedOut);
    }
    self._callback(error, self._arg);
    if (self._state == Deleted) {
        handle->reset();
    } else {
        self._state = Pending;
    }
}

EventEngineLibevent::EventDataSimple::EventDataSimple(
    EventEngineLibevent::Callback callback,
    void* arg)
: _state(Pending), _callback(callback), _arg(arg)
{}

EventEngineLibevent::EventDataSimple::~EventDataSimple() {
    event_del(&_event);
}

// EventDataSimpleFunctional

void EventEngineLibevent::EventDataSimpleFunctional::callback(int io_handle, short event, void* args) {
    auto* handle = static_cast<Optional<EventDataSimpleFunctional>*>(args);
    auto& self = handle->get();
    self._state = Running;
    error::Error error;
    if ((event & EV_TIMEOUT) != 0) {
        error = make_error(posix::ErrorPosix::TimedOut);
    }
    self._callback(error);
    if (self._state == Deleted) {
        handle->reset();
    } else {
        self._state = Pending;
    }
}

EventEngineLibevent::EventDataSimpleFunctional::EventDataSimpleFunctional(
    std::function<void(const error::Error&)>&& callback)
: _state(Pending), _callback(std::move(callback))
{}

EventEngineLibevent::EventDataSimpleFunctional::~EventDataSimpleFunctional() {
    event_del(&_event);
}

// preactor

static
inline
long do_io(
    EventEngineLibevent::IONativeHandleType io_handle, 
    SliceMutable& buffer, 
    size_t bytes_transferred) 
{
    return ::read(
        io_handle, 
        reinterpret_cast<char*>(buffer.data()) + bytes_transferred, 
        buffer.size() - bytes_transferred);
}

static
inline
long do_io(
    EventEngineLibevent::IONativeHandleType io_handle, 
    const SliceConst& buffer, 
    size_t bytes_transferred) 
{
    return ::write(
        io_handle, 
        reinterpret_cast<const char*>(buffer.data()) + bytes_transferred, 
        buffer.size() - bytes_transferred);
}

// backend

const void* EventEngineLibevent::type_tag() {
    return &libevent_type_tag;
}

void EventEngineLibevent::add_signal(
    EventHandleSignal& signal, 
    int sig, 
    EventEngineLibevent::Callback callback, 
    void* arg)
{
    signal.reset();
    signal.emplace(callback, arg);

    static_assert(std::is_same<EventHandleTimer, EventHandleSignal>::value, 
        "do not touch this class");
    
    event_assign(&signal.get()._event, _base, sig, 
        EV_SIGNAL, 
        &EventEngineLibevent::EventDataSimple::callback, &signal);
    event_add(&signal.get()._event, nullptr);
}

void EventEngineLibevent::remove_signal(EventHandleSignal& signal) { // NOLINT
    if (signal.empty()) {
        return;
    }

    if (signal.get()._state == Running) {
        signal.get()._state = Deleted;
    } else {
        signal.reset();
    }
}

void EventEngineLibevent::add_signal(
    EventHandleSignalFunctional& signal, 
    int sig, 
    std::function<void(const error::Error&)>&& callback)
{
    signal.reset();
    signal.emplace(std::move(callback));

    static_assert(std::is_same<EventHandleTimer, EventHandleSignal>::value, 
        "do not touch this class");
    
    event_assign(
        &signal.get()._event, 
        _base, 
        sig, 
        EV_SIGNAL, 
        &EventEngineLibevent::EventDataSimpleFunctional::callback, 
        &signal);
    event_add(&signal.get()._event, nullptr);
}

void EventEngineLibevent::remove_signal(EventHandleSignalFunctional& signal) { // NOLINT
    if (signal.empty()) {
        return;
    }

    if (signal.get()._state == Running) {
        signal.get()._state = Deleted;
    } else {
        signal.reset();
    }
}

void EventEngineLibevent::add_timer(
    EventHandleTimer& timer, 
    struct timeval* timeval, 
    EventEngineLibevent::Callback callback, 
    void* arg)
{
    timer.reset();
    timer.emplace(callback, arg);

    static_assert(std::is_same<EventHandleTimer, EventHandleSignal>::value, 
        "do not touch this class");
    event_assign(
        &timer.get()._event, 
        _base, 
        -1, 
        EV_TIMEOUT, 
        &EventEngineLibevent::EventDataSimple::callback, 
        &timer);
    event_add(&timer.get()._event, timeval);
}

void EventEngineLibevent::remove_timer(EventHandleTimer& timer) { // NOLINT
    if (timer.empty()) {
        return;
    }

    if (timer.get()._state == Running) {
        timer.get()._state = Deleted;
    } else {
        timer.reset();
    }
}

void EventEngineLibevent::add_io(
    unsigned int flags, 
    EventHandleIO& event_handle, 
    IONativeHandleType io_handle, 
    EventEngineLibevent::IOCallback callback, 
    void* arg)
{
    event_handle.reset();
    event_handle.emplace(callback, arg);
    event_assign(
        &event_handle.get()._event, 
        _base, 
        io_handle, 
        flags,
        &EventEngineLibevent::EventDataIO::callback, 
        &event_handle);
    event_add(&event_handle.get()._event, nullptr);
}

void EventEngineLibevent::remove_io(EventHandleIO& handle) // NOLINT
{
    if (handle.get()._state == Running) {
        handle.get()._state = Deleted;
    } else {
        handle.reset();
    }
}

#if 0
error::Error EventEngineLibevent::read(IONativeHandle fd, SliceMutable buffer, IOOption& opt, PreactorCallback callback, void* arg)
{
    if (opt._value > buffer.size()) {
        return std::make_error(std::errc::result_out_of_range);
    }
    new Preactor<SliceMutable>(_base, fd, EV_READ, opt, buffer, callback, arg);
    return {};
}

error::Error EventEngineLibevent::write(IONativeHandle fd, SliceConst buffer, IOOption& opt, PreactorCallback callback, void* arg)
{
    if (opt._value > buffer.size()) {
        return std::make_error(std::errc::result_out_of_range);
    }
    new Preactor<SliceConst>(_base, fd, EV_WRITE, opt, buffer, callback, arg);
    return {};
}
#endif

void EventEngineLibevent::poll()
{
    event_base_loop(_base, _poll_flags | _additional_flags);
    _additional_flags = 0;
    auto add = event_base_get_num_events(_base, EVENT_BASE_COUNT_ADDED);
    auto act = event_base_get_num_events(_base, EVENT_BASE_COUNT_ACTIVE);
    auto vir = event_base_get_num_events(_base, EVENT_BASE_COUNT_VIRTUAL);
    // std::cout << add << " " << act << " " << vir << std::endl;
}

void EventEngineLibevent::wakeup() {
    _additional_flags = EVLOOP_NONBLOCK;
    event_base_loopbreak(_base);
}

void EventEngineLibevent::set_non_blocking(bool enable)
{
    if (enable) {
        _poll_flags |= EVLOOP_NONBLOCK;
    } else {
        _poll_flags &= ~EVLOOP_NONBLOCK;
    }
}

void EventEngineLibevent::set_no_exit_on_empty(bool enable)
{
    if (enable) {
        _poll_flags |= EVLOOP_NO_EXIT_ON_EMPTY;
    } else {
        _poll_flags &= ~EVLOOP_NO_EXIT_ON_EMPTY;
    }
}

void EventEngineLibevent::set_debug(bool enable)
{
    if (enable) {
        event_enable_debug_mode();
        event_enable_debug_logging(EVENT_DBG_ALL);
    }
}

EventEngineLibevent::EventEngineLibevent(bool thread_safe)
: _base(nullptr), _poll_flags(EVLOOP_ONCE)
{
    auto *config = event_config_new();

    event_config_set_flag(config, EVENT_BASE_FLAG_PRECISE_TIMER);
    if (!thread_safe) {
        event_config_set_flag(config, EVENT_BASE_FLAG_NOLOCK);
    }
    event_config_require_features(config, EV_FEATURE_ET);
    _base = event_base_new_with_config(config);

    event_config_free(config);
}

EventEngineLibevent::~EventEngineLibevent() {
    if (_base != nullptr) {
        event_base_free(_base);
        _base = nullptr;
    }
}

} // namespace libevent
} // namespace jinx
