#include <jinx/assert.hpp>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

class AsyncTest : public AsyncRoutine {
public:
    AsyncTest& operator ()()
    {
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        return async_yield(&AsyncTest::bar);
    }

    Async bar() {
        return async_throw(async_cancel(get_task()).unwrap());
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    bool flag = false;
    try {
        loop.run();
    } catch(const exception::JinxError& ec) {
        jinx_assert(ec.code().as<ErrorCancel>() == ErrorCancel::CancelRunningTask);
        flag = true;
    }
    jinx_assert(flag);
    return 0;
}
