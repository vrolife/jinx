#include <jinx/assert.hpp>
#include <iostream>

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
        if (_cb) {
            _cb();
        }
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
    int _i{1};

public:
    AsyncNonJinxObject& operator ()(Queue2<std::queue<int>>* queue) {
        _njo._queue = queue;
        _i = 1;
        async_start(&AsyncNonJinxObject::put);
        return *this;
    }

    Async put() 
    {
        while(true) {
            int val = _i;
            ++_i;
            if (val == 10) {
                return async_return();
            }
            _njo._value = val;
            if (not _njo.write()) {
                _njo._cb = [&]() {
                    this->async_resume() >> JINX_IGNORE_RESULT;
                };
                return async_suspend();
            }
        }
    }
};

class AsyncTest2 : public AsyncRoutine {
    Queue2<std::queue<int>> *_queue{nullptr};

    Queue2<std::queue<int>>::Get _get{};

public:
    AsyncTest2& operator ()(Queue2<std::queue<int>>* queue) {
        _queue = queue;
        async_start(&AsyncTest2::_yield);
        return *this;
    }

protected:
    void async_finalize() noexcept override {
        _queue->reset();
        return AsyncRoutine::async_finalize();
    }
    Async _yield() {
        return async_yield(&AsyncTest2::get_all);
    }

    Async get_all() {
        return *this / _get(_queue) / &AsyncTest2::print;
    }

    Async print() {
        std::cout << _get.get_result() << std::endl;
        sum += _get.get_result();
        if (_get.get_result() == 9) {
            return this->async_return();
        }
        return get_all();
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
