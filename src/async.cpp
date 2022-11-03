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
#include <event2/event.h>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/queue.hpp>
#include <jinx/buffer.hpp>

namespace jinx {

Async Awaitable::run() {
    ControlState state;
    while (true) {
        if (JINX_UNLIKELY(this->_error)) 
        {
            /*
                Member `_error` will be clean by move assign constructor
            */
            auto error = std::move(this->_error);

            state = handle_error(error);

            if (state == ControlState::Raise or state == ControlState::Passthrough)
            {
                if (JINX_UNLIKELY(_task->_exc == nullptr)) {
                    _task->_exc = this;
                }
                _caller->_error = error;
                _task->_top = _caller;
                this->async_finalize();
                return Async{ControlState::Ready};
            }
            
            if (state == ControlState::Exited) {
                _task->_error = error;
            }

        } else {
#if defined(__cpp_exceptions) && __cpp_exceptions
try {
#endif
            state = async_poll();
#if defined(__cpp_exceptions) && __cpp_exceptions
} catch (...) {
            state = ControlState::Raise;
            this->_error = make_error(ErrorAwaitable::CXXException);
            _task->set_exception(std::current_exception());
}
#endif
        }

        static_assert(static_cast<int>(ControlState::Ready) == 0, "");
        static_assert(static_cast<int>(ControlState::Raise) == 1, "");
        if (state > ControlState::Raise)
        {
            if (state == ControlState::Pause) {
                // prevent async_finalize
                return Async{ControlState::Ready};
            }
            break;
        }

        if (_task->_top != this) {
            this->async_finalize();
            break;
        }
    }
    return Async {state};
}

ResultGeneric Task::resume(const error::Error& error) noexcept {
    if (_task_queue == nullptr) {
        return Failed_;
    }

    if (_task_queue->schedule(this, error).is(Failed_)) {
        return Failed_;
    }
    return Successful_;
}

JINX_ERROR_IMPLEMENT(awaitable, {
    switch(code.as<ErrorAwaitable>()) {
        case ErrorAwaitable::NoError:
            return "No error";
        case ErrorAwaitable::Uninitialized:
            return "Poll an uninitialized awaitable object";
        case ErrorAwaitable::Cancelled:
            return "Task is cancelled";
        case ErrorAwaitable::Popped:
            return "Task is popped";
        case ErrorAwaitable::InvalidErrorCode:
            return "Raise error::Error with value 0";
        case ErrorAwaitable::NoSys:
            return "Function not implemented";
        case ErrorAwaitable::CXXException:
            return "Uncaught C++ exception";
        case ErrorAwaitable::ExitLoop:
            return "Exit loop";
        case ErrorAwaitable::InternalError:
            return "Internal error";
    }
    return "unknown error value";
});

JINX_ERROR_IMPLEMENT(cancel, {
    switch(code.as<ErrorCancel>()) {
        case ErrorCancel::NoError:
            return "No error";
        case ErrorCancel::CancelRunningTask:
            return "Try to cancel a running task";
        case ErrorCancel::CancelDequeuedTask:
            return "Try to cancel a dequeued task";
        case ErrorCancel::DeferredDequeue:
            return "The task has been cancelled, but it will be dequeue later";
    }
    return "unknown error value";
});

JINX_ERROR_IMPLEMENT(wait, {
    switch(code.as<ErrorWait>()) {
        case ErrorWait::Timeout:
            return "Timeout";
    }
    return "unknown error value";
});

JINX_ERROR_IMPLEMENT(queue, {
    switch(code.as<ErrorQueue>()) {
        case ErrorQueue::NoError:
            return "No error";
        case ErrorQueue::PendingPutError:
            return "Unable push Put operation into pending list";
        case ErrorQueue::PendingGetError:
            return "Unable push Gut operation into pending list";
    }
    return "unknown error value";
});

namespace buffer {

JINX_ERROR_IMPLEMENT(buffer, {
    switch(code.as<buffer::ErrorBuffer>()) {
        case buffer::ErrorBuffer::NoError:
            return "No error";
        case buffer::ErrorBuffer::OutOfMemory:
            return "Out of memory";
    }
    return "unknown error value";
});

JINX_ERROR_IMPLEMENT(async_allocate, {
    switch(code.as<buffer::ErrorAllocate>()) {
        case buffer::ErrorAllocate::NoError:
            return "No error";
        case buffer::ErrorAllocate::RegisterAllocateError:
            return "Unable register buffer allocation";
        case buffer::ErrorAllocate::UnregisterAllocateError:
            return "Unable unregister buffer allocation";
    }
    return "unknown error value";
});

} // namespace buffer
} // namespace jinx
