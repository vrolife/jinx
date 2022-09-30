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
        return async_pause();
    }

    void async_finalize() noexcept override {
        counter += 1;
        AsyncRoutinePausable::async_finalize();
    }
};

class AsyncTest : public AsyncRoutine {
    AsyncFoo _foo{};
public:
    AsyncTest& operator ()()
    {
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        return *this / _foo() / &AsyncTest::bar;
    }

    Async bar() {
        jinx_assert(counter == 0);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);

    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    loop.run();
    jinx_assert(counter = 1);
    return 0;
}
