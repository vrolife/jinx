#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

class AsyncTest : public AsyncRoutine {
    AsyncFuture<void>* _future1{};
    AsyncFuture<void>* _future2{};
public:
    AsyncTest& operator ()(AsyncFuture<void>* future1, AsyncFuture<void>* future2)
    {
        _future1 = future1;
        _future2 = future2;
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        _future1->emplace_result() >> JINX_IGNORE_RESULT;
        return async_await((*_future2), &AsyncTest::bar);
    }

    Async bar() {
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    AsyncFuture<void> future1{};
    AsyncFuture<void> future2{};

    loop.task_new<AsyncTest>(&future1, &future2);
    loop.task_new<AsyncTest>(&future2, &future1);

    loop.run();
    return 0;
}
