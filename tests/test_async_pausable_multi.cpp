#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

static int counter = 0;

class AsyncFoo : public AsyncRoutinePausable {
public:
    AsyncFoo& operator ()() {
        async_start(&AsyncFoo::foo);
        return *this;
    }

    Async foo() {
        async_start(&AsyncFoo::reentry);
        return async_pause();
    }

    Async reentry() {
        jinx_assert(counter == 0);
        return this->async_return();
    }

    void async_finalize() noexcept override {
        counter += 1;
        AsyncRoutinePausable::async_finalize();
    }
};

class AsyncTest : public AsyncRoutine {
    AsyncFoo _foo1{};
    AsyncFoo _foo2{};
public:
    AsyncTest& operator ()()
    {
        async_start(&AsyncTest::foo1);
        return *this;
    }

    Async foo1() {
        return *this / _foo1() / &AsyncTest::foo2;
    }

    Async foo2() {
        return *this / _foo2() / &AsyncTest::reentry;
    }

    Async reentry() {
        jinx_assert(counter == 0);
        return *this / _foo1 / &AsyncTest::bar;
    }

    Async bar() {
        jinx_assert(counter == 1);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);

    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    loop.run();
    jinx_assert(counter = 2);
    return 0;
}
