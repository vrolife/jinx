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
#ifndef __jinx_async_awaitable_ipp__
#define __jinx_async_awaitable_ipp__

#include <jinx/async/task.hpp>

namespace jinx {

class AsyncJoin : public Awaitable 
{
    class Join : public detail::AwaitableTaskNode 
    {
        bool _done{false};
        TaskPtr _observer{};

    public:
        Join& operator ()(TaskPtr&& observer) {
            _done = false;
            _observer = std::move(observer);
            return *this;
        }

        bool is_done() const { return _done; }

    protected:
        void async_finalize() noexcept override {
            _observer.reset();
            Awaitable::async_finalize();
        }

        ResultGeneric resume(const error::Error& error) override {
            _done = true;
            detail::AwaitableTaskNode::resume(error) >> JINX_IGNORE_RESULT;
            auto result = _observer->resume(error);
            _observer.reset();
            return result;
        }
    };

    TaskPtr _target{};
    Join _hook{};

public:
    AsyncJoin& operator ()(TaskPtr& target) {
        _target = target;
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        _target.reset();
        Awaitable::async_finalize();
    }

    Async async_poll() override {
        if (_hook.is_done()) {
            return async_return();
        }
        _hook(this->get_task());
        _target->add_hook(&_hook);
        return async_suspend();
    }
};

inline
TaskPtr Awaitable::get_task() noexcept {
    return TaskPtr::shared_from_this(_task);
}

inline
Async Awaitable::async_suspend() noexcept {
    _task->_top = this;
    return Async{ControlState::Suspended};
}

inline
Async Awaitable::async_return() noexcept {
    _task->_top = _caller;
    return Async{ControlState::Ready};
}

inline
Async Awaitable::async_yield() noexcept {
    _task->_top = this;
    return Async{ControlState::Yield};
}

inline
Async Awaitable::async_throw(const error::Error& error) noexcept 
{
    /*
        Sometimes, the base class returns normally(_top == _caller), 
        but the derived class treats the result as an error and throws the error.
        So, restore the pointer `_top` to handle error.
    */
    if (JINX_LIKELY(_task != nullptr)) {
        _task->_top = this;
    }

    if (error.value() == 0) {
        this->_error = make_error(ErrorAwaitable::InvalidErrorCode);
    } else {
        this->_error = error;
    }
    return Async{ControlState::Raise};
}

template<typename A, typename TaskHeapType, typename... Args>
inline
TaskPtr Awaitable::task_new(Args&&... args) {
    auto task = TaskHeapType::template new_task<TaskHeapType>();

    if (task == nullptr) {
        return TaskPtr{nullptr};
    }

    auto task_ptr = TaskPtr{task};

    (*task)(std::forward<Args>(args)...);

    _task->_task_queue->spawn(task_ptr) >> JINX_IGNORE_RESULT;
    return task_ptr;
}

template<typename T>
JINX_NO_DISCARD
ResultGeneric Awaitable::task_create(TaskStatic<T>& branch) {
    TaskPtr task{&branch};
    auto result = _task->_task_queue->spawn(task);
    task.release();
    return result;
}

JINX_NO_DISCARD
inline
ResultGeneric Awaitable::task_create(TaskPtr& task) {
    return _task->_task_queue->spawn(task);
}

template<typename Config, typename Allocator, typename... TArgs>
JINX_NO_DISCARD
ResultGeneric Awaitable::task_allocate(Allocator* allocator, TArgs&&... args) {
    typedef typename Config::TaskType TaskType;
    return TaskType::template allocate<Config>(this->_task->_task_queue, allocator, std::forward<TArgs>(args)...);
}

JINX_NO_DISCARD
inline
ResultGeneric Awaitable::async_resume(const error::Error& error)
{
    if (_task == nullptr) {
        return Failed_;
    }
    return _task->resume(error);
}

template<typename TaskPointer>
inline
Result<ErrorCancel> Awaitable::async_cancel(TaskPointer&& task)
{
    auto* task_queue = task->_task_queue;
    if (task_queue == nullptr) {
        return ErrorCancel::CancelDequeuedTask;
    }
    return task_queue->cancel(task);
}

} // namespace jinx

#endif
