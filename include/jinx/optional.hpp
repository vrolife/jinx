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
