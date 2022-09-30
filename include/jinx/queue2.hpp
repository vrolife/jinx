/*
Copyright (C) 2022  pom@vro.life

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    class GetNode;
    class PutNode;

    class Get;
    class Put;
    typedef typename Container::value_type ValueType;

private:
    friend class Get;
    friend class Put;

    LinkedList<GetNode> _pending_get;
    LinkedList<PutNode> _pending_put;

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
        for (auto& get : _pending_get) {
            get.cancel_get();
        }
        _pending_get.clear();
        for (auto& put : _pending_put) {
            put.cancel_put();
        }
        _pending_put.clear();
        BaseType::reset();
    }

    JINX_NO_DISCARD
    Result<Queue2Status> get(GetNode* node) {
        if (empty()) {
            if (not _pending_put.empty()) {
                auto* put = this->_pending_put.back();
                node->queue_get(put->queue_put());
                this->_pending_put.pop_back() >> JINX_IGNORE_RESULT;
                return Queue2Status::Complete;
            }

            if (_pending_get.push_back(node).is(Faileb)) {
                return Queue2Status::Error;
            }
            return Queue2Status::Pending;
        }

        node->queue_get(std::move(front()));
        pop();
        return Queue2Status::Complete;
    }

    JINX_NO_DISCARD
    Result<Queue2Status> put(PutNode* node) {
        while (not _pending_get.empty())
        {
            auto* get = this->_pending_get.back();
            this->_pending_get.pop_back() >> JINX_IGNORE_RESULT;
            if (empty()) {
                get->queue_get(node->queue_put());
                return Queue2Status::Complete;
            }
        
            get->queue_get(std::move(front()));
            pop();
        }
        if (full()) {
            if (_pending_put.push_back(node).is(Faileb)) {
                return Queue2Status::Error;
            }
            return Queue2Status::Pending;
        }
        emplace(node->queue_put());
        return Queue2Status::Complete;
    }
};

template<typename Container>
class Queue2<Container>::GetNode : public  LinkedList<GetNode>::Node
{
    friend Queue2<Container>;
    typedef typename Container::value_type ValueType;
protected:
    virtual void queue_get(ValueType&& value) = 0;
    virtual void cancel_get() = 0;
};

template<typename Container>
class Queue2<Container>::PutNode : public  LinkedList<PutNode>::Node
{
    friend Queue2<Container>;
    typedef typename Container::value_type ValueType;
protected:
    virtual ValueType queue_put() = 0;
    virtual void cancel_put() = 0;
};

template<typename Container>
class Queue2<Container>::Get
: public AsyncFuture<typename Container::value_type>,
  private GetNode
{
    friend class LinkedList<GetNode>;
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

    void queue_get(ValueType&& value) override {
        this->emplace_result(std::move(value)) >> JINX_IGNORE_RESULT;
    }

    void cancel_get() override {
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
  private PutNode
{
    
    friend class LinkedList<PutNode>;
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

    ValueType queue_put() override {
        this->emplace_result() >> JINX_IGNORE_RESULT;
        return std::move(_value.release());
    }

    void cancel_put() override {
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
