#include <csignal>
#include <jinx/assert.hpp>
#include <iostream>
#include <chrono>

#include <jinx/async.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

typedef libevent::EventEngineLibevent EventEngine;

static int counter = 0;

class AsyncTest : public AsyncRoutine {
    EventEngine::EventHandleSignal _signal{};
public:
    AsyncTest& operator ()()
    {
        async_start(&AsyncTest::prepare);
        return *this;
    }

    Async prepare() {
        auto* eve = this->template get_event_engine<EventEngine>();
        eve->add_signal(_signal, SIGUSR1, &AsyncTest::on_signal, this).abort_on(Failed_, "register signal event failed");
        async_start(&AsyncTest::exit);
        return this->async_suspend();
    }

    Async exit() {
        counter ++;
        return async_return();
    }

    static void on_signal(const error::Error& error, void* data) {
        auto* self = reinterpret_cast<AsyncTest*>(data);
        auto* eve = self->template get_event_engine<EventEngine>();
        eve->remove_signal(self->_signal) >> JINX_IGNORE_RESULT;
        self->async_resume(error) >> JINX_IGNORE_RESULT;
        counter ++;
    }
};

class AsyncKill : public AsyncRoutine {
public:
    AsyncKill& operator ()() {
        async_start(&AsyncKill::yield);
        return *this;
    }

    Async yield() {
        return this->async_yield(&AsyncKill::kill);
    }

    Async kill() {
        counter ++;
        ::kill(getpid(), SIGUSR1);
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    loop.task_new<AsyncTest>();
    loop.task_new<AsyncKill>();

    loop.run();
    jinx_assert(counter == 3);
    return 0;
}
