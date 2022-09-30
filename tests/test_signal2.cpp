#include <csignal>
#include <jinx/assert.hpp>
#include <iostream>
#include <chrono>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;

typedef libevent::EventEngineLibevent EventEngine;

class AsyncService : public AsyncRoutine {
    async::Sleep _sleep;
public:
    AsyncService& operator ()()
    {
        async_start(&AsyncService::foo);
        return *this;
    }

    Async foo() {
        kill(getpid(), SIGUSR1);
        return async_await(_sleep(std::chrono::seconds(1)), &AsyncService::bar);
    }

// LCOV_EXCL_START
// never reach
    Async bar() {
        ::abort();
        return async_return();
    }
// LCOV_EXCL_STOP
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    auto task = loop.task_new<AsyncService>();

    EventEngine::EventHandleSignalFunctional sigint;

    bool flag = false;

    eve.add_signal(sigint, SIGUSR1, [&](const error::Error& error){
        eve.remove_signal(sigint);

        std::cout << "exit\n";
        flag = true;
        loop.cancel(task) >> JINX_IGNORE_RESULT;
        eve.wakeup();
    });

    loop.run();

    jinx_assert(flag);
    return 0;
}
