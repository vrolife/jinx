#include <jinx/assert.hpp>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;
typedef AsyncEngine<libevent::EventEngineLibevent> async;

int counter = 0;

class AsyncYield : public AsyncRoutine {
public:
    AsyncYield& operator ()() {
        async_start(&AsyncYield::foo);
        return *this;
    }

    Async foo() {
        return async_throw(posix::ErrorPosix::InvalidArgument);
    }
};

class AsyncTest : public AsyncRoutine {
    enum Tags {
        Sleep1,
        Raise
    };

    Tagged<Tags, async::Sleep> _sleep1{};
    Tagged<Tags, AsyncYield> _raise_error{};
    async::Wait<Tags> _wait{};

public:
    AsyncTest& operator ()() {
        _wait.initialize(std::chrono::milliseconds(1000000), WaitCondition::AllCompleted);
        _wait.branch_create(_sleep1(std::chrono::milliseconds(20)).set_tag(Sleep1)) >> JINX_IGNORE_RESULT;
        _wait.branch_create(_raise_error().set_tag(Raise)) >> JINX_IGNORE_RESULT;

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
        ++counter;
        _wait.for_each([&](TaskPtr& task, Tags tag){
            ++counter;
            switch(tag) {
                case Sleep1:
                    ++counter;
                    break;
                case Raise:
                    ++counter;
                    jinx_assert(task->get_error_code());
                    jinx_assert(task->get_error_code().value() == static_cast<int>(std::errc::invalid_argument));
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
    jinx_assert(counter == 5);
    return 0;
}
