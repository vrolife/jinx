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
#ifndef __jinx_error_hpp__
#define __jinx_error_hpp__

#include <cstddef>
#include <type_traits>

#include <jinx/macros.hpp>

#define JINX_ERROR_IMPLEMENT(error_name, message_body) \
struct Category_##error_name : jinx::error::Category { \
    const char* name() const noexcept override { \
        return #error_name; \
    } \
    const char* message(const jinx::error::Error& code) const noexcept override { \
        message_body; \
    } \
}; \
static const Category_##error_name __category_##error_name; \
const jinx::error::Category& category_##error_name() noexcept { \
    return __category_##error_name; \
}

#define JINX_ERROR_DEFINE(name, enum_name) \
const jinx::error::Category& category_##name() noexcept; \
inline jinx::error::Error make_error(enum_name enum_value) noexcept { \
    return jinx::error::Error{static_cast<int>(enum_value), category_##name()}; \
}

namespace jinx {
namespace error {

struct JinxErrorAbort { };

[[noreturn]]
void error(const char* message);

[[noreturn]]
void fatal(const char* message);

template<typename T>
struct is_error_enum : public std::false_type { };

class Error;

struct Category {
    constexpr Category() noexcept = default;
    virtual ~Category() = default;
    
    JINX_NO_COPY_NO_MOVE(Category);

    virtual const char* name() const noexcept = 0;
    virtual const char* message(const Error& code) const noexcept = 0;

    bool operator==(const Category& other) const noexcept { return this == &other; }
    bool operator!=(const Category& other) const noexcept { return this != &other; }
};

const Category& category_uninitialized() noexcept;

class Error {
    const Category* _category{};
    int _value{};

public:
    Error() noexcept : _value(0), _category(&category_uninitialized()) { }
    Error(int value, const Category& category) noexcept
    : _value(value), _category(&category) { }

    Error(const Error& other) noexcept = default;
    Error(Error&& other) noexcept {
        _category = other._category;
        _value = other._value;
        other.clear();
    }

    ~Error() noexcept = default;

    Error& operator=(const Error& other) noexcept = default;
    Error& operator=(Error&& other) noexcept {
        _category = other._category;
        _value = other._value;
        other.clear();
        return *this;
    }

    void clear() noexcept {
        _value = 0;
        _category = nullptr;
    }

    int value() const noexcept { return _value; }

    const Category& category() const noexcept { return *_category; }

    void assign(int value, const Category& category) noexcept {
        _value = value;
        _category = &category;
    }

    template<typename T>
    typename std::enable_if<std::is_enum<T>::value, T>::
    type as() const noexcept {
        return static_cast<T>(_value);
    }

    template<typename T>
    typename std::enable_if<not std::is_enum<T>::value, T>::
    type as() const {
        return T(_value);
    }

    const char* message() const noexcept {
        return _category->message(*this);
    }

    explicit operator bool() const noexcept { return _value != 0; }

    bool operator ==(const Error& other) const {
        return _value == other._value and _category == other._category;
    }

    bool operator !=(const Error& other) const {
        return !(*this == other);
    }
};

} // namespace error
} // namespace jinx

#endif
