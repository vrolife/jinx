/*
MIT License

Copyright (c) 2023 pom@vro.life

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef __jinx_queue2_hpp__
#define __jinx_queue2_hpp__

#include <climits>
#include <cstddef>

#include <queue>
#include <utility>

#include <jinx/async.hpp>
#include <jinx/linkedlist.hpp>
#include <jinx/optional.hpp>
#include <jinx/queue.hpp>

namespace jinx {

enum class Queue2Status {
    Complete,
    Pending,
    Error
};

template<typename Container>
class Queue2 : private detail::QueueBase<Container> 
{
    typedef detail::QueueBase<Container> BaseType;

public:
    class CallbackGet;
    class CallbackPut;

    class Get;
    class Put;
    typedef typename Container::value_type ValueType;

private:
    friend class Get;
    friend class Put;

    LinkedList<CallbackGet> _pending_get;
    LinkedList<CallbackPut> _pending_put;

public:
    using BaseType::QueueBase;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::front;
    using BaseType::emplace;
    using BaseType::pop;
    using BaseType::get_max_size;
    using BaseType::set_max_size;

    void size() const {
        return BaseType::size() + _pending_put.size();
    }

    void reset() noexcept {
        _pending_get.for_each([](CallbackGet* node){
            node->queue2_cancel_pending_get();
        });
        _pending_get.clear();

        _pending_put.for_each([](CallbackPut* node){
            node->queue2_cancel_pending_put();
        });
        _pending_put.clear();

        BaseType::reset();
    }

    JINX_NO_DISCARD
    Result<Queue2Status> get(CallbackGet* node) {
        if (empty()) {
            if (not _pending_put.empty()) {
                auto* put = this->_pending_put.back();
                node->queue2_get(put->queue2_put());
                this->_pending_put.pop_back() >> JINX_IGNORE_RESULT;
                return Queue2Status::Complete;
            }

            if (_pending_get.push_back(node).is(Failed_)) {
                return Queue2Status::Error;
            }
            return Queue2Status::Pending;
        }

        node->queue2_get(std::move(front()));
        pop();
        return Queue2Status::Complete;
    }

    JINX_NO_DISCARD
    Result<Queue2Status> put(CallbackPut* node) {
        while (not _pending_get.empty())
        {
            auto* get = this->_pending_get.back();
            this->_pending_get.pop_back() >> JINX_IGNORE_RESULT;
            if (empty()) {
                get->queue2_get(node->queue2_put());
                return Queue2Status::Complete;
            }
        
            get->queue2_get(std::move(front()));
            pop();
        }
        if (full()) {
            if (_pending_put.push_back(node).is(Failed_)) {
                return Queue2Status::Error;
            }
            return Queue2Status::Pending;
        }
        emplace(node->queue2_put());
        return Queue2Status::Complete;
    }
};

template<typename Container>
class Queue2<Container>::CallbackGet : public  LinkedList<CallbackGet>::Node
{
    friend Queue2<Container>;
    typedef typename Container::value_type ValueType;
protected:
    virtual void queue2_get(ValueType&& value) = 0;
    virtual void queue2_cancel_pending_get() = 0;
};

template<typename Container>
class Queue2<Container>::CallbackPut : public  LinkedList<CallbackPut>::Node
{
    friend Queue2<Container>;
    typedef typename Container::value_type ValueType;
protected:
    virtual ValueType queue2_put() = 0;
    virtual void queue2_cancel_pending_put() = 0;
};

template<typename Container>
class Queue2<Container>::Get
: public AsyncFuture<typename Container::value_type>,
  private CallbackGet
{
    friend class LinkedList<CallbackGet>;
    typedef typename Container::value_type ValueType;
    typedef Queue2<Container> Queue2Type;
    Queue2<Container>* _queue{nullptr};

protected:
    void async_finalize() noexcept override {
        if (this->is_inserted()) {
            _queue->_pending_get.erase(this) >> JINX_IGNORE_RESULT;
            _queue = nullptr;
        }
        AsyncFuture<ValueType>::async_finalize();
    }

    void queue2_get(ValueType&& value) override {
        this->emplace_result(std::move(value)) >> JINX_IGNORE_RESULT;
    }

    void queue2_cancel_pending_get() override {
        this->set_error(make_error(
            ErrorAwaitable::Cancelled));
    }

public:
    Get() = default;
    JINX_NO_COPY_NO_MOVE(Get);
    ~Get() override = default;

    Get& operator() (Queue2Type* queue) 
    {
        this->reset();
        _queue = queue;
        return *this;
    }

    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }
        switch (_queue->get(this).unwrap()) {
            case Queue2Status::Error:
                return this->async_throw(ErrorQueue::PendingGetError);
            case Queue2Status::Pending:
                return this->async_suspend();
            case Queue2Status::Complete:
                return this->async_return();
        }
    }
};

template<typename Container>
class Queue2<Container>::Put
: public AsyncFuture<void>,
  private CallbackPut
{
    
    friend class LinkedList<CallbackPut>;
    friend class Queue2<Container>::Get;

    typedef typename Container::value_type ValueType;
    typedef Queue2<Container> Queue2Type;

    Queue2<Container>* _queue{nullptr};
    Optional<ValueType> _value{};

protected:
    void async_finalize() noexcept override {
        if (this->is_inserted()) {
            _queue->_pending_put.erase(this) >> JINX_IGNORE_RESULT;
            _queue = nullptr;
        }
        _value.reset();
        AsyncFuture<void>::async_finalize();
    }

    ValueType queue2_put() override {
        this->emplace_result() >> JINX_IGNORE_RESULT;
        return std::move(_value.release());
    }

    void queue2_cancel_pending_put() override {
        this->set_error(make_error(ErrorAwaitable::Cancelled));
    }

public:
    Put() = default;
    JINX_NO_COPY_NO_MOVE(Put);
    ~Put() override = default;

    template<typename V>
    Put& operator() (Queue2Type* queue, V&& value) 
    {
        this->reset();
        _queue = queue;
        _value.emplace(std::forward<V>(value));
        return *this;
    }

    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }
        switch (_queue->put(this).unwrap()) {
            case Queue2Status::Error:
                return async_throw(ErrorQueue::PendingPutError);
            case Queue2Status::Pending:
                return this->async_suspend();
            case Queue2Status::Complete:
                return this->async_return();
        }
    }
};

} // namespace jinx

#endif
