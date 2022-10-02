#include <jinx/assert.hpp>
#include <iostream>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef AsyncImplement<libevent::EventEngineLibevent> async;

int counter = 0;

class WorkerTest : public AsyncRoutine {
    enum Tags {
        Sleep1,
        Sleep2
    };

    Tagged<Tags, async::Sleep> _sleep1{};
    Tagged<Tags, async::Sleep> _sleep2{};
    async::Wait<Tags> _wait{};
public:
    WorkerTest& operator ()() {
        _wait.initialize(WaitCondition::AllCompleted);
        _wait.branch_create(_sleep1(std::chrono::milliseconds(5000)).set_tag(Sleep1)) >> JINX_IGNORE_RESULT;
        _wait.branch_create(_sleep2(std::chrono::milliseconds(5000)).set_tag(Sleep2)) >> JINX_IGNORE_RESULT;
        async_start(&WorkerTest::wait);
        return *this;
    }

    Async wait() {
        return *this 
            / _wait
            / &WorkerTest::process;
    }

    Async process() {
        ++counter;
        _wait.for_each([&](TaskPtr&, Tags tag){
            ++counter;
            switch(tag) {
                case Sleep1:
                    ++counter;
                    break;
                case Sleep2:
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

class WorkerTest2 : public AsyncRoutine {
    TaskPtr _task{};
public:
    WorkerTest2& operator ()(TaskPtr& task) {
        _task = task;
        async_start(&WorkerTest2::foo);
        return *this;
    }

    Async foo() {
        return async_yield(&WorkerTest2::bar);
    }

    Async bar() {
        jinx_assert(async_cancel(_task).unwrap() == ErrorCancel::NoError);
        return this->async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);
    auto task = loop.task_new<WorkerTest>();
    loop.task_new<WorkerTest2>(task);
    loop.run();
    jinx_assert(counter == 0);
    return 0;
}
