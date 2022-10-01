#include <jinx/assert.hpp>
#include <iostream>
#include <chrono>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

enum class MyEnum {
    NoError
};

typedef AsyncImplement<libevent::EventEngineLibevent> async;

class AsyncSleepTest : public AsyncRoutine {
    async::Sleep _sleep;

public:
    AsyncSleepTest& operator ()()
    {
        async_start(&AsyncSleepTest::foo);
        return *this;
    }

    Async foo() {
        return async_await(_sleep(std::chrono::seconds(100)), &AsyncSleepTest::bar);
    }

    Async bar() {
        return async_return();
    }
};

class AsyncThrow : public AsyncRoutine {
public:
    AsyncThrow& operator ()()
    {
        async_start(&AsyncThrow::foo);
        return *this;
    }

    Async foo() {
        return async_yield(&AsyncThrow::bar);
    }
    
    Async bar() {
        return async_throw(posix::ErrorPosix::InvalidArgument);
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    loop.task_new<AsyncThrow>();
    loop.task_new<AsyncSleepTest>();
    try {
        loop.run();
    } catch(...) {
        loop.exit();
    }
    return 0;
}
