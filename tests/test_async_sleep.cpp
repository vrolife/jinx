#include <jinx/assert.hpp>
#include <iostream>
#include <chrono>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;

class AsyncTest : public AsyncRoutine {
    async::Sleep _sleep;

    std::chrono::system_clock::time_point _t;
public:
    AsyncTest& operator ()()
    {
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        _t = std::chrono::system_clock::now();
        return async_await(_sleep(std::chrono::milliseconds(100)), &AsyncTest::bar);
    }

    Async bar() {
        const auto t = std::chrono::system_clock::now() - _t;
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t);
        std::cout << ms.count() << std::endl;
        jinx_assert(ms.count() >= 100);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    auto task = loop.task_new<AsyncTest>();

    loop.run();
    return 0;
}
