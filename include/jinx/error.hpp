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
