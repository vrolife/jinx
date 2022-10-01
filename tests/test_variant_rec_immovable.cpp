#include <string>

#include <jinx/async.hpp>
#include <jinx/variant.hpp>
#include <jinx/libevent.hpp>
#include <jinx/macros.hpp>

using namespace jinx;

#ifndef VARIANT
#define VARIANT jinx::VariantRecursive
#endif

typedef AsyncImplement<libevent::EventEngineLibevent> async;

class AsyncTest : public AsyncRoutine {
    VARIANT<async::Sleep, AsyncDoNothing> _var{};

public:
    AsyncTest& operator ()() {
        async_start(&AsyncTest::sleep);
        return *this;
    }

    Async sleep() {
        return *this / _var.emplace<async::Sleep>()(std::chrono::milliseconds(10)) / & AsyncTest::nop;
    }

    Async nop() {
        return *this / _var.emplace<AsyncDoNothing>() / & AsyncTest::async_return;
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
