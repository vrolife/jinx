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
#ifndef __pointer_hpp__
#define __pointer_hpp__

#include <cstdlib>

#include <type_traits>

#include <jinx/error.hpp>

namespace jinx {
namespace pointer {

template<typename T>
class PointerShared
{
private:
    T* _ptr;

    template<typename Type>
    inline
    static
    typename std::enable_if<
        Type::SharedPointerRelease::value
    >::
    type release(T* ptr) {
        T::shared_pointer_release(ptr);
    }

    template<typename Type>
    inline
    static
    void release(Type* ptr) {
        delete ptr;
    }

    template<typename Type>
    inline
    static
    typename std::enable_if<
        Type::SharedPointerGetPointer::value,
        typename Type::PointerType
    >::type get_pointer(T* ptr) {
        return T::shared_pointer_get_pointer(ptr);
    }

    template<typename Type>
    inline
    static
    Type* get_pointer(Type* ptr) {
        return ptr;
    }

    template<typename Type>
    inline
    static
    typename std::enable_if<Type::SharedPointerCheckSingleReference::value>::
    type check_signle_reference(T* ptr) {
        ptr->shared_pointer_check_signle_reference();
    }

    template<typename Type>
    inline
    static
    std::nullptr_t check_signle_reference(Type* ptr) { return nullptr; }

public:
    typedef T ValueType;

    PointerShared() : _ptr(nullptr) { }
    explicit PointerShared(T* ptr) : _ptr(ptr) { }

    PointerShared(PointerShared&& other) noexcept {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }

    PointerShared& operator =(PointerShared&& other) noexcept {
        unref();
        _ptr = other._ptr;
        other._ptr = nullptr;
        return *this;
    }

    PointerShared(const PointerShared& other) {
        _ptr = other._ptr;
        ref();
    }

    PointerShared& operator =(const PointerShared& other) {
        unref();
        _ptr = other._ptr;
        ref();
        return *this;
    }

    ~PointerShared() {
        unref();
    }

    static
    PointerShared shared_from_this(T* self) {
        jinx_assert(self != nullptr);
        PointerShared ptr{self};
        ptr.ref();
        return ptr;
    }

    T* release() noexcept {
        auto* tmp = _ptr;
        _ptr = nullptr;
        return tmp;
    }

    void reset(T* ptr = nullptr) {
        unref();
        _ptr = ptr;
    }

    void replace(T* ptr) noexcept {
        _ptr = ptr;
    }

    void unref() {
        if (_ptr == nullptr) {
            return;
        }
        auto refc = _ptr->_refc.fetch_sub(1);
        if (refc == 1) {
            auto* ptr = _ptr;
            _ptr = nullptr;
            this->release<T>(ptr);

        } else if (
            not std::is_same<decltype(this->check_signle_reference<T>(_ptr)), std::nullptr_t>::value 
            and refc == 2
        ) {
            this->check_signle_reference<T>(_ptr);

        } else if (refc == 0) {
            error::fatal("shared pointer double free");
        }
    }

    void ref() {
        if (_ptr == nullptr) {
            return;
        }
        auto refc = _ptr->_refc.fetch_add(1);
        if (refc <= 0) {
            error::fatal("reference after free");
        }
    }

    T* get() noexcept { return _ptr; }

    operator T*() noexcept { return _ptr; } // NOLINT
    operator bool() const noexcept { return _ptr != nullptr; } // NOLINT

    decltype(get_pointer<T>(std::declval<T*>())) operator ->() noexcept { return get_pointer<T>(_ptr); }
    
    T& operator *() noexcept {
        return *_ptr;
    }

    bool operator ==(const PointerShared& other) const noexcept {
        return _ptr == other._ptr;
    }

    bool operator !=(const PointerShared& other) const noexcept {
        return _ptr != other._ptr;
    }
};

static_assert(sizeof(PointerShared<void*>) == sizeof(void*), "should be no cost");

template<typename T>
class PointerAutoDestructor {
    T* _ptr{nullptr};
public:
    PointerAutoDestructor() = default;
    explicit PointerAutoDestructor(T* ptr) :_ptr(ptr) { }
    PointerAutoDestructor(const PointerAutoDestructor& other) = delete;
    PointerAutoDestructor& operator =(const PointerAutoDestructor& other) = delete;
    PointerAutoDestructor(PointerAutoDestructor&& other) noexcept {
        _ptr = other._ptr;
        other._ptr = nullptr;
    }
    
    PointerAutoDestructor& operator=(PointerAutoDestructor&& other) noexcept {
        reset(other._ptr);
        other._ptr = nullptr;
    }

    ~PointerAutoDestructor() {
        reset();
    }

    void reset(T* ptr = nullptr) {
        if (_ptr != nullptr) {
            _ptr->~T();
        }
        _ptr = ptr;
    }
    
    void replace(T* ptr) noexcept {
        _ptr = ptr;
    }

    T* get() noexcept { return _ptr; }
    
    operator T*() noexcept { return _ptr; } // NOLINT
    
    operator bool() const noexcept { return _ptr != nullptr; } // NOLINT

    T* operator->() noexcept {
        return _ptr;
    }
    
    T& operator *() noexcept {
        return *_ptr;
    }
    
    bool operator ==(const PointerAutoDestructor& other) const noexcept {
        return _ptr == other._ptr;
    }

    bool operator !=(const PointerAutoDestructor& other) const noexcept {
        return _ptr != other._ptr;
    }
};

} // namespace pointer
} // namespace jinx

#endif
