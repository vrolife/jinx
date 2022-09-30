#include <jinx/assert.hpp>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;

int counter = 0;

class AsyncTest : public AsyncRoutine {
    enum Tags {
        Sleep1,
        Sleep2
    };

    Tagged<Tags, async::Sleep> _sleep1{};
    Tagged<Tags, async::Sleep> _sleep2{};
    async::Wait<Tags> _wait{};
public:
    AsyncTest& operator ()() {
        _wait.initialize(std::chrono::milliseconds(10), WaitCondition::FirstCompleted);
        _wait.branch_create(_sleep1(std::chrono::milliseconds(100)).set_tag(Sleep1)) >> JINX_IGNORE_RESULT;
        _wait.branch_create(_sleep2(std::chrono::seconds(99999)).set_tag(Sleep2)) >> JINX_IGNORE_RESULT;
        async_start(&AsyncTest::wait);
        return *this;
    }

protected:

    Async wait() {
        return *this 
            / _wait
            / &AsyncTest::process;
    }

    Async process() {
        _wait.for_each([&](TaskPtr& task, Tags tag){
            jinx_assert(tag == Sleep1);
        });
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
    jinx_assert(counter == 1);
    return 0;
}
