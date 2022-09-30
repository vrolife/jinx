#include <jinx/assert.hpp>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

int counter = 0;

class AsyncTest1 : public AsyncRoutine {
public:
    AsyncTest1& operator ()()
    {
        async_start(&AsyncTest1::foo);
        return *this;
    }

    Async foo() {
        counter += 1;
        return async_return();
    }
};

class AsyncTest2 : public AsyncRoutine {
    AsyncJoin _join{};

public:
    AsyncTest2& operator ()()
    {
        async_start(&AsyncTest2::foo);
        return *this;
    }

    Async foo() {
        auto task = this->task_new<AsyncTest1>();
        return async_await(_join(task), &AsyncTest2::finish);
    }

    Async finish() {
        jinx_assert(counter == 1);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    loop.task_new<AsyncTest2>();
    loop.run();
    return 0;
}
