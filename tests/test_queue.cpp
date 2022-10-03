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
        async_start(&AsyncTest1::put1);
        return *this;
    }

    Async put1() {
        buf.append("put1");
        return async_await(_put(_queue, 1), &AsyncTest1::put2);
    }

    Async put2() {
        buf.append("put2");
        return async_await(_put(_queue, 1), &AsyncTest1::put3);
    }

    Async put3() {
        buf.append("put3");
        return async_await(_put(_queue, 1), &AsyncTest1::finish);
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
        async_start(&AsyncTest2::get1);
        return *this;
    }

    Async get1() {
        buf.append("get1");
        return async_await(_get(_queue), &AsyncTest2::get2);
    }

    Async get2() {
        buf.append("get2");
        return async_await(_get(_queue), &AsyncTest2::get3);
    }

    Async get3() {
        buf.append("get3");
        return async_await(_get(_queue), &AsyncTest2::finish);
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
    jinx_assert(buf == "put1put2get1get2get3put3");
    return 0;
}
