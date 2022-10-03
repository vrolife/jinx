#include <jinx/assert.hpp>
#include <stdexcept>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

class AsyncTest : public AsyncRoutine {
    TaskPtr _task;

public:
    AsyncTest& operator ()() {
        async_start(&AsyncTest::spawn);
        return *this;
    }

    Async spawn() {
        auto task = this->task_new<AsyncDoNothing>();
        jinx_assert(async_cancel(task).unwrap() == ErrorCancel::DeferredDequeue);
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
