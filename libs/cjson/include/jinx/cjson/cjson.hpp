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
#ifndef __jinx_cjson_hpp__
#define __jinx_cjson_hpp__

#include <cstdlib>

#include <jinx/macros.hpp>

#include "cJSON.h"

namespace jinx {
namespace cjson {

class JSON_String
{
    typedef char* ValueType;
    ValueType _value{nullptr};

public:
    JSON_String() = default;
    JSON_String(JSON_String&& other) noexcept {
        _value = other.release();
    }
    JINX_NO_COPY(JSON_String);
    explicit JSON_String(ValueType value) : _value(value) { }
    ~JSON_String() {
        free();
    }
    
    operator ValueType () { // NOLINT
        return _value;
    }

    void free() {
        if (_value != nullptr) {
            ::free(_value); // NOLINT
            _value = nullptr;
        }
    }

    ValueType  get() { return _value; }

    JSON_String& operator=(JSON_String&& other) noexcept {
        free();
        _value = other.release();
        return *this;
    }

    ValueType  release() {
        auto* tmp = _value;
        _value = nullptr;
        return tmp;
    }

    ValueType * address() {
        return &_value;
    }
};

class JSON_Object
{
    typedef cJSON* ValueType;
    ValueType _value{nullptr};

public:
    JSON_Object() = default;
    JSON_Object(JSON_Object&& other) noexcept {
        _value = other.release();
    }
    JINX_NO_COPY(JSON_Object);
    explicit JSON_Object(ValueType value) : _value(value) { }
    ~JSON_Object() {
        free();
    }
    
    operator ValueType () { // NOLINT
        return _value;
    }

    void free() {
        if (_value != nullptr) {
            cJSON_Delete(_value);
            _value = nullptr;
        }
    }

    ValueType  get() { return _value; }

    JSON_Object& operator=(JSON_Object&& other) noexcept {
        free();
        _value = other.release();
        return *this;
    }

    ValueType  release() {
        auto* tmp = _value;
        _value = nullptr;
        return tmp;
    }

    ValueType * address() {
        return &_value;
    }

    JSON_String print() {
        return JSON_String(cJSON_Print(_value));
    }

    JSON_String print_unformatted() {
        return JSON_String(cJSON_PrintUnformatted(_value));
    }
};

} // namespace cjson
} // namespace jinx

#endif
