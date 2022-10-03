#include <jinx/assert.hpp>
#include <iostream>
#include <jinx/async.hpp>
#include <jinx/queue.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

std::string buf;

class AsyncTest1 : public AsyncRoutine {
    Queue<std::queue<int>>* _queue{nullptr};
    Queue<std::queue<int>>::Get _get{};
    Queue<std::queue<int>>::Put _put{};
public:
    AsyncTest1& operator ()(Queue<std::queue<int>>* queue)
    {
        _queue = queue;
        async_start(&AsyncTest1::foo);
        return *this;
    }

    Async foo() {
        buf.append("foo1");
        return async_await(_put(_queue, 1), &AsyncTest1::bar);
    }

    Async bar() {
        buf.append("bar1");
        return async_await(_put(_queue, 1), &AsyncTest1::get);
    }

    Async get() {
        buf.append("get");
        return async_await(_get(_queue), &AsyncTest1::finish);
    }

    Async finish() {
        return async_return();
    }
};

class AsyncTest2 : public AsyncRoutine {
    Queue<std::queue<int>>* _queue{nullptr};
    Queue<std::queue<int>>::Get _get{};
    Queue<std::queue<int>>::Put _put{};
public:
    AsyncTest2& operator ()(Queue<std::queue<int>>* queue)
    {
        _queue = queue;
        async_start(&AsyncTest2::foo);
        return *this;
    }

    Async foo() {
        buf.append("foo2");
        return async_await(_get(_queue), &AsyncTest2::bar);
    }

    Async bar() {
        buf.append("bar2");
        return async_await(_get(_queue), &AsyncTest2::put);
    }

    Async put() {
        buf.append("put");
        return async_await(_put(_queue, 1), &AsyncTest2::finish);
    }

    Async finish() {
        return async_return();
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    Queue<std::queue<int>> queue(1);

    loop.task_new<AsyncTest2>(&queue);
    loop.task_new<AsyncTest1>(&queue);

    loop.run();

    queue.reset();

    std::cout << buf << std::endl;
    jinx_assert(buf == "foo1bar1foo2bar2putget");
    return 0;
}
