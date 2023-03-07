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
#ifndef __raii_hpp__
#define __raii_hpp__

#define JINX_RAII_SIMPLE_OBJECT(ObjectName, ObjectType, __free) \
class ObjectName \
{ \
    ObjectType *_obj{nullptr}; \
 \
public: \
    ObjectName() = default; \
    ObjectName(ObjectName&& other) noexcept { \
        _obj = other.release(); \
    } \
    JINX_NO_COPY(ObjectName); \
    explicit ObjectName(ObjectType *obj) : _obj(obj) { } \
    ~ObjectName() { \
        reset(); \
    } \
     \
    operator ObjectType *() { \
        return _obj; \
    } \
 \
    void reset() noexcept { \
        if (_obj != nullptr) { \
            __free(_obj); \
            _obj = nullptr; \
        } \
    } \
 \
    ObjectType * get() noexcept { return _obj; } \
 \
    ObjectName& operator=(ObjectName&& other) noexcept { \
        reset(); \
        _obj = other.release(); \
        return *this; \
    } \
 \
    ObjectName& operator=(ObjectType* other) noexcept { \
        reset(); \
        _obj = other; \
        return *this; \
    } \
 \
    ObjectType * release() noexcept { \
        auto* tmp = _obj; \
        _obj = nullptr; \
        return tmp; \
    } \
 \
    ObjectType ** address() noexcept { \
        return &_obj; \
    } \
    operator bool() const noexcept { return _obj != nullptr; } \
    ObjectType* operator ->() noexcept { return _obj; } \
    const ObjectType* operator ->() const noexcept { return _obj; } \
};

#define JINX_RAII_SHARED_OBJECT(ObjectName, ObjectType, __ref, __unref) \
class ObjectName \
{ \
    ObjectType* _obj{nullptr}; \
public: \
 \
   ObjectName() = default; \
    explicit ObjectName(ObjectType* obj) : _obj(obj){ } \
 \
    ObjectName(const ObjectName& other) { \
        _obj = other._obj; \
        ref(); \
    } \
 \
    ObjectName(ObjectName&& other)  noexcept { \
        _obj = other.release(); \
    } \
 \
    ~ObjectName() { \
        unref(); \
    } \
 \
    ObjectName& operator=(const ObjectName& other) { \
        unref(); \
        _obj = other._obj; \
        ref(); \
        return *this; \
    } \
 \
    ObjectName& operator=(ObjectName&& other) noexcept { \
        unref(); \
        _obj = other.release(); \
        return *this; \
    } \
 \
    void ref() { \
        if (_obj != nullptr) { \
            __ref(_obj); \
        } \
    } \
 \
     void unref() { \
        if (_obj != nullptr) { \
            __unref(_obj); \
        } \
    } \
 \
    ObjectType* release() noexcept { \
        auto* obj = _obj; \
        _obj = nullptr; \
        return obj; \
    } \
 \
    ObjectType ** address() noexcept { \
        return &_obj; \
    } \
 \
    ObjectType* get() const noexcept { \
        return _obj; \
    } \
    void reset() noexcept { unref(); _obj = nullptr; } \
    void reset(ObjectType* obj) { unref(); _obj = obj; } \
\
    operator bool() noexcept { return _obj != nullptr; } \
    operator ObjectType*() noexcept { return _obj; } \
    ObjectType* operator ->() noexcept { return _obj; } \
}

#endif
