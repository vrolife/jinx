#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/lock.hpp>

using namespace jinx;

int counter = 0;

class AsyncFoo : public AsyncRoutine {
    lock::LockPtr _lock{};

    lock::AsyncLock _async_lock{};

public:
    AsyncFoo& operator ()(lock::LockPtr&& lock) {
        _lock = lock;
        async_start(&AsyncFoo::foo);
        return *this;
    }

protected:
    Async foo() {
        return *this / _async_lock(_lock) / &AsyncFoo::yield;
    }

    Async yield() {
        return this->async_yield(&AsyncFoo::finish);
    }

    Async finish() {
        jinx_assert(counter == 1);
        counter += 1;
        _lock->unlock();
        return this->async_return();
    }
};

class AsyncBar : public AsyncRoutine {
    lock::LockPtr _lock{};

    lock::AsyncLock _async_lock{};
    
public:
    AsyncBar& operator ()(lock::LockPtr&& lock) {
        _lock = lock;
        async_start(&AsyncBar::bar);
        return *this;
    }

    Async bar() {
        return *this / _async_lock(_lock) / &AsyncBar::yield;
    }

    Async yield() {
        return this->async_yield(&AsyncBar::finish);
    }

    Async finish() {
        jinx_assert(counter == 0);
        counter += 1;
        _lock->unlock();
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    lock::Lock lock{};
    loop.task_new<AsyncFoo>(jinx::lock::LockPtr::shared_from_this(&lock));
    loop.task_new<AsyncBar>(jinx::lock::LockPtr::shared_from_this(&lock));

    loop.run();

    jinx_assert(counter == 2);
    return 0;
}
