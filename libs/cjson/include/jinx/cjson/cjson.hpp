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
