#include <jinx/assert.hpp>
#include <stdexcept>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

int counter = 0;

class AsyncSuspend : public AsyncRoutine {
    typedef AsyncRoutine BaseType;
public:
    AsyncSuspend& operator ()() {
        async_start(&AsyncSuspend::suspend);
        return *this;
    }

    Async suspend() {
        counter += 1;
        return this->async_suspend();
    }

    Async exit() {
        ::abort();
        return this->async_return();
    }
};

class AsyncTest : public AsyncRoutine {
    TaskPtr _task;

public:
    AsyncTest& operator ()() {
        async_start(&AsyncTest::test);
        return *this;
    }

    Async test() {
        _task = this->task_new<AsyncSuspend>();
        return async_yield(&AsyncTest::exit);
    }

    Async exit() {
        jinx_assert(counter == 1);
        jinx_assert(async_cancel(_task).unwrap() == ErrorCancel::NoError);
        _task.reset();
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    loop.run();
    return 0;
}
