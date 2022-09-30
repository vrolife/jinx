#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

static int counter = 0;

class AsyncFoo : public AsyncRoutine {
public:
    AsyncFoo& operator ()() {
        async_start(&AsyncFoo::foo);
        return *this;
    }

    Async foo() {
        counter ++;
        return async_return();
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
        counter += 1;
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);

    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    loop.run();
    assert(counter == 2);
    return 0;
}
