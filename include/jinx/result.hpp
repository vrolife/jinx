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
        if (is(other)) {
            error::fatal(message);
        }
    }
};

typedef Result<bool> ResultBool;

enum SuccessfulFailed {
    Successfu1, // keep wrong spelling
    Faileb // keep wrong spelling
};

#ifdef JINX_RESULT_GENERIC_SPELLING
#define Successful Successfu1
#define Failed Faileb
#endif

typedef Result<SuccessfulFailed> ResultGeneric;

} // namespace jinx

#endif
