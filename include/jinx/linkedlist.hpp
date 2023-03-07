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
#ifndef __jinx_linkedlist_hpp__
#define __jinx_linkedlist_hpp__

#include <type_traits>
#include <stdexcept>
#include <atomic>

#include <jinx/macros.hpp>
#include <jinx/result.hpp>

namespace jinx {


template<typename T>
struct LinkedList {
    struct Node {
        friend class LinkedList<T>;
    protected:
        Node* _prev{nullptr};
        Node* _next{nullptr};
    public:
        Node() = default;
        bool is_inserted() const {
            return _prev != nullptr or _next != nullptr;
        }
    };

    JINX_NO_DISCARD
    ResultGeneric push_front(T* value) noexcept {
        auto node = static_cast<Node*>(value);
        if (node->_next != nullptr or node->_prev != nullptr) {
            return Failed_;
        }

        _head->_prev = node;

        node->_next = _head;
        node->_prev = nullptr;

        _head = node;

        _size += 1;
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric push_back(T* value) noexcept {
        auto node = static_cast<Node*>(value);
        if (node->_next != nullptr or node->_prev != nullptr) {
            return Failed_;
        }
        
        node->_prev = _end._prev;
        node->_next = &_end;
        if (node->_prev) {
            node->_prev->_next = node;
        }
        _end._prev = node;

        _size += 1;
        return Successful_;
    }

    JINX_NO_DISCARD
    ResultGeneric erase(T* value) noexcept {
        auto node = static_cast<Node*>(value);
        if (node->_next == nullptr and node->_prev == nullptr) {
            return Failed_;
        }

        if (node->_prev == nullptr) {
            _head = node->_next;
            _head->_prev = nullptr;
            node->_next = nullptr;
        } else {
            node->_prev->_next = node->_next;
            node->_next->_prev = node->_prev;
            node->_prev = nullptr;
            node->_next = nullptr;
        }

        _size -= 1;
        return Successful_;
    }

    bool empty() const noexcept { return _size == 0; }

    T* front() noexcept {
        return static_cast<T*>(_head);
    }

    T* back() noexcept {
        return static_cast<T*>(_end._prev);
    }

    JINX_NO_DISCARD
    ResultGeneric pop_back() noexcept {
        return erase(back());
    }

    JINX_NO_DISCARD
    ResultGeneric pop_front() noexcept {
        return erase(front());
    }

    void clear() noexcept {
        for_each([this](T* ptr){
            erase(ptr) >> JINX_IGNORE_RESULT;
        });
    }

    template<typename F>
    void for_each(F&& fun) {
        auto* ptr = _head;
        while (ptr != &_end) {
            auto* next = ptr->_next;
            fun(static_cast<T*>(ptr));
            ptr = next;
        }
    }

    size_t size() const noexcept { return _size; }

private:
    Node _end{};
    Node* _head{&_end};
    size_t _size{0};

public:
    LinkedList() = default;
    ~LinkedList() = default;
    JINX_NO_COPY_NO_MOVE(LinkedList);
};

template<typename T>
class LinkedListThreadSafe {
public:
    struct Node {
        Node* _next{};
        Node() = default;
    };
private:
    std::atomic<Node*> _head{nullptr};
    size_t _size{0};
public:
    LinkedListThreadSafe() = default;

    void push(T* value) noexcept {
        auto* new_node = static_cast<Node*>(value);
        auto* old_head = _head.load(std::memory_order_relaxed);

        // compiler bugs
        // see https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
        do {
            new_node->_next = old_head;
        } while(!_head.compare_exchange_weak(old_head, new_node, std::memory_order_release, std::memory_order_relaxed));

        _size += 1;
    }

    T* pop() noexcept {
        auto* old_head = _head.load(std::memory_order_relaxed);
        Node* new_head{nullptr};
        do {
            new_head = old_head ? old_head->_next : nullptr;
        }
        while (!_head.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed));

        if (old_head != nullptr) {
            _size -= 1;
        }
        return static_cast<T*>(old_head);
    }

    size_t get_size_unsafe() const noexcept {
        return _size;
    }

    T* get_top_unsafe() noexcept {
        return _head.load();
    }

    bool is_empty_unsafe() const noexcept {
        return get_top_unsafe() == nullptr;
    }
};

}

#endif
