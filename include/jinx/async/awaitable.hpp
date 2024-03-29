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
#ifndef __jinx_async_awaitable_hpp__
#define __jinx_async_awaitable_hpp__

#include <chrono>
#include <iostream>

#include <jinx/assert.hpp>
#include <jinx/macros.hpp>
#include <jinx/linkedlist.hpp>
#include <jinx/optional.hpp>
#include <jinx/eventengine.hpp>
#include <jinx/pointer.hpp>
#include <jinx/error.hpp>

#if defined(__cpp_exceptions) && __cpp_exceptions
#include <jinx/exception.hpp>
#endif

namespace jinx {

class Loop;
class Task;
class TaskQueue;
class AwaitablePausable;

template<typename Base>
class MixinPausable;

template<typename Base>
class TaskHeap;

template<typename Base>
class TaskStatic;

template<typename Base, typename A>
class TaskEntry;

template<typename T>
class Wait;

namespace detail {
class AwaitableTaskNode;
}

enum class ControlState : int {
    Ready = 0,
    Raise = 1,
    Suspended = 2,
    Yield = 4,
    Pause = 8,
    Passthrough = 16,
    Exited = 24,
};

enum class ErrorAwaitable {
    NoError,
    Uninitialized,
    Cancelled,
    Popped,
    
    /*
        Throw error with zero
    */
    InvalidErrorCode,
    NoSys,
    CXXException,
    ExitLoop,
    InternalError
};

enum class ErrorCancel {
    NoError,
    CancelRunningTask,
    CancelDequeuedTask,

    /*
        The task is ready. It will be canceled in the next loop
    */
    DeferredDequeue
};

enum class ErrorWait {
    Timeout = 1
};

JINX_ERROR_DEFINE(awaitable, ErrorAwaitable);
JINX_ERROR_DEFINE(wait, ErrorWait);
JINX_ERROR_DEFINE(cancel, ErrorCancel);

typedef pointer::PointerShared<Task> TaskPtr;

#ifndef NDEBUG
class Async {
    friend class Awaitable;
    friend class AwaitablePausable;
    friend class Task;

    template<typename Base>
    friend class MixinPausable;

    template<typename T>
    friend class Wait;

    ControlState _state;

    explicit Async(ControlState state) 
    : _state(state) { }

public:
    inline
    operator ControlState () const { return _state; } // NOLINT
};
#else
typedef ControlState Async;
#endif

class Awaitable {
    template<typename T>
    friend class Wait;

    template<typename Base>
    friend class MixinPausable;

    friend class detail::AwaitableTaskNode;
    friend class AwaitablePausable;

    friend class Loop;
    friend class Task;
    friend class TaskQueue;
    friend class EventEngineAbstract;

    Task* _task{nullptr};

    Awaitable* _caller{nullptr};

    error::Error _error{};

private:
    Async run();

public:
    virtual ~Awaitable() = default;

protected:
    Awaitable() = default;
    JINX_NO_COPY_NO_MOVE(Awaitable);

    TaskPtr get_task() noexcept;

    virtual void print_backtrace(std::ostream& output_stream, int indent) {
        output_stream 
            << (indent > 0 ? '-' : '|') 
            << "Awaitable{ task: " << _task 
            << ", caller: " << _caller 
            << " }" << std::endl;
    }

    JINX_NO_DISCARD
    virtual Async async_poll() = 0;

    JINX_NO_DISCARD
    virtual Async handle_error(const error::Error& error) {
        return Async {ControlState::Raise};
    }

    // do not call async_finalize() in destructor
    virtual void async_finalize() noexcept {
        _task = nullptr;
        _error = {};

        // Keep this member for backtrace
        // _caller = nullptr;
    }

    JINX_NO_DISCARD
    inline
    Async async_call(Awaitable& callee) {
        callee._caller = this;
        callee._task = _task;
        return callee.run();
    }

    JINX_NO_DISCARD
    Async async_return() noexcept;

    JINX_NO_DISCARD
    Async async_suspend() noexcept;

    JINX_NO_DISCARD
    Async async_yield() noexcept;

    Async async_throw(const error::Error& error) noexcept;

    template<typename T, typename =decltype(make_error(std::declval<T>()))>
    Async async_throw(T error) noexcept {
        return async_throw(make_error(error));
    }

    template<
        typename A, 
        typename TaskHeapType=typename A::template TaskHeapType<A>, 
        typename... Args>
    TaskPtr task_new(Args&&... args);

    template<typename T>
    JINX_NO_DISCARD
    ResultGeneric task_create(TaskStatic<T>& branch);

    JINX_NO_DISCARD
    ResultGeneric task_create(TaskPtr& task);

    template<typename Config, typename Allocator, typename... TArgs>
    JINX_NO_DISCARD
    ResultGeneric task_allocate(Allocator* allocator, TArgs&&... args);

public:
    JINX_NO_DISCARD
    ResultGeneric async_resume(const error::Error& error = {});

    template<typename A>
    using TaskHeapType = TaskEntry<TaskHeap<Task>, A>;

    template<typename TaskPointer>
    JINX_NO_DISCARD
    static Result<ErrorCancel> async_cancel(TaskPointer&& task);

private:
    // see AwaitablePausable::swap_awaitable
    virtual bool is_pausable() const { return false; }
};

template<typename T>
class MixinResult {
    Optional<T> _optional_value;

public:
    typedef typename Optional<T>::ValueType ValueType;

    bool empty() const { return _optional_value.empty(); }

    typename Optional<T>::
    ValueType& get_result() noexcept {
        return _optional_value.get();
    }

    const
    typename Optional<T>::
    ValueType& get_result() const noexcept  {
        return _optional_value.get();
    }

    void reset() noexcept {
        _optional_value.reset();
    }

    typename Optional<T>::
    ValueType release() {
        return _optional_value.release();
    }

    template<typename... Args>
    void emplace_result(Args&&... args) {
        _optional_value.emplace(std::forward<Args>(args)...);
    }
};

template<typename Base>
class MixinRoutine
: public Base 
{
    template<typename T>
    struct Callback {
        Async (T::*_callback)();
        T *_self;

        static Async run(Callback<T>* calllback) {
            return (calllback->_self->*(calllback->_callback))();
        }
    };
    
    Async (*_run)(void*){};
    char _mem[sizeof(Callback<Awaitable>)]{};

    template<typename T>
    inline void heapless_bind(Async(T::*callback)(), T* self) noexcept {
        // UB yes
        static_assert(sizeof(_mem) == sizeof(Callback<T>), "incompatable c++ implement");

        new(_mem)Callback<T>{callback, self};
        _run = reinterpret_cast<decltype(_run)>(&Callback<T>::run);
    }

    Async uninitialized() {
        return this->async_throw(ErrorAwaitable::Uninitialized);
    }
    
public:
    MixinRoutine() = default;

protected:
    Async async_poll() override {
        return _run(_mem);
    }
  
    void async_finalize() noexcept override {
        heapless_bind(&MixinRoutine::uninitialized, static_cast<MixinRoutine*>(this));
        Awaitable::async_finalize();
    }

    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
           << "MixinRoutine{ this: " 
           << this << ", callback: " 
           << reinterpret_cast<void*>(_run)
           << " }" << std::endl;
        Awaitable::print_backtrace(output_stream, indent + 1);
    }

    template<typename T>
    inline
    void async_start(Async(T::*callback)()) noexcept {
        heapless_bind(callback, static_cast<T*>(this));
    }

    template<typename T>
    inline
    Async async_yield(Async(T::*callback)()) noexcept {
        heapless_bind(callback, static_cast<T*>(this));
        return Awaitable::async_yield();
    }

    template<typename T>
    inline
    Async async_await(Awaitable& callee, Async(T::*callback)()) {
        heapless_bind(callback, static_cast<T*>(this));
        return this->async_call(callee);
    }

    class Pack {
        MixinRoutine* _caller;
        Awaitable& _callee;
        
    public:
        Pack(MixinRoutine* caller, Awaitable& callee) : _caller(caller), _callee(callee) { }

        template<typename T>
        inline
        Async operator /(Async(T::*callback)()) {
            _caller->heapless_bind(callback, static_cast<T*>(_caller));
            return _caller->async_call(_callee);
        }
    };

    friend Pack;

    inline
    Pack operator /(Awaitable& callee) noexcept {
        return {this, callee};
    }
};

template<typename Base, typename T>
class MixinFunction : public MixinRoutine<Base>, private MixinResult<T> {
public:
    typedef typename MixinResult<T>::ValueType ValueType;

    using MixinResult<T>::empty;
    using MixinResult<T>::get_result;
    using MixinResult<T>::reset;
    using MixinResult<T>::release;

protected:
    using MixinResult<T>::emplace_result;
};

using AsyncRoutine = MixinRoutine<Awaitable>;

template<typename T>
using AsyncFunction = MixinFunction<Awaitable, T>;

template<typename T>
class AsyncFuture : public Awaitable, public MixinResult<T> {
public:
    AsyncFuture() = default;

    bool is_done() {
        return not this->empty();
    }

    template<typename... Args>
    ResultGeneric emplace_result(Args&&... args) {
        MixinResult<T>::emplace_result(std::forward<Args>(args)...);
        return this->async_resume();
    }

    ResultGeneric set_error(const error::Error& error) noexcept {
        return this->async_resume(error);
    }

protected:
    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }
        return this->async_suspend();
    }
};

template<typename EventEngine>
class AsyncSleep : public Awaitable, public MixinResult<void>
{
    typename EventEngine::EventHandleTimer _handle;
    struct timeval _timeval{};

protected:
    void async_finalize() noexcept override {
        EventEngine::remove_timer(this, _handle) >> JINX_IGNORE_RESULT;
        Awaitable::async_finalize();
    }

public:
    AsyncSleep() = default;
    JINX_NO_COPY_NO_MOVE(AsyncSleep);
    ~AsyncSleep() override = default;

    template<typename Rep, typename Period>
    AsyncSleep& operator () (const std::chrono::duration<Rep, Period>& duration)
    {
        this->reset();
        const auto sec = std::chrono::duration_cast<std::chrono::seconds>(duration);
        _timeval.tv_sec = sec.count();
        _timeval.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(duration - sec).count();
        return *this;
    }

    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }
        if (EventEngine::add_timer(this, _handle, &_timeval, &AsyncSleep::timer_callabck, this).is(Failed_)) {
            return this->async_throw(ErrorEventEngine::FailedToRegisterTimerEvent);
        }
        return this->async_suspend();
    }

    static void timer_callabck(const error::Error& error, void* data) {
        auto* self = reinterpret_cast<AsyncSleep*>(data);
        EventEngine::remove_timer(self, self->_handle) >> JINX_IGNORE_RESULT;
        self->emplace_result();
        self->async_resume() >> JINX_IGNORE_RESULT;
    }
};

class AsyncDoNothing : public Awaitable {
public:

    AsyncDoNothing() = default;
    JINX_NO_COPY_NO_MOVE(AsyncDoNothing);
    ~AsyncDoNothing() override = default;

    AsyncDoNothing& operator()() {
        return *this;
    }

protected:
    Async async_poll() override {
        return async_return();
    }
};

} // namespace jinx

#endif
