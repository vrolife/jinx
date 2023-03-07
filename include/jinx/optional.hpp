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
#ifndef __jinx_optional_hpp__
#define __jinx_optional_hpp__

#include <utility>

namespace jinx
{

template<typename T>
class Optional
{
    alignas(T) char _memory[sizeof(T)]{};
    bool _empty{true};

public:

    typedef T ValueType;

    Optional() = default;
    explicit Optional(T&& value) {
        emplace(std::move(value));
    }

    Optional(const Optional& other) {
        if (not other._empty) {
            this->_empty = false;
            new(this->_memory) T(other.get());
        }
    }

    Optional(Optional&& other)  noexcept {
        if (not other._empty) {
            this->_empty = false;
            new(this->_memory) T(std::move(other.get()));
            other._empty = true;
        }
    }

    Optional& operator = (Optional&& other)  noexcept {
        reset();
        if (not other._empty) {
            this->_empty = false;
            new(this->_memory) T(std::move(other.get()));
            other._empty = true;
        }
        return *this;
    }

    Optional& operator = (const Optional& other) {
        reset();
        if (not other._empty) {
            this->_empty = false;
            new(this->_memory) T(other.get());
        }
        return *this;
    }

    ~Optional() {
        reset();
    }

    bool empty() const noexcept {
        return _empty;
    }

    void reset() noexcept {
        if (!this->_empty) {
            get().~T();
            this->_empty = true;
        }
    }

    template<typename... Args>
    void emplace(Args&&... args) {
        reset();
        this->_empty = false;
        new(this->_memory) T(std::forward<Args>(args)...);
    }

    T& get() noexcept {
        return *reinterpret_cast<T*>(_memory);
    }

    const T& get() const {
        return *reinterpret_cast<const T*>(_memory);
    }

    T release() noexcept { return std::move(get()); }
};

template<>
class Optional<void>
{
    bool _empty{true};

public:

    typedef std::nullptr_t ValueType;

    Optional() = default;
    explicit Optional(std::nullptr_t) {
        _empty = false;
    }

    Optional(const Optional& other) {
        this->_empty = other._empty;
    }

    Optional(Optional&& other)  noexcept {
        this->_empty = other._empty;
        other._empty = true;
    }

    Optional& operator = (Optional&& other)  noexcept {
        this->_empty = other._empty;
        other._empty = true;
        return *this;
    }

    Optional& operator = (const Optional& other) = default;

    ~Optional() {
        reset();
    }

    bool empty() const noexcept {
        return _empty;
    }

    void reset() noexcept {
        this->_empty = true;
    }

    void emplace(std::nullptr_t ptr = nullptr) {
        reset();
        this->_empty = false;
    }

    std::nullptr_t get() const noexcept { return nullptr; } // NOLINT

    std::nullptr_t release() noexcept { return nullptr; } // NOLINT
};

} // namespace jinx

#endif
