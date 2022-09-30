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
#ifndef __jinx_container_hpp__
#define __jinx_container_hpp__

#include <climits>
#include <cstddef>

#include <queue>
#include <array>
#include <utility>

#include "jinx/async.hpp"
#include <jinx/linkedlist.hpp>
#include <jinx/optional.hpp>

namespace jinx {
namespace detail {

template<typename Container>  
class QueueBase
{
public:
    typedef typename Container::value_type ValueType;
private:

    long _max_size{0};
    Container _container{};

public:
    QueueBase() : _max_size(SSIZE_MAX) { }
    explicit QueueBase(long max_size) : _max_size(max_size) { }

    size_t size() const {
        return _container.size();
    }

    size_t get_max_size() const { return _max_size; }
    void set_max_size(size_t size) { _max_size = size; }

    bool empty() const { return _container.empty(); }

    bool full() const {
        return this->_max_size > 0 and this->_container.size() >= this->_max_size;
    }

    ValueType& front() {
        return _container.front();
    }

    template<typename...Args>
    void emplace(Args&&... args) {
        _container.emplace(std::forward<Args>(args)...);
    }

    void pop() {
        _container.pop();
    }

    void reset() noexcept {
        _container = Container{};
    }
};

template<typename T, size_t N>
class QueueBase<std::array<T, N>> {
public:
    typedef T ValueType;
private:

    alignas(T)
    char _memory[sizeof(std::array<T, N>)]{};

    T* _begin{reinterpret_cast<T*>(_memory)};
    T* _end{_begin};
    size_t _size{0};

public:
    QueueBase() = default;
    JINX_NO_COPY_NO_MOVE(QueueBase);
    ~QueueBase() {
        while(size()) {
            pop();
        }
    }

    size_t size() const {
        return _size;
    }

    size_t get_max_size() const { return N; }
    void set_max_size(size_t size) { }

    bool empty() const {
        return size() == 0;
    }

    bool full() const {
        const size_t size = this->size();
        return size > 0 and size == N;
    }

    T& front() {
        return *_begin;
    }

    template<typename...Args>
    void emplace(Args&&... args) {
        if (full()) {
            return;
        }

        if (_end == &reinterpret_cast<T*>(_memory)[N]) {
            _end = reinterpret_cast<T*>(_memory);
        }

        new (_end)T(std::forward<Args>(args)...);

        ++_end;
        ++_size;
    }

    void pop() {
        if (empty()) {
            return;
        }

        reinterpret_cast<T*>(_begin)->~T();

        -- _size;
        ++_begin;

        if (_begin == &reinterpret_cast<T*>(_memory)[N]) {
            _begin = reinterpret_cast<T*>(_memory);
        }
    }

    void reset() noexcept {
        while(size()) {
            pop();
        }
    }
};
    
} // namespace detail

enum class ErrorQueue {
    NoError,
    PendingPutError,
    PendingGetError
};

JINX_ERROR_DEFINE(queue, ErrorQueue);

template<typename Container>
class Queue : private detail::QueueBase<Container> {
    typedef detail::QueueBase<Container> BaseType;

public:
    class Get;
    class Put;
    typedef typename Container::value_type ValueType;

private:
    friend class Get;
    friend class Put;

    LinkedList<Get> _pending_get{};
    LinkedList<Put> _pending_put{};

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
            get.set_error(make_error(ErrorAwaitable::Cancelled));
        }
        for (auto& put : _pending_put) {
            put.set_error(make_error(ErrorAwaitable::Cancelled));
        }
        _pending_get.clear();
        _pending_put.clear();

        BaseType::reset();
    }
};

template<typename Container>
class Queue<Container>::Get
: public AsyncFuture<typename Container::value_type>,
  private LinkedList<Get>::Node
{
    friend class LinkedList<Get>;
    typedef typename Container::value_type ValueType;
    typedef Queue<Container> QueueType;

    Queue<Container>* _queue{nullptr};

protected:
    void async_finalize() noexcept override {
        if (this->is_inserted()) {
            _queue->_pending_get.erase(this) >> JINX_IGNORE_RESULT;
            _queue = nullptr;
        }
        AsyncFuture<ValueType>::async_finalize();
    }

public:
    Get() = default;
    JINX_NO_COPY_NO_MOVE(Get);
    ~Get() override = default;

    Get& operator() (QueueType* queue) 
    {
        this->reset();
        _queue = queue;
        return *this;
    }

    Async async_poll() override {
        if (not this->empty()) {
            return this->async_return();
        }

        if (_queue->empty()) {
            if (not _queue->_pending_put.empty()) 
            {
                auto* put = _queue->_pending_put.back();
                _queue->_pending_put.pop_back() >> JINX_IGNORE_RESULT;

                this->emplace_result(std::move(put->_value.get())) >> JINX_IGNORE_RESULT;
                put->emplace_result() >> JINX_IGNORE_RESULT;
                return this->async_return();
            }

            if (_queue->_pending_get.push_back(this).is(Faileb)) {
                return this->async_throw(ErrorQueue::PendingGetError);
            }

            return this->async_suspend();
        }

        this->emplace_result(std::move(_queue->front())) >> JINX_IGNORE_RESULT;
        _queue->pop();
        return this->async_return();
    }
};

template<typename Container>
class Queue<Container>::Put
: public AsyncFuture<void>,
  private LinkedList<Put>::Node
{
    friend class LinkedList<Put>;
    friend class Queue<Container>::Get;
    
    typedef typename Container::value_type ValueType;
    typedef Queue<Container> QueueType;

    Queue<Container>* _queue{nullptr};
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

public:
    Put() = default;
    JINX_NO_COPY_NO_MOVE(Put);
    ~Put() override = default;

    template<typename V>
    Put& operator() (QueueType* queue, V&& value) 
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

        /*
            Flush queue
            TODO limit loop ?
        */
        while (not _queue->_pending_get.empty()) 
        {
            auto* get = _queue->_pending_get.back();
            _queue->_pending_get.pop_back() >> JINX_IGNORE_RESULT;
            if (_queue->empty()) {
                get->emplace_result(std::move(_value.get()));
                this->emplace_result();
                return async_return();
            }
            get->emplace_result(_queue->front());
            _queue->pop();
        }
        
        if (_queue->full()) {
            if (static_cast<bool>(_queue->_pending_put.push_back(this).is(Faileb))) {
                return async_throw(ErrorQueue::PendingPutError);;
            }
            return this->async_suspend();

        }

        _queue->emplace(std::move(_value.get()));
        this->emplace_result() >> JINX_IGNORE_RESULT;
        return async_return();
    }
};

} // namespace jinx

#endif
