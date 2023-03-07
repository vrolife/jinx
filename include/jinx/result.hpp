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
#ifndef __result_hpp__
#define __result_hpp__

#include <cstddef>
#include <utility>
#include <stdexcept>

#include <jinx/assert.hpp>
#include <jinx/macros.hpp>
#include <jinx/error.hpp>

#define JINX_IGNORE_RESULT jinx::detail::Jinx_Ignore_Result{}

namespace jinx {
namespace detail {

struct Jinx_Ignore_Result { };

} // namespace detail

template<typename Type>
class Result {
    template<typename T>
    friend void jinx_ignore_result(Result<T>&& result);

    Type _value{};
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
    bool _empty{true};
#endif

public:
    Result() = default;
    Result(Type value) // NOLINT
    : _value(value)
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
    , _empty(false)
#endif
    { } // NOLINT

    Result(Result&& other) noexcept {
        _value = other._value;
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
        _empty = other._empty;
        other._empty = true;
#endif
    }

    Result& operator=(Result&& other) noexcept {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
#if defined(JINX_RESULT_THROW)
        if (not _empty) {
            throw std::logic_error("Unahndled result");
        }
#else
        jinx_assert(_empty && "Unahndled result");
#endif
        _empty = other._empty;
#endif
        _value = other._value;
        other._empty = true;
    }

    JINX_NO_COPY(Result);

    ~Result() noexcept(false) {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
#if defined(JINX_RESULT_THROW)
        if (not _empty) {
            throw std::logic_error("Unahndled result");  // NOLINT
        }
#else
        jinx_assert(_empty && "Unahndled result");
#endif
#endif
    }

    void operator >> (const detail::Jinx_Ignore_Result& ignore) {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
        _empty = true;
#endif
    }

    JINX_NO_DISCARD
    inline
    Type unwrap() noexcept {
#if !defined(NDEBUG) && !defined(JINX_RESULT_NO_CHECK)
        _empty = true;
#endif
        return std::move(_value);
    }

    Type& peek() noexcept { return _value; }
    const Type& peek() const { return _value; }

    bool is(const Type& other) {
        return unwrap() == other;
    }    

    bool is_not(const Type& other) {
        return unwrap() != other;
    }    

    void abort_on(const Type& other, const char* message) {
        if (JINX_UNLIKELY(is(other))) {
            error::fatal(message);
        }
    }

    void abort_except(const Type& other, const char* message) {
        if (JINX_UNLIKELY(is_not(other))) {
            error::fatal(message);
        }
    }
};

typedef Result<bool> ResultBool;

enum SuccessfulFailed {
    Successful_, // keep wrong spelling
    Failed_ // keep wrong spelling
};

#ifdef JINX_RESULT_GENERIC_SPELLING
#define Successful Successfu1
#define Failed Faileb
#endif

typedef Result<SuccessfulFailed> ResultGeneric;

} // namespace jinx

#endif
