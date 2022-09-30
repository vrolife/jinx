#include <thread>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>
#include <jinx/lock.hpp>

using namespace jinx;

int counter = 0;

static volatile bool sync_thread = false;

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
        sync_thread = false;
        return *this / _async_lock(_lock) / &AsyncFoo::finish;
    }

    Async finish() {
        jinx_assert(counter == 1);
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

    std::thread thread{[&](){
        lock.lock();
        sync_thread = true;
        while (sync_thread) { }
        timespec tim{
            .tv_sec = 0,
            .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::milliseconds(100)).count()
        };
        nanosleep(&tim, nullptr);
        jinx_assert(counter == 0);
        counter += 1;
        lock.unlock();
        sync_thread = false;
    }};
    thread.detach();

    while (!sync_thread) { }

    loop.run();

    jinx_assert(counter == 2);
    return 0;
}
