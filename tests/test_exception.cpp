#include <jinx/assert.hpp>
#include <stdexcept>

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
        throw std::invalid_argument("test");
// LCOV_EXCL_START
// never reach
        return bar();
    }

    Async bar() {
        return async_return();
    }
// LCOV_EXCL_STOP
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    loop.task_new<AsyncTest>();

    bool flag = false;

    try {
        loop.run();
    } catch (const std::invalid_argument& e) {
        flag = true;
    }
    jinx_assert(flag);
    return 0;
}
