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
#ifndef __jinx_async_pausable_hpp__
#define __jinx_async_pausable_hpp__

#include <jinx/async/task.hpp>

namespace jinx {

class AwaitablePausable : public Awaitable {
    template<typename Base>
    friend class MixinPausable;
    
    template<typename T>
    friend class Wait;

    Awaitable* _prev;

    bool is_pausable() const override { return true; }

protected:
    inline
    bool is_paused() const {
        return _prev != nullptr;
    }

    inline 
    bool is_reentry() const {
        return this->_prev != this->_caller;
    }

    // Move this Awaitable to the top of call stack
    void swap_awaitable() {
        auto* caller = this->_caller;
        if (caller->_caller != this) {
            // walk call stack
            do {
                caller = caller->_caller;
            } while(caller->_caller != this);

            // stupid "RTTI"
            if (caller->is_pausable()) {
                auto* pausable = static_cast<AwaitablePausable*>(caller);
                pausable->_prev = this->_prev;
            }
        }
        caller->_caller = this->_prev;
        this->_prev = nullptr;
    }

    Async handle_error(const error::Error& error) override {
        if (JINX_UNLIKELY(is_paused())) {
            return Async { ControlState::Passthrough };
        }
        return Awaitable::handle_error(error);
    }

    /* 
        Swap between the caller and this Awaitable.
        Because we need a chance to finalize the Awaitable after the caller returns
    */
    JINX_NO_DISCARD
    Async async_pause() noexcept{
        _prev = this->_caller->_caller;
        this->_caller->_caller = this;
        this->_task->_top = this->_caller;
        this->_caller = _prev;
        return Async{ControlState::Pause};
    }

    void async_finalize() noexcept override {
        _prev = nullptr;
        Awaitable::async_finalize();
    }
};

template<typename Base>
class MixinPausable
: public Base
{
    typedef Base BaseType;
    
protected:
    Async async_poll() override {
        if (not this->is_paused()) {
            return BaseType::async_poll();
        }

        // reentry
        if (this->is_reentry()) 
        {    
            this->swap_awaitable();
            return BaseType::async_poll();
        }
        return this->async_return();
    }
};

using AsyncRoutinePausable = MixinPausable<MixinRoutine<AwaitablePausable>>;

template<typename T>
using AsyncFunctionPausable = MixinPausable<MixinFunction<AwaitablePausable,T>>;

enum WaitCondition {
    AllCompleted,
    FirstCompleted,
    FirstException
};

template<typename T>
class Wait : public AwaitablePausable, private TaskQueue
{
    typedef Awaitable BaseType;
private:
    enum class Stage {
        Suspended,
        Paused,
        Call
    };

    WaitCondition _condition{};
    bool _timeout{false};
    Stage _stage{Stage::Call};

    LinkedListThreadSafe<Task> _tasks_done;

    EventEngineAbstract* get_event_engine() override {
        return _task->_task_queue->get_event_engine();
    }

    typedef TaskTagged<T, TaskBound<Task>> Tagged;

public:

    Wait() = default;
    JINX_NO_COPY_NO_MOVE(Wait);

    ~Wait() override {
        if (_tasks_done.get_size_unsafe() != 0) {
            error::error("jinx::Wait task queue was destroyed but the completed tasks has not been processed.");
        }
    }

    void initialize(WaitCondition condition)
    {
        set_condition(condition);

        _condition = condition;
        _timeout = false;
        _stage = Stage::Call;
    }

    void set_condition(WaitCondition cond) noexcept {
        _condition = cond;
    }

    bool is_timeout() const noexcept {
        return _timeout;
    }

    bool is_done() const noexcept {
        return empty();
    }

    template<typename A, typename... Args>
    JINX_NO_DISCARD
    inline
    TaskPtr branch_new(Args&&... args) {
        typedef TaskEntry<TaskHeap<Tagged>, A> TaggedShared;
        auto* branch = new(std::nothrow) TaggedShared();
        
        if (branch == nullptr) {
            return TaskPtr{nullptr};
        }

        auto task = TaskPtr{branch};

        (*branch)(std::forward<Args>(args)...);

        TaskQueue::spawn(task) >> JINX_IGNORE_RESULT;
        return task;
    }

    JINX_NO_DISCARD
    inline
    ResultGeneric branch_create(Tagged& branch) {
        TaskPtr task{&branch};
        auto result = TaskQueue::spawn(task);
        task.release();
        return result;
    }

    template<typename F>
    void for_each(F&& fun) {
        while (auto* task = _tasks_done.pop()) {
            TaskPtr ptr{task};
            fun(ptr, static_cast<Tagged*>(task)->get_tag());
        }
    }

protected:
    JINX_NO_DISCARD
    ResultGeneric schedule(Task *task, const error::Error& error) override {
        if (TaskQueue::schedule(task, error).is(Failed_)) {
            return Failed_;
        }
        return this->async_resume();
    }

    Async task_queue_run() {
        bool should_suspend = true;
        TaskQueue::run(
            [&]() {
                return true;
            },
            [&](Task* task) {
                static_cast<Tagged*>(task)->set_bound_task(get_task());
            },
            [&](Task* task) {
                auto ptr = TaskPtr::shared_from_this(task);
                
                if (JINX_UNLIKELY(pop(ptr).is(Failed_))) {
                    error::error("The task is not attached to any queue");
                }
                static_cast<Tagged*>(task)->set_bound_task(nullptr);
                _tasks_done.push(ptr.release());

                if (_condition == WaitCondition::FirstException and task->_error) {
                    should_suspend = false;
                    return true;
                }

                if (_condition == WaitCondition::FirstCompleted) {
                    should_suspend = false;
                    return true;
                }

                if (empty()) {
                    should_suspend = false;
                    return true;
                }

                return false;
            }
        );

        if (should_suspend) {
            return async_suspend();
        }
        return async_pause();
    }

    Async async_poll() override  // NOLINT
    {
        if (not is_paused()) {
            return task_queue_run();
        }

        if (this->is_reentry()) 
        {    
            this->swap_awaitable();
            return task_queue_run();
        }
        return this->async_return();
    }
    
    Async handle_error(const error::Error& error) override {
        if (JINX_UNLIKELY(is_paused())) {
            return Async { ControlState::Passthrough };
        }
        return BaseType::handle_error(error);
    }

    void async_finalize() noexcept override {
        TaskQueue::exit();
        for_each([&](TaskPtr&, T){ });
        Awaitable::async_finalize();
    }

    JINX_NO_DISCARD
    Result<ErrorCancel> cancel(TaskPtr& task) override {
        if (_running_task == task) {
            return ErrorCancel::CancelRunningTask;
        }
        return _task->_task_queue->cancel(task);
    }

    JINX_NO_DISCARD
    ResultGeneric spawn(TaskPtr& task) override {
        return this->_task->_task_queue->spawn(task);
    }
};


} // namespace jinx

#endif
