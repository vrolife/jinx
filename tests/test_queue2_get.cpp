#include <jinx/assert.hpp>
#include <iostream>
#include <functional>

#include <jinx/async.hpp>
#include <jinx/queue2.hpp>
#include <jinx/libevent.hpp>

using namespace jinx;

int sum{0};

struct NonJinxInterface : Queue2<std::queue<int>>::CallbackGet, Queue2<std::queue<int>>::CallbackPut
{
    Queue2<std::queue<int>> *_queue{nullptr};

    std::function<void(void)> _cb{};
    int _value{0};

    NonJinxInterface() = default;
    explicit NonJinxInterface(Queue2<std::queue<int>>* queue) :_queue(queue) { }

    void queue2_get(int&& value) override {
        _value = value;
        if (_cb) {
            _cb();
        }
    }
    
    void queue2_cancel_pending_get() override { }

    int queue2_put() override {
        return _value;
    }

    void queue2_cancel_pending_put() override { }

    bool read() {
        return _queue->get(this).unwrap() == Queue2Status::Complete;
    }

    bool write() {
        return _queue->put(this).unwrap() == Queue2Status::Complete;
    }
};

class AsyncNonJinxObject : public AsyncRoutine {
    NonJinxInterface _njo{};

public:
    AsyncNonJinxObject& operator ()(Queue2<std::queue<int>>* queue) {
        _njo._queue = queue;
        async_start(&AsyncNonJinxObject::get);
        return *this;
    }

    Async get() {
        if (not _njo.read()) {
            async_start(&AsyncNonJinxObject::print);
            _njo._cb = [&]() {
                this->async_resume() >> JINX_IGNORE_RESULT;
            };
            return async_suspend();
        }
        return print();
    }

    Async print() {
        std::cout << _njo._value << std::endl;
        if (_njo._value == -1) {
            return async_return();
        }
        sum += _njo._value;
        return get();
    }
};

class AsyncTest2 : public AsyncRoutine {
    Queue2<std::queue<int>> *_queue{nullptr};

    Queue2<std::queue<int>>::Put _put{};

    int i{2};

public:
    AsyncTest2& operator ()(Queue2<std::queue<int>>* queue) {
        _queue = queue;
        async_start(&AsyncTest2::test);
        return *this;
    }

    Async test() {
        return *this / _put(_queue, 1) / &AsyncTest2::_yield;
    }

    Async _yield() {
        return async_yield(&AsyncTest2::put_all);
    }

    Async put_all() {
        if (i == -1) {
            return async_return();
        }
        int val = i ++;
        if (val == 10) {
            val = i = -1;
        }
        return *this / _put(_queue, val) / &AsyncTest2::put_all;
    }
};

int main(int argc, const char* argv[])
{
    libevent::EventEngineLibevent eve(false);
    Loop loop(&eve);

    Queue2<std::queue<int>> queue(2);

    loop.task_new<AsyncNonJinxObject>(&queue);
    loop.task_new<AsyncTest2>(&queue);

    loop.run();

    jinx_assert(sum == 45);
    return 0;
}
