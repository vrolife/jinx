#include <jinx/assert.hpp>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncEngine<libevent::EventEngineLibevent> async;

int counter = 0;

class AsyncYield : public AsyncRoutine {
    async::Sleep _sleep2{};

public:
    AsyncYield& operator ()() {
        async_start(&AsyncYield::foo);
        return *this;
    }

    Async foo() {
        return async_yield(&AsyncYield::bar);
    }

    Async bar() {
        ++ counter;
        return *this / _sleep2(std::chrono::milliseconds(10)) / &AsyncYield::async_return;
    }
};

class AsyncTest : public AsyncRoutine {
    enum Tags {
        Sleep1,
        Yield
    };

    Tagged<Tags, async::Sleep> _sleep1{};
    Tagged<Tags, AsyncYield> _raise_error{};
    async::Wait<Tags> _wait{};

public:
    AsyncTest& operator ()() {
        _wait.initialize(std::chrono::milliseconds(1000000), WaitCondition::FirstCompleted);
        _wait.branch_create(_sleep1(std::chrono::milliseconds(20)).set_tag(Sleep1)) >> JINX_IGNORE_RESULT;
        _wait.branch_create(_raise_error().set_tag(Yield)) >> JINX_IGNORE_RESULT;
        async_start(&AsyncTest::wait);
        return *this;
    }

    Async wait() {
        return *this 
            / _wait
            / &AsyncTest::process;
    }

    Async process() {
        ++counter;
        _wait.for_each([&](TaskPtr& task, Tags tag){
            ++counter;
            switch(tag) {
                case Sleep1:
                    ++counter;
                    break;
                case Yield:
                    ++counter;
                    break;
            }
        });
        if (not _wait.is_done()) {
            return wait();
        }
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    loop.task_new<AsyncTest>();
    loop.run();
    jinx_assert(counter == 7);
    return 0;
}
