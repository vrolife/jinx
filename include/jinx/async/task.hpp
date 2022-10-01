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
#ifndef __jinx_async_task_hpp__
#define __jinx_async_task_hpp__

#include <jinx/async/awaitable.hpp>

namespace jinx {

typedef void (*DestroyTaskFunction)(Task* task);

namespace detail {
class AwaitableTaskNode : public Awaitable {
    Awaitable* _next{nullptr};

    friend class ::jinx::Task;

protected:
    JINX_NO_DISCARD
    virtual ResultGeneric resume(const error::Error& error);

    Async handle_error(const error::Error& error) override {
        resume(error) >> JINX_IGNORE_RESULT;
        return async_throw(error);
    }

    Async async_poll() override {
        resume({}) >> JINX_IGNORE_RESULT;
        return async_return();
    }
};
} // namespace detail

class Task 
: protected Awaitable,
  private LinkedListThreadSafe<Task>::Node,
  private LinkedList<Task>::Node
{
    friend class TaskQueue;

    template<typename T>
    friend class pointer::PointerShared;

    template<typename T, typename EventEngine>
    friend class Wait;

    template<typename Base>
    friend class MixinPausable;

    friend class Loop;
    friend class Awaitable;
    friend class AwaitablePausable;
    friend class AsyncWait;
    friend class LinkedListThreadSafe<Task>;
    friend class LinkedList<Task>;

    typedef std::true_type SharedPointerRelease;
    typedef std::true_type SharedPointerCheckSingleReference;

protected:
    std::atomic<long> _refc{1};

private:
    std::atomic<ControlState> _state{ControlState::Suspended};
    Awaitable* _top{nullptr};
    TaskQueue* _task_queue{nullptr};
#if defined(__cpp_exceptions) && __cpp_exceptions
    std::exception_ptr _exception;
#endif
    Awaitable* _exc{nullptr};

    inline ControlState run() {
        ControlState state{};
        while((state = _top->run()) == ControlState::Ready) { }
        return state;
    }

    Awaitable* get_entry() noexcept { return _caller; }
    void set_entry(Awaitable* awaitable) noexcept { _caller = awaitable; }

    inline
    static void shared_pointer_release(Task* task) {
        task->destroy()(task);
    }

protected:
    virtual void shared_pointer_check_signle_reference() { }

    JINX_NO_DISCARD
    virtual DestroyTaskFunction destroy() {
        error::fatal("destroy unmanaged task");
    }

    Async handle_error(const error::Error& error) override {
        _top = nullptr;
        _state = ControlState::Exited;
        return Async{ControlState::Exited};
    }

    Async async_poll() override {
        _top = nullptr;
        _state = ControlState::Exited;
        return Async{ControlState::Exited};
    }

    void init(Awaitable* top) noexcept {
        _task = this;

        set_entry(top); // _caller = top;

        _state = ControlState::Suspended;
        _top = top;
        _error = {};
        _task_queue = nullptr;
#if defined(__cpp_exceptions) && __cpp_exceptions
        _exception = nullptr;
#endif
        _exc = nullptr;

        _top->_task = this;
        _top->_caller = this;
    }

    void print_backtrace(std::ostream& output_stream, int indent) override {
        output_stream << (indent > 0 ? '-' : '|') 
            << "Task{ this: " << this << ", state: " << static_cast<int>(_state.load()) 
            << ", top: " << _top << " }" << std::endl;
    
    }
    
    Task() = default;

public:
    JINX_NO_COPY_NO_MOVE(Task);
    ~Task() override = default;

    const error::Error& get_error_code() const noexcept { return _error; }
    inline void set_error_code(const error::Error& error) noexcept {
        _error = error;
    }

#if defined(__cpp_exceptions) && __cpp_exceptions
    std::exception_ptr get_exception() const noexcept {
        return _exception;
    }

    inline void set_exception(const std::exception_ptr& exception) noexcept {
        _exception = exception;
    }
#endif

    JINX_NO_DISCARD
    ResultGeneric resume(const error::Error& error) noexcept;

    void add_hook(detail::AwaitableTaskNode* hook) noexcept {
        hook->_task = this;
        hook->_next = this->get_entry();
        hook->_caller = this;
        hook->_next->_caller = hook;
        set_entry(hook);
    }

    void remove_hook(detail::AwaitableTaskNode* hook) noexcept {
        jinx_assert(hook->_task == this);
        
        hook->_next->_caller = hook->_caller;
        if (hook->_caller == this) {
            set_entry(hook->_next);
        } else {
            auto* prev = static_cast<detail::AwaitableTaskNode*>(hook->_caller);
            prev->_next = hook->_next;
        }

        hook->_next = nullptr;
    }
};

template<typename Base = Task>
class TaskStatic : public Base
{
    static void destroy_task(Task* task) { }

protected:
    DestroyTaskFunction destroy() override {
        return destroy_task;
    }

    TaskStatic() = default;

public:
    ~TaskStatic() override {
        auto refc = this->_refc.fetch_sub(1);
        if (refc != 1) {
            error::fatal("destroy static task but still referenced");
        }
    }

    JINX_NO_COPY_NO_MOVE(TaskStatic);

    void* operator new(std::size_t size) = delete;
    void* operator new[](std::size_t size) = delete;
};

template<typename Base = Task>
class TaskHeap : public Base
{
    friend class ::jinx::Loop;
    friend class ::jinx::Awaitable;

    static void destroy_task(Task* task) {
        delete task;
    }

protected:
    DestroyTaskFunction destroy() override {
        return destroy_task;
    }

    TaskHeap() = default;

public:
    template<typename T>
    static
    T* new_task() {
        return new(std::nothrow)T();
    }
};

template<typename Base, typename A>
class TaskEntry : public Base
{
    friend class ::jinx::Loop;
    friend class ::jinx::Awaitable;

    A _awaitable{};

public:
    TaskEntry() = default;

    template<typename... Args>
    TaskEntry& operator()(Args&&... args) {
        _awaitable(std::forward<Args>(args)...);
        this->init(&_awaitable);
        return *this;
    }

    A* operator ->() noexcept {
        return &_awaitable;
    }
};

template<typename T, typename Base = Task>
class TaskTagged : public Base
{
    T _tag{};

protected:
    using Base::Base;

public:
    T& get_tag() noexcept { return _tag; }

    template<typename Arg>
    TaskTagged& set_tag(Arg&& arg) noexcept {
        _tag = std::forward<Arg>(arg);
        return *this;
    }

    using Task::get_error_code;
};

template<typename Base = Task>
class TaskBound : public Base
{
    TaskPtr _bound_task{};

protected:
    void shared_pointer_check_signle_reference() override {
        if (_bound_task != nullptr) {
            _bound_task.reset();
        }
        Base::shared_pointer_check_signle_reference();
    }

public:
    void set_bound_task(TaskPtr& task) {
        _bound_task = task;
    }

    void set_bound_task(TaskPtr&& task) {
        _bound_task = std::move(task);
    }

    void set_bound_task(std::nullptr_t ignored) {
        _bound_task.reset();
    }

    TaskPtr& get_bound_task() {
        return _bound_task;
    }

protected:
    TaskBound() = default;
};

template<typename Tag,  typename A>
using Tagged = TaskEntry<TaskStatic<TaskTagged<Tag, TaskBound<Task>>>, A>;

namespace detail {

JINX_NO_DISCARD
inline
ResultGeneric AwaitableTaskNode::resume(const error::Error& error) {
    this->_task->remove_hook(this);
    return Successful_;
}

} // namespace detail

class TaskQueue {
    friend class Task;
    friend class Awaitable;

    LinkedList<Task> _tasks_all{};

protected:
    Task* _running_task{};
    LinkedListThreadSafe<Task> _tasks_ready;

    TaskQueue() = default;
    JINX_NO_COPY_NO_MOVE(TaskQueue);

    virtual ~TaskQueue() {
        if (not _tasks_all.empty()) {
            error::error("Destroying a non empty task queue");
        }
    }

    virtual EventEngineAbstract* get_event_engine() = 0;

    JINX_NO_DISCARD
    virtual ResultGeneric schedule(Task* task) {
        auto state = task->_state.exchange(ControlState::Ready);
        if (state == ControlState::Ready) {
            return Failed_;
        }
        
        if (state == ControlState::Exited) {
            task->_state = ControlState::Exited;
            return Failed_;
        }

        _tasks_ready.push(task);
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric push(TaskPtr& task) noexcept {
        task.ref();
        task->_task_queue = this;
        return _tasks_all.push_front(task);
    }

    JINX_NO_DISCARD
    ResultGeneric pop(TaskPtr& task) noexcept {
        if (JINX_UNLIKELY(_tasks_all.erase(task).is(Failed_))) {
            return Failed_; 
        }
        
        task->_task_queue = nullptr;
        task.unref();
        return Successful_;
    }

    bool empty() const noexcept {
        return _tasks_all.empty();
    }

    size_t size() const noexcept {
        return _tasks_all.size();
    }

    template<typename OnIdle, typename BeforeRun, typename OnTaskExit>
    inline
    void run(
        const OnIdle& on_idle, 
        const BeforeRun& before_run, 
        const OnTaskExit& on_task_exit) 
    {
        while (not this->empty()) 
        {
            auto* task = _tasks_ready.pop();
            while (task != nullptr) 
            {
                task->_exc = nullptr;
                _running_task = task;

                before_run(task);

                ControlState state{task->run()};
                
                _running_task = nullptr;
                task->_state = state;

                if (JINX_UNLIKELY(state == ControlState::Exited)) {

                    if (on_task_exit(task)) {
                        return;
                    }

                } if (JINX_UNLIKELY(state == ControlState::Yield)) {
                    get_event_engine()->poll();
                    auto* next = _tasks_ready.pop();
                    if (next == nullptr) {
                        continue;
                    }
                    schedule(task) >> JINX_IGNORE_RESULT;
                    task = next;
                    continue;
                }

                task = _tasks_ready.pop();
            }

            if (on_idle()) {
                break;
            }
        }
    }

    void exit() {
        if (_running_task != nullptr) {
            error::error("Exit from a running task");
        }

        _tasks_all.for_each([this](Task* task){
            auto ptr = TaskPtr::shared_from_this(task);
            TaskQueue::cancel(ptr) >> JINX_IGNORE_RESULT;
        });
        while(_tasks_ready.pop() != nullptr) {};
    }

public:
    JINX_NO_DISCARD
    virtual Result<ErrorCancel> cancel(TaskPtr& task) {
        if (_running_task == task) {
            return ErrorCancel::CancelRunningTask;
        }

        while (task->_top != nullptr && task->_top != static_cast<Awaitable*>(task)) {
            auto* caller = task->_top->_caller;
            task->_top->async_finalize();
            task->_top = caller;
        }

        auto state = task->_state.exchange(ControlState::Ready);
        if (state == ControlState::Ready) {
            task->set_error_code({});
            /*
                Task was inserted into _ready_tasks queue. 
                There is no safe way to pop it from _ready_tasks queue.
                So we clear the call stack and just return.
                TaskQueue will pop it in next loop.
            */
            return ErrorCancel::DeferredDequeue;
        }

        task->_state = ControlState::Exited;

        pop(task) >> JINX_IGNORE_RESULT; // ignore result
        return ErrorCancel::NoError;
    }

    JINX_NO_DISCARD
    virtual ResultGeneric spawn(TaskPtr& task) {
        if (push(task).is(Failed_)) {
            return Failed_;
        }
        return schedule(task);
    }
};

class Loop : public TaskQueue {
    typedef error::Error(*ErrorHandlerType)(Loop& loop, TaskPtr& task, const error::Error& error);

    EventEngineAbstract* _event_engine;
    ErrorHandlerType _error_handler{};

public:
    explicit Loop(EventEngineAbstract* event_engine) : _event_engine(event_engine) { }
    
    JINX_NO_DISCARD
    ResultGeneric schedule(Task *task) override {
        if (TaskQueue::schedule(task).is(Failed_)) {
            return Failed_;
        }
        _event_engine->wakeup();
        return Successful_;
    }

    EventEngineAbstract* get_event_engine() override { return _event_engine; }

    void set_error_handler(ErrorHandlerType handler) noexcept {
        _error_handler = handler;
    }

    virtual error::Error unhandled_error(TaskPtr& task) {
        // backtrace
        if (task->_error and task->_exc != nullptr) {
            auto* awaitable = task->_exc;
            while (awaitable != task) {
                awaitable->print_backtrace(std::cerr, 0);
                awaitable = awaitable->_caller;
            }
        }

        const error::Error& error = task->get_error_code();
        if (_error_handler != nullptr) {
            return _error_handler(*this, task, error);
        }
#if defined(__cpp_exceptions) && __cpp_exceptions
        if (error.category() == category_awaitable() 
            and static_cast<ErrorAwaitable>(error.value()) == ErrorAwaitable::CXXException) 
        {
            std::rethrow_exception(task->get_exception());
        }
        throw exception::JinxError(error);
#else
        std::cerr << "unhandled error: " << ec.category().message(ec.value()) << std::endl;
        std::terminate();
#endif
    }

    error::Error run() { // NOLINT(readability-function-cognitive-complexity)
        bool exit_loop = false;
        TaskQueue::run(
            [&]() {
                get_event_engine()->poll();
                return false;
            },
            [](...) { },
            [&](Task* task) {
                auto ptr = TaskPtr::shared_from_this(task);
                if (JINX_UNLIKELY(pop(ptr).is(Failed_))) {
                    error::error("The task is not attached to any queue");
                }

                if (task->_error) {
                    if (task->_error.category() == category_awaitable() and 
                        static_cast<ErrorAwaitable>(task->_error.value()) == ErrorAwaitable::ExitLoop) 
                    {
                        exit_loop = true;
                        return true;
                    }
                    unhandled_error(ptr);
                    return true;
                }
                return false;
            }
        );
        if (exit_loop) {
            exit();
        }
        return {};
    }

    template<
        typename T, 
        typename TaskHeapType=typename T::template TaskHeapType<T>,
        typename... Args>
    TaskPtr task_new(Args&&... args) {
        auto task = TaskHeapType::template new_task<TaskHeapType>();

        if (task == nullptr) {
            return TaskPtr{nullptr};
        }

        auto task_ptr = TaskPtr{task};

        (*task)(std::forward<Args>(args)...);

        spawn(task_ptr) >> JINX_IGNORE_RESULT;
        return task_ptr;
    }

    template<typename T, typename... TArgs>
    JINX_NO_DISCARD
    ResultGeneric task_create(TaskStatic<T>& branch) {
        TaskPtr task{&branch};
        auto result = spawn(task);
        task.release();
        return result;
    }

    JINX_NO_DISCARD
    ResultGeneric task_create(TaskPtr& task) {
        return spawn(task);
    }

    template<typename Config, typename Allocator, typename... TArgs>
    JINX_NO_DISCARD
    ResultGeneric task_allocate(Allocator* allocator, TArgs&&... args) {
        typedef typename Config::TaskType TaskType;
        return TaskType::template allocate<Config>(this, allocator, std::forward<TArgs>(args)...);
    }

    using TaskQueue::exit;
    using TaskQueue::cancel;
};

} // namespace jinx

#endif
