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
        reset();
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
        if (_end == &reinterpret_cast<T*>(_memory)[N]) {
            _end = reinterpret_cast<T*>(_memory);
        }

        new (_end)T(std::forward<Args>(args)...);

        ++_end;
        ++_size;
    }

    void pop() {
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

protected:
    using BaseType::emplace;
    using BaseType::pop;
    using BaseType::front;

public:
    using BaseType::QueueBase;
    using BaseType::empty;
    using BaseType::full;
    using BaseType::get_max_size;
    using BaseType::set_max_size;

    void size() const {
        return BaseType::size() + _pending_put.size();
    }

    void reset() noexcept {
        _pending_get.for_each([](Get* get){
            get->async_resume(make_error(ErrorAwaitable::Cancelled)) >> JINX_IGNORE_RESULT;
        });
        
        _pending_put.for_each([](Put* put){
            put->async_resume(make_error(ErrorAwaitable::Cancelled)) >> JINX_IGNORE_RESULT;
        });
        
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

            if (_queue->_pending_get.push_back(this).is(Failed_)) {
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
        if (not _queue->_pending_get.empty()) 
        {
            jinx_assert(_queue->empty());

            auto* get = _queue->_pending_get.back();
            _queue->_pending_get.pop_back() >> JINX_IGNORE_RESULT;
        
            get->emplace_result(std::move(_value.get())) >> JINX_IGNORE_RESULT;
            this->emplace_result() >> JINX_IGNORE_RESULT;;
            return async_return();
        }
        
        if (_queue->full()) {
            if (static_cast<bool>(_queue->_pending_put.push_back(this).is(Failed_))) {
                return async_throw(ErrorQueue::PendingPutError);
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
